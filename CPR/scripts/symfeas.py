#!/usr/bin/env python3
import os
import sys
import argparse
from typing import List, Tuple, Set, Dict
import multiprocessing as mp
import subprocess
import json
import re
import signal

import traceback
import networkx as nx
import graphviz
import sbsv

import uni_klee
import sympatch

import enum

import pysmt.environment
from pysmt.shortcuts import Symbol, BVType, ArrayType, BV, Select, BVConcat, BVULT, Bool, Ite, And, Or, Symbol, Equals, Not, LE, LT, GE, GT, Int, is_sat
import pysmt.shortcuts as smt
from pysmt.smtlib.parser import SmtLibParser
from pysmt.smtlib.script import SmtLibScript
from pysmt.typing import BV32, BV8, BV64, INT

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def print_log(msg: str):
    print(msg, file=sys.stderr)

def print_out(msg: str):
    print(msg, file=sys.stdout)

def parse_smt2_file(file_path: str):
    pysmt.environment.push_env()
    parser = SmtLibParser()
    with open(file_path, "r") as f:
        script = parser.get_script(f)
    formulae = script.get_last_formula()
    symbols = script.get_declared_symbols()
    print_log("Formulae: ", formulae)
    print_log("Symbols: ", symbols)
    
def parse_mem_result_file(file_path: str) -> sbsv.parser:
    parser = sbsv.parser()
    parser.add_schema("[mem] [index: int] [u-addr: int] [a-addr: int]")
    parser.add_schema("[heap-check] [error] [no-mapping] [u-addr: int] [u-value: int]")
    parser.add_schema("[heap-check] [error] [value-mismatch] [u-addr: int] [u-value: int] [a-addr: int] [a-value: int]")
    parser.add_schema("[heap-check] [ok] [u-addr: int] [u-value: int] [a-addr: int] [a-value: int]")
    parser.add_schema("[heap-check] [begin]")
    parser.add_schema("[heap-check] [end]")
    parser.add_schema("[val] [arg] [index: int] [value: str] [size: int] [name: str] [num: int]")
    parser.add_schema("[val] [error] [no-mapping] [u-addr: int] [name: str]")
    parser.add_schema("[val] [error] [null-pointer] [addr: int] [name: str]")
    parser.add_schema("[val] [heap] [u-addr: int] [name: str] [value: str] [size: int] [num: int]")
    parser.add_schema("[global] [sym: str] [value: str]")
    parser.add_group("heap-check", "heap-check$begin", "heap-check$end")
    with open(file_path, "r") as f:
        try:
            parser.load(f)
            return parser
        except Exception as e:
            print_log(f"Error parsing file {file_path}: {e}")
            traceback.print_exc()
            return None
            

def find_num(dir: str, name: str) -> int:
    result = 0
    dirs = set(os.listdir(dir))
    while True:
        if f"{name}-{result}" in dirs:
            result += 1
        else:
            break
    return result

def read_config_file(file_path: str) -> Dict[str, str]:
    result = dict()
    if os.path.exists(file_path):
        with open(file_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                parts = line.split("=")
                if len(parts) == 2:
                    result[parts[0].strip()] = parts[1].strip()
    return result

def get_metadata(subject_name: str) -> dict:
    subject = None
    for sub in uni_klee.global_config.get_meta_data_list():
        if subject_name.lower() in sub["bug_id"].lower():
            subject = sub
            break
    return subject

def get_conf(subject: dict, subject_dir: str) -> Dict[str, str]:
    conf = uni_klee.global_config.get_meta_data_info_by_id(subject["id"])["conf"]
    config_for_fuzz = read_config_file(os.path.join(subject_dir, "config"))
    if len(config_for_fuzz) > 0:
        conf["test_input_list"] = config_for_fuzz["cmd"].replace("<exploit>", "$POC")
        conf["poc_path"] = os.path.basename(config_for_fuzz["exploit"])
    return conf


def run_fuzzer_multi(subject: dict, subject_dir: str, debug: bool = False):
    # Find subject
    conf = get_conf(subject, subject_dir)
    print_log(conf)
    runtime_dir = os.path.join(subject_dir, "runtime")
    out_no = find_num(runtime_dir, "aflrun-multi-out")
    out_dir = os.path.join(runtime_dir, f"aflrun-multi-out-{out_no}")

    in_dir = in_dir = os.path.join(subject_dir, "seed")
    if not os.path.exists(in_dir): # Single seed
        in_dir = os.path.join(runtime_dir, "in")
        if os.path.exists(in_dir):
            os.system(f"rm -rf {in_dir}")
        os.makedirs(in_dir)
        os.system(f"cp {os.path.join(subject_dir, conf['poc_path'])} {in_dir}/")
    
    env = os.environ.copy()
    env["AFL_NO_UI"] = "1"
    bin = os.path.basename(conf["binary_path"])
    opts = conf["test_input_list"].replace("$POC", "@@")
    cmd = f"timeout 12h /root/projects/AFLRun/afl-fuzz -C -i {in_dir} -o {out_dir} -m none -t 2000ms -- {runtime_dir}/{bin}.aflrun {opts}"
    print_log(f"Running fuzzer: {cmd}")
    stdout = sys.stdout if debug else subprocess.DEVNULL
    stderr = sys.stderr # if debug else subprocess.DEVNULL
    proc = subprocess.run(cmd, shell=True, cwd=runtime_dir, env=env, stdout=stdout, stderr=stderr)
    if proc.returncode != 0:
        print_log(f"Fuzzer failed {proc.stderr}")
    print_log("Fuzzer finished")
    collect_val_runtime(subject_dir, out_dir)


def run_fuzzer(subject: dict, subject_dir: str, debug: bool = False):
    # Find subject
    conf = get_conf(subject, subject_dir)
    print_log(conf)
    runtime_dir = os.path.join(subject_dir, "runtime")
    out_no = find_num(runtime_dir, "aflrun-out")
    out_dir = os.path.join(runtime_dir, f"aflrun-out-{out_no}")

    in_dir = os.path.join(runtime_dir, "in")
    if os.path.exists(in_dir):
        os.system(f"rm -rf {in_dir}")
    os.makedirs(in_dir)
    os.system(f"cp {os.path.join(subject_dir, conf['poc_path'])} {in_dir}/")
    
    env = os.environ.copy()
    env["AFL_NO_UI"] = "1"
    bin = os.path.basename(conf["binary_path"])
    opts = conf["test_input_list"].replace("$POC", "@@")
    cmd = f"timeout 12h /root/projects/AFLRun/afl-fuzz -C -i {in_dir} -o {out_dir} -m none -t 2000ms -- {runtime_dir}/{bin}.aflrun {opts}"
    print_log(f"Running fuzzer: {cmd}")
    stdout = sys.stdout if debug else subprocess.DEVNULL
    stderr = sys.stderr # if debug else subprocess.DEVNULL
    proc = subprocess.run(cmd, shell=True, cwd=runtime_dir, env=env, stdout=stdout, stderr=stderr)
    if proc.returncode != 0:
        print_log(f"Fuzzer failed {proc.stderr}")
    print_log("Fuzzer finished")
    collect_val_runtime(subject_dir, out_dir)


def collect_val_runtime(subject_dir: str, out_dir: str):
    print_log(f"Collecting val runtime from {out_dir}")
    # Collect results in val-runtime
    conc_inputs_dir = os.path.join(subject_dir, "concrete-inputs")
    if os.path.exists(conc_inputs_dir):
        os.system(f"rm -rf {conc_inputs_dir}")
    os.makedirs(conc_inputs_dir, exist_ok=True)
    # Copy from crashes
    os.system(f"rsync -az {out_dir}/default/crashes/ {conc_inputs_dir}/")
    # Copy from queue
    os.system(f"rsync -az {out_dir}/default/queue/ {conc_inputs_dir}/")
    
def clear_val(dir: str):
    files = os.listdir(dir)
    for f in files:
        file = os.path.join(dir, f)
        if f.startswith("core.") and os.path.isfile(file):
            os.remove(file)

def run_val(subject: dict, subject_dir: str, symvass_prefix: str, val_prefix: str, debug: bool = False):
    conf = get_conf(subject, subject_dir)
    val_runtime = os.path.join(subject_dir, "val-runtime")
    val_bin = os.path.join(val_runtime, os.path.basename(conf["binary_path"]))
    val_out_no = find_num(val_runtime, val_prefix)
    val_out_dir = os.path.join(val_runtime, f"{val_prefix}-{val_out_no}")
    
    out_no = find_num(os.path.join(subject_dir, "patched"), symvass_prefix)
    out_dir = os.path.join(subject_dir, "patched", f"{symvass_prefix}-{out_no - 1}")
    if os.path.exists(os.path.join(out_dir, "base-mem.symbolic-globals")):
        with open(os.path.join(out_dir, "base-mem.symbolic-globals")) as f:
            globals = f.readlines()
            if len(globals) > 1:
                # with open("/root/projects/CPR/out/out.txt", "a") as f:
                #     f.write(f"{subject_dir} {len(globals)}\n")
                pass
    symin_cluster_json = os.path.join(out_dir, "symin-cluster.json")
    if not os.path.exists(symin_cluster_json):
        print_log(f"symin-cluster.json not found in {out_dir}")
        return
    with open(symin_cluster_json, "r") as f:
        data = json.load(f)
    
    os.makedirs(val_out_dir, exist_ok=True)
    cluster = data["mem_cluster"]
    
    with open(os.path.join(val_out_dir, "val.json"), "w") as f:
        save_obj = dict()
        save_obj["uni_klee_out_dir"] = out_dir
        save_obj["val_out_dir"] = val_out_dir
        save_obj["cluster"] = cluster
        json.dump(save_obj, f, indent=2)
        
    conc_inputs_dir = os.path.join(subject_dir, "concrete-inputs")
    if val_prefix == "cludafl-queue":
        conc_inputs_dir = os.path.join(val_runtime, "..", "runtime", "cludafl-queue", "queue")
    elif val_prefix == "cludafl-memory":
        conc_inputs_dir = os.path.join(val_runtime, "..", "runtime", "cludafl-memory", "input")
    print_log(f"Conc inputs dir: {conc_inputs_dir}")
    cinputs = os.listdir(conc_inputs_dir)
    tmp_inputs_dir = os.path.join(val_out_dir, "inputs")
    os.makedirs(tmp_inputs_dir, exist_ok=True)
    
    for i, c in enumerate(cluster):
        print_log(f"Processing cluster {i}")
        file = c["file"]
        nodes = c["nodes"]
        group_out_dir = os.path.join(val_out_dir, f"group-{i}")
        os.makedirs(group_out_dir, exist_ok=True)
        env = os.environ.copy()
        env["UNI_KLEE_MEM_BASE_FILE"] = os.path.join(out_dir, "base-mem.graph")
        env["UNI_KLEE_MEM_FILE"] = file
        for cid, cinput in enumerate(cinputs):
            original_file = os.path.join(conc_inputs_dir, cinput)
            if os.path.isdir(original_file):
                continue
            if cinput == "README.txt":
                continue
            c_file = os.path.join(tmp_inputs_dir, f"val{cid}")
            if os.path.exists(c_file):
                os.unlink(c_file)
            os.link(original_file, c_file)
            env_local = env.copy()
            local_out_file = os.path.join(group_out_dir, f"val-{cid}.txt")
            env_local["UNI_KLEE_MEM_RESULT"] = local_out_file
            env_str = f"UNI_KLEE_MEM_BASE_FILE={env['UNI_KLEE_MEM_BASE_FILE']} UNI_KLEE_MEM_FILE={env['UNI_KLEE_MEM_FILE']} UNI_KLEE_MEM_RESULT={local_out_file}"
            if "test_input_list" in conf:
                use_stdin = conf["test_input_list"].find("$POC") == -1
                target_cmd = conf["test_input_list"].replace("$POC", c_file)
                if use_stdin:
                    target_cmd = f"cat {c_file} | {val_bin} {target_cmd}"
                else:
                    target_cmd = f"{val_bin} {target_cmd}"
            else:
                target_cmd = val_bin
            try:
                subprocess.run(target_cmd, shell=True, env=env_local, cwd=val_runtime, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=10)
            except subprocess.TimeoutExpired:
                print_log(f"Timeout for {target_cmd}")
                continue
            except Exception as e:
                print_log(f"Error for {target_cmd}: {e}")
                continue
            print_log(f"Finished {cid}: {env_str} {target_cmd}")
            if os.path.exists(local_out_file):
                with open(local_out_file, "a") as f:
                    f.write(f"[input] [id {cid}] [symgroup {i}] [file {cinput}]")
        clear_val(val_runtime)


def load_smt_file(file_path: str) -> Tuple[SmtLibScript, pysmt.environment.Environment]:
    pysmt.environment.push_env()
    cur_env = pysmt.environment.get_env()
    parser = SmtLibParser(cur_env)
    with open(file_path, "r") as f:
        script = parser.get_script(f)
    return script, cur_env

def get_var_from_script(script: SmtLibScript, name: str, cur_env: pysmt.environment.Environment):
    # Access the parser's cache which contains declared variables
    existing_symbol = None
    for s in script.get_declared_symbols():
        if s.symbol_name() == name:
            existing_symbol = s
            break
    return existing_symbol


def get_bv_const(cur_env: pysmt.environment.Environment, value: str, size: int):
    bytes_data = bytearray.fromhex(value)
    int_val = int.from_bytes(bytes_data, byteorder="little")
    bv = cur_env.formula_manager.BV(int_val, size * 8)
    return bv


def read_val_out_file(parser: sbsv.parser, globals_list: List[Dict[str, str]], iter: int, index: Tuple[int, int], script: SmtLibScript, cur_env: pysmt.environment.Environment):
    # Check error
    if len(parser.get_result_by_index("heap-check$error$no-mapping", index)) > 0: #  or len(result["val"]["error"]["no-mapping"]) > 0
        print_log("Memory mismatch")
        return "MEM_MISMATCH"
    if len(parser.get_result_by_index("val$error$null-pointer", index)) > 0:
        print_log("Null pointer")
        return "NULL_PTR"
    if len(parser.get_result_by_index("heap-check$error$value-mismatch", index)) > 0:
        print_log("Value mismatch")
        return "VAL_MISMATCH"
    
    # Read values and check satisfiability
    formula = script.get_last_formula()
    for heap in parser.get_result_by_index("val$heap", index):
        addr = heap["u-addr"]
        name = heap["name"]
        value = heap["value"]
        size = heap["size"]
        num = heap["num"]
        print_log(f"Heap: {addr} {name} {value} {size} {num}")
        # Build the bitvector formula
        # value: hex string little endian
        # size: number of bytes
        # num: number of elements
        # Get variable already declared in script
        val = get_bv_const(cur_env, value, size)
        var = get_var_from_script(script, name, cur_env)
        if var is None:
            print_log(f"Variable {name} not found")
            continue
        bv = None
        for i in range(size):
            index_bv = cur_env.formula_manager.BV(i, 32)
            byte_bv = cur_env.formula_manager.Select(var, index_bv)
            if bv is None:
                bv = byte_bv
            else:
                bv = cur_env.formula_manager.BVConcat(byte_bv, bv)
        # Add constraint
        eq = Equals(bv, val)
        formula = And(formula, eq)
    
    for arg in parser.get_result_by_index("val$arg", index):
        index = arg["index"]
        value = arg["value"]
        size = arg["size"]
        name = arg["name"]
        print_log(f"Arg: {index} {value} {size} {name}")
        val = get_bv_const(cur_env, value, size)
        var = get_var_from_script(script, name, cur_env)
        if var is None:
            print_log(f"Variable {name} not found")
            continue
        bv = None
        for i in range(size):
            index_bv = cur_env.formula_manager.BV(i, 32)
            byte_bv = cur_env.formula_manager.Select(var, index_bv)
            if bv is None:
                bv = byte_bv
            else:
                bv = cur_env.formula_manager.BVConcat(byte_bv, bv)
        eq = cur_env.formula_manager.Equals(bv, val)
        formula = cur_env.formula_manager.And(formula, eq)
    
    if len(globals_list) > iter:
        for name, value in globals_list[iter].items():
            var = get_var_from_script(script, name, cur_env)
            if var is None:
                print_log(f"Variable {name} not found")
                continue
            size = len(value) // 2
            val = get_bv_const(cur_env, value, size)
            bv = None
            for i in range(size):
                index_bv = cur_env.formula_manager.BV(i, 32)
                byte_bv = cur_env.formula_manager.Select(var, index_bv)
                if bv is None:
                    bv = byte_bv
                else:
                    bv = cur_env.formula_manager.BVConcat(byte_bv, bv)
            eq = cur_env.formula_manager.Equals(bv, val)
            formula = cur_env.formula_manager.And(formula, eq)
    
    if is_sat(formula):
        print_log("SAT")
        return "SAT"
    else:
        print_log("UNSAT")
        return "UNSAT"

def parse_symvass_result(file_path: str) -> Dict[int, List[int]]:
    result = dict()
    parser = sbsv.parser()
    parser.add_schema("[sym-in] [id: int] [base: int] [test: int] [cnt: int] [patches: str]")
    # parser.add_schema("[sym-out] [best] [cnt: int] [patches: str]")
    parser.add_schema("[meta-data] [default] [correct: int] [all-patches: int] [sym-input: int] [is-correct: bool] [patches: str]")
    with open(file_path, "r") as f:
        parser.load(f)
    for sym_in in parser.get_result()["sym-in"]:
        test = sym_in["test"]
        patches = sym_in["patches"]
        result[test] = eval(patches)
    result[-1] = list(range(parser.get_result()["meta-data"]["default"][0]["all-patches"]))
    return result
        
def parse_val_results(val_out_dir: str):
    if not os.path.exists(val_out_dir) or not os.path.exists(os.path.join(val_out_dir, "val.json")):
        print_log(f"Val out dir or {val_out_dir}/val.json not found")
        return
    with open(os.path.join(val_out_dir, "val.json"), "r") as f:
        result = json.load(f)
    uni_klee_out_dir = result["uni_klee_out_dir"]
    val_out_dir = result["val_out_dir"]
    cluster = result["cluster"]
    result = dict()
    result["uni_klee_out_dir"] = uni_klee_out_dir
    result["val_out_dir"] = val_out_dir
    result["val"] = list()
    rf = open(os.path.join(val_out_dir, "result.sbsv"), "w")
    remaining_base_states = list()
    result["remaining_patches"] = list()
    symvass_result = parse_symvass_result(os.path.join(uni_klee_out_dir, "table_v3.sbsv"))
    for i, c in enumerate(cluster):
        print_log(f"Processing cluster {i}")
        nodes = c["nodes"]
        group_out_dir = os.path.join(val_out_dir, f"group-{i}")
        vals = os.listdir(group_out_dir)
        group_result = dict()
        group_result["group_id"] = i
        group_result["nodes"] = nodes
        group_result["nodes_result"] = list()
        result["val"].append(group_result)
        for node in nodes:
            node_file = os.path.join(uni_klee_out_dir, f"test{node:06d}.smt2")
            node_result = dict()
            node_result["node_id"] = node
            node_result["result"] = list()
            group_result["nodes_result"].append(node_result)
            succ = False
            for val in vals:
                local_out_file = os.path.join(group_out_dir, val)
                parser = parse_mem_result_file(local_out_file)
                if parser is None:
                    continue
                ends = parser.get_result_in_order(["heap-check$end"])
                if len(ends) == 0:
                    print_log(f"No heap-check end in {local_out_file}")
                    continue
                begins = parser.get_result_in_order(["heap-check$begin"])
                indices = parser.get_group_index("heap-check")
                if len(indices) == 0:
                    print_log(f"No heap-check in {local_out_file}")
                    node_result["result"].append({"val_file": val, "result": "NOT_DONE"})
                    rf.write(f"[res] [c {i}] [n {node}] [res NOT_DONE] [val {val}]\n")
                    continue
                globals_list = list()
                globals_map = dict()
                for global_var in parser.get_result()["global"]:
                    name = global_var["sym"]
                    value = global_var["value"]
                    if name in globals_map:
                        globals_list.append(globals_map.copy())
                        globals_map.clear()
                    globals_map[name] = value
                globals_list.append(globals_map)
                if len(indices) > 1:
                    print_log(f"Multiple heap-check ({len(indices)}) in {local_out_file}")
                for iter, index in enumerate(indices):
                    if succ:
                        break
                    script, cur_env = load_smt_file(node_file) # push_env
                    res = read_val_out_file(parser, globals_list, iter, index, script, cur_env)
                    pysmt.environment.pop_env()
                    if res is not None:
                        node_result["result"].append({"val_file": val, "result": res})
                        rf.write(f"[res] [c {i}] [n {node}] [res {res}] [val {val}]\n")
                        if res == "SAT":
                            succ = True
                            break
                    else:
                        node_result["result"].append({"val_file": val, "result": "ERROR"})
                        rf.write(f"[res] [c {i}] [n {node}] [res ERROR] [val {val}]\n")
            node_result["success"] = succ
            if succ:
                rf.write(f"[success] [c {i}] [n {node}]\n")
                remaining_base_states.append(node)
        print_log(f"Finished cluster {i}")
        remaining_patches = set(symvass_result[-1])
        remaining_sym_inputs = list()
        for symin in remaining_base_states:
            if symin in symvass_result:
                remaining_patches = remaining_patches.intersection(set(symvass_result[symin]))
                rf.write(f"[remaining] [input] [input {symin}] [patches {symvass_result[symin]}]\n")
        result["remaining_patches"] = sorted(list(remaining_patches))
        rf.write(f"[remaining] [patch] [patches {result['remaining_patches']}]\n")
    rf.close()
    with open(os.path.join(val_out_dir, "result.json"), "w") as f:
        json.dump(result, f, indent=2)


def analyze(subject: dict, val_out_dir: str, output: str):
    subject_dir = os.path.join(ROOT_DIR, "patches", subject["benchmark"], subject["subject"], subject["bug_id"])
    group_patches_json = os.path.join(subject_dir, "group-patches-original.json")
    if not os.path.exists(group_patches_json):
        print_log(f"Group patches file {group_patches_json} not found")
        return
    with open(group_patches_json, "r") as f:
        group_patches = json.load(f)
    filter_json = os.path.join(subject_dir, "patched", "filter", "filtered.json")
    if not os.path.exists(filter_json):
        print_log(f"Filter file {filter_json} not found")
        return
    with open(filter_json, "r") as f:
        filter_data = json.load(f)
    correct_patch = group_patches["correct_patch_id"]
    patch_eq_map = dict()
    patch_group_tmp = dict()
    for patches in group_patches["equivalences"]:
        representative = patches[0]
        for patch in patches:
            patch_group_tmp[patch] = representative
    for patch in filter_data["remaining"]:
        if patch in patch_group_tmp:
            patch_eq_map[patch] = patch_group_tmp[patch]
        else:
            patch_eq_map[patch] = patch
    all_patches = set()
    for patch in filter_data["remaining"]:
        if patch == patch_eq_map[patch]:
            all_patches.add(patch)
    correct_patch = patch_eq_map[correct_patch]
    
    result_file = os.path.join(val_out_dir, "result.sbsv")
    if not os.path.exists(result_file):
        print_log(f"Result file {result_file} not found")
        return
    parser = sbsv.parser()
    parser.add_schema("[res] [c: int] [n: int] [res: str] [val: str]")
    parser.add_schema("[success] [c: int] [n: int]")
    parser.add_schema("[remaining] [input] [input: int] [patches: str]")
    parser.add_schema("[remaining] [patch] [patches: str]")
    with open(result_file, "r") as f:
        result = parser.load(f)
    remaining_inputs = len(result["remaining"]["input"])
    if remaining_inputs == 0:
        print_log("No remaining inputs")
        found = correct_patch in all_patches
        res = f"{subject['subject']}\t{subject['bug_id']}\t{remaining_inputs}\t{len(all_patches)}\t{found}\t{all_patches}"
        if output != "":
            with open(os.path.join(f"{ROOT_DIR}/out", output), "a") as f:
                f.write(res + "\n")
        else:
            print_out(res)
        return
    
    remaining_patches = eval(result["remaining"]["patch"][0]["patches"])
    remaining_patches_filtered = list()
    for patch in remaining_patches:
        if patch in all_patches:
            remaining_patches_filtered.append(patch)

    correct_patch = subject["correct"]["no"]
    found = correct_patch in remaining_patches_filtered
    # val_inputs val_remaining_patches val_filtered val_found
    res = f"{subject['subject']}\t{subject['bug_id']}\t{remaining_inputs}\t{len(remaining_patches_filtered)}\t{found}\t{remaining_patches_filtered}"
    if output != "":
        with open(os.path.join(f"{ROOT_DIR}/out", output), "a") as f:
            f.write(res + "\n")
    else:
        print_out(res)

class TokenType(enum.Enum):
    IDENTIFIER = 'IDENTIFIER'
    NUMBER = 'NUMBER'
    LE = 'LE'          # <=
    GE = 'GE'          # >=
    LT = 'LT'          # <
    GT = 'GT'          # >
    EQ = 'EQ'          # ==
    NE = 'NE'          # !=
    AND = 'AND'        # &&
    OR = 'OR'          # ||
    # NOT = 'NOT'      # ! (optional)
    LPAREN = 'LPAREN'  # (
    RPAREN = 'RPAREN'  # )
    PLUS = 'PLUS'      # +
    MINUS = 'MINUS'    # -
    MUL = 'MUL'        # *
    DIV = 'DIV'        # / # Be careful with INT vs REAL division
    EOF = 'EOF'        # End of File/Input

# --- Token class (remains the same) ---
class Token:
    def __init__(self, type, value=None, lineno=1, col=1):
        self.type = type
        self.value = value
        self.lineno = lineno
        self.col = col
    def __str__(self):
        return f"Token({self.type.name}, {repr(self.value)}, L{self.lineno} C{self.col})"

# --- Lexer class (assuming it correctly tokenizes +, -, *, / and handles negative numbers) ---
# (Lexer code from the previous response should be sufficient)
class Lexer:
    def __init__(self, text):
        self.text = text
        self.pos = 0
        self.current_char = self.text[self.pos] if self.pos < len(self.text) else None
        self.lineno = 1
        self.col = 1

    def advance(self):
        if self.current_char == '\n':
            self.lineno += 1
            self.col = 0
        self.pos += 1
        if self.pos < len(self.text):
            self.current_char = self.text[self.pos]
            self.col += 1
        else:
            self.current_char = None # EOF

    def peek(self):
        peek_pos = self.pos + 1
        if peek_pos < len(self.text):
            return self.text[peek_pos]
        else:
            return None

    def skip_whitespace(self):
        while self.current_char is not None and self.current_char.isspace():
            self.advance()

    def number(self):
        result = ''
        start_col = self.col
        is_negative = False
        # Handle potential negative sign ONLY at the start of the number sequence
        if self.current_char == '-':
             # Ensure it's followed by a digit to be a negative number, not just minus operator
             peek_char = self.peek()
             if peek_char is not None and peek_char.isdigit():
                 is_negative = True
                 result += self.current_char
                 self.advance()
             else:
                 # If '-' is not followed by digit, it's likely a MINUS operator, handle in get_next_token
                 self._error(f"Invalid character sequence starting with '-'")


        if self.current_char is None or not self.current_char.isdigit():
             # This error should ideally be caught before calling number() unless it's the negative sign case
             self._error(f"Expected digit at L{self.lineno} C{self.col}")

        while self.current_char is not None and self.current_char.isdigit():
            result += self.current_char
            self.advance()

        # Check for decimal point if supporting Floats/Reals
        # if self.current_char == '.':
        #    ... handle floating point ...
        #    return Token(TokenType.REAL_NUMBER, float(result), ...)

        return Token(TokenType.NUMBER, int(result), self.lineno, start_col)


    def identifier(self):
        result = ''
        start_col = self.col
        while self.current_char is not None and (self.current_char.isalnum() or self.current_char == '_'):
            result += self.current_char
            self.advance()
        # Check for keywords (like TRUE, FALSE if needed)
        # type = KEYWORDS.get(result, TokenType.IDENTIFIER)
        # return Token(type, result, ...)
        return Token(TokenType.IDENTIFIER, result, self.lineno, start_col)

    def _error(self, message="Invalid character"):
        char = self.current_char if self.current_char is not None else 'EOF'
        raise ValueError(f"{message}: '{char}' at L{self.lineno} C{self.col}")

    def get_next_token(self):
        while self.current_char is not None:
            start_col = self.col

            if self.current_char.isspace():
                self.skip_whitespace()
                continue

            # Handle multi-character operators first
            if self.current_char == '<':
                if self.peek() == '=':
                    self.advance()
                    self.advance()
                    return Token(TokenType.LE, '<=', self.lineno, start_col)
                self.advance()
                return Token(TokenType.LT, '<', self.lineno, start_col)
            if self.current_char == '>':
                if self.peek() == '=':
                    self.advance()
                    self.advance()
                    return Token(TokenType.GE, '>=', self.lineno, start_col)
                self.advance()
                return Token(TokenType.GT, '>', self.lineno, start_col)
            if self.current_char == '=':
                if self.peek() == '=':
                    self.advance()
                    self.advance()
                    return Token(TokenType.EQ, '==', self.lineno, start_col)
                self._error("Expected '=' after '=' for '=='")
            if self.current_char == '!':
                if self.peek() == '=':
                    self.advance()
                    self.advance()
                    return Token(TokenType.NE, '!=', self.lineno, start_col)
                # Handle '!' (NOT) if needed
                self._error("Expected '=' after '!' for '!=' (standalone '!' not supported)")
            if self.current_char == '&':
                if self.peek() == '&':
                    self.advance()
                    self.advance()
                    return Token(TokenType.AND, '&&', self.lineno, start_col)
                self._error("Expected '&' after '&' for '&&'")
            if self.current_char == '|':
                if self.peek() == '|':
                    self.advance()
                    self.advance()
                    return Token(TokenType.OR, '||', self.lineno, start_col)
                self._error("Expected '|' after '|' for '||'")

            # Single character tokens & Numbers/Identifiers
            if self.current_char == '+':
                self.advance()
                return Token(TokenType.PLUS, '+', self.lineno, start_col)
            if self.current_char == '-':
                 # Check if it starts a negative number
                 peek_char = self.peek()
                 if peek_char is not None and peek_char.isdigit():
                     return self.number() # number() handles the '-' sign
                 else:
                     # Otherwise, it's the MINUS operator
                     self.advance()
                     return Token(TokenType.MINUS, '-', self.lineno, start_col)
            if self.current_char == '*':
                self.advance()
                return Token(TokenType.MUL, '*', self.lineno, start_col)
            if self.current_char == '/':
                self.advance()
                return Token(TokenType.DIV, '/', self.lineno, start_col)
            if self.current_char == '(':
                self.advance()
                return Token(TokenType.LPAREN, '(', self.lineno, start_col)
            if self.current_char == ')':
                self.advance()
                return Token(TokenType.RPAREN, ')', self.lineno, start_col)

            if self.current_char.isdigit():
                return self.number()

            if self.current_char.isalpha() or self.current_char == '_':
                return self.identifier()

            # If character is unrecognized
            self._error()

        # End of file
        return Token(TokenType.EOF, None, self.lineno, self.col)

# --- Revised Parser Class ---
class Parser:
    def __init__(self, lexer, variable_type=INT):
        self.lexer = lexer
        self.current_token = self.lexer.get_next_token()
        self.variable_type = variable_type # INT or REAL
        self.variables = {} # Stores SMT variable objects keyed by name

    def _get_or_create_variable(self, name):
        if name not in self.variables:
            self.variables[name] = smt.Symbol(name, self.variable_type)
        return self.variables[name]

    def _error(self, expected_type=None):
        msg = f"Syntax error: Unexpected token {self.current_token}"
        if expected_type:
            msg += f" (expected {expected_type})"
        lineno = self.current_token.lineno if self.current_token else '?'
        col = self.current_token.col if self.current_token else '?'
        raise SyntaxError(msg + f" at L{lineno} C{col}")

    def eat(self, token_type):
        if self.current_token.type == token_type:
            # print(f"Eating: {self.current_token}") # Debugging
            self.current_token = self.lexer.get_next_token()
        else:
            self._error(expected_type=token_type.name)

    # --- Grammar Rules (Standard Precedence) ---
    # expression    ::= logic_or
    # logic_or      ::= logic_and ( OR logic_and )*
    # logic_and     ::= comparison ( AND comparison )*    # Assuming AND higher precedence than OR
    # comparison    ::= arith_expr ( COMPARISON_OP arith_expr )? # Optional comparison allows boolean results from primary
    # arith_expr    ::= term ( (PLUS | MINUS) term )*
    # term          ::= factor ( (MUL | DIV) factor )*
    # factor        ::= PLUS factor | MINUS factor | primary # Unary operators
    # primary       ::= NUMBER | IDENTIFIER | LPAREN expression RPAREN # Parentheses restart parsing

    def parse_primary(self):
        """ Parses the highest precedence items: literals, identifiers, parenthesized expressions. """
        token = self.current_token
        if token.type == TokenType.NUMBER:
            self.eat(TokenType.NUMBER)
            # Use Int() or Real() based on variable_type or token type if lexer distinguishes
            if self.variable_type == INT:
                return smt.Int(token.value)
            else: # Assume REAL if not INT
                return smt.Real(token.value) # Ensure number lexer can produce floats/reals if needed
        elif token.type == TokenType.IDENTIFIER:
            self.eat(TokenType.IDENTIFIER)
            # Could be a boolean variable later, but for now assume numeric based on variable_type
            return self._get_or_create_variable(token.value)
        elif token.type == TokenType.LPAREN:
            self.eat(TokenType.LPAREN)
            # Crucially, parentheses restart parsing at the top expression level
            node = self.parse_expression()
            self.eat(TokenType.RPAREN)
            return node
        else:
            self._error(expected_type="NUMBER, IDENTIFIER, or '('")

    def parse_factor(self):
        """ Parses unary plus/minus and then calls primary. """
        token = self.current_token
        if token.type == TokenType.PLUS: # Unary plus
            self.eat(TokenType.PLUS)
            # Unary plus usually has no effect in SMT
            return self.parse_factor()
        elif token.type == TokenType.MINUS: # Unary minus
            self.eat(TokenType.MINUS)
            node = self.parse_factor()
            # Simplify if possible, e.g., -(5) -> -5
            if node.is_constant():
                 if self.variable_type == INT and node.is_int_constant():
                      return smt.Int(-node.constant_value())
            # General case: 0 - node or Times(-1, node)
            # Times(-1, node) often preferred
            if self.variable_type == INT:
                return smt.Times(smt.Int(-1), node)
                # return smt.Minus(smt.Int(0), node) # Alternative for INT
            else: # REAL
                 return smt.Times(smt.Real(-1), node)
                 # return smt.Minus(smt.Real(0), node) # Alternative for REAL
        else:
            # No unary operator, parse the primary directly
            return self.parse_primary()

    def parse_term(self):
        """ Parses multiplication and division. """
        node = self.parse_factor()
        while self.current_token.type in (TokenType.MUL, TokenType.DIV):
            token = self.current_token
            if token.type == TokenType.MUL:
                self.eat(TokenType.MUL)
                node = smt.Times(node, self.parse_factor())
            elif token.type == TokenType.DIV:
                self.eat(TokenType.DIV)
                right_factor = self.parse_factor()
                if self.variable_type == INT:
                     # PySMT IntDiv might require specific versions or care.
                     # Using smt.Div requires REALs.
                     # Check pysmt.operators.op.DIV for integer division behavior, might truncate.
                     # For true integer division, smt.IntDiv might be needed if available.
                     # Let's use smt.Div and rely on pysmt's handling or raise error if types mismatch.
                     # Safest might be to require REAL type for division.
                     # Or raise specific error for Int division:
                    #  node = smt.IntDiv(node, right_factor)
                     raise NotImplementedError("Integer division '/' is ambiguous. Use Real type or ensure PySMT handles IntDiv appropriately.")
                else: # Assume REAL
                     node = smt.Div(node, right_factor)
        return node

    def parse_arith_expr(self):
        """ Parses addition and subtraction. """
        node = self.parse_term()
        while self.current_token.type in (TokenType.PLUS, TokenType.MINUS):
            token = self.current_token
            if token.type == TokenType.PLUS:
                self.eat(TokenType.PLUS)
                node = smt.Plus(node, self.parse_term())
            elif token.type == TokenType.MINUS:
                self.eat(TokenType.MINUS)
                node = smt.Minus(node, self.parse_term())
        return node

    def parse_comparison(self):
        """ Parses comparison operators (==, !=, <, <=, >, >=).
            If no comparison op is found, returns the result of the arith_expr,
            which must evaluate to boolean contextually (e.g., from a parenthesized bool expr).
        """
        left_node = self.parse_arith_expr()

        # Check if a comparison operator follows
        if self.current_token.type in (TokenType.LE, TokenType.GE, TokenType.LT, TokenType.GT, TokenType.EQ, TokenType.NE):
            op_token = self.current_token
            self.eat(op_token.type)
            right_node = self.parse_arith_expr() # Parse the right side arithmetic expression

            op_type = op_token.type
            if op_type == TokenType.LE:
                return smt.LE(left_node, right_node)
            elif op_type == TokenType.GE:
                return smt.GE(left_node, right_node)
            elif op_type == TokenType.LT:
                return smt.LT(left_node, right_node)
            elif op_type == TokenType.GT:
                return smt.GT(left_node, right_node)
            elif op_type == TokenType.EQ:
                return smt.Equals(left_node, right_node)
            elif op_type == TokenType.NE:
                return smt.Not(smt.Equals(left_node, right_node))
        else:
            # No comparison operator found. The result is just the left node.
            # This handles cases like `(x > 5)` being used in a logical context.
            # The type system of the surrounding logic (AND/OR) must handle it.
            # If left_node is Int/Real here, it might cause type errors later if used directly as Bool.
            # This structure relies on parse_primary correctly returning boolean nodes
            # when parsing parenthesized logical expressions like `(x < y)`.
            return left_node

    def parse_logic_and(self):
        """ Parses logical AND (&&). """
        # Assuming NOT would be handled at a higher precedence level if added (e.g., in factor/primary or a dedicated level)
        node = self.parse_comparison()
        while self.current_token.type == TokenType.AND:
            self.eat(TokenType.AND)
            # Ensure both sides are boolean compatible. PySMT handles this.
            node = smt.And(node, self.parse_comparison())
        return node

    def parse_logic_or(self):
        """ Parses logical OR (||). """
        node = self.parse_logic_and() # Parse higher precedence first
        while self.current_token.type == TokenType.OR:
            self.eat(TokenType.OR)
            # Ensure both sides are boolean compatible. PySMT handles this.
            node = smt.Or(node, self.parse_logic_and())
        return node

    # The top-level rule for a full expression
    def parse_expression(self):
        """ Parses a full logical expression (starting point). """
        return self.parse_logic_or() # Start with the lowest precedence operator

    def parse(self):
        """ Parses the entire input text. """
        self.variables.clear() # Reset variables for a new parse
        node = self.parse_expression()
        if self.current_token.type != TokenType.EOF:
            # If parsing stopped before EOF, there's extra input or a syntax error
            self._error(expected_type="EOF or operator") # Be more specific if possible
        return node

    def get_parsed_variables(self):
        """ Returns the SMT variables discovered during parsing. """
        return self.variables

def code_to_formula(code_str, variable_type=INT):
    substitutions = {}
    result_expr_str = None # Renamed to avoid confusion with SMT expr node

    # Basic preprocessing for C-like assignments
    for line in code_str.split(';'):
        line = line.strip()
        if not line: continue

        if line.startswith('result'):
            result_expr_str = line.split('=', 1)[1].strip().rstrip(';')
        elif '=' in line:
            # Simple textual substitution (use with caution)
            var, value = map(str.strip, line.split('=', 1))
            # Use word boundaries to avoid partial replacements
            substitutions[r'\b' + re.escape(var) + r'\b'] = value

    if result_expr_str is None:
         print_log(f"Warning: No 'result =' line found in code: {code_str}")
         return None # Indicate failure

    # Apply substitutions textually
    processed_expr_str = result_expr_str
    if substitutions:
        for pattern, val in substitutions.items():
            processed_expr_str = re.sub(pattern, val, processed_expr_str)
        print_log(f"Code (after substitutions): {processed_expr_str}")
    else:
         print_log(f"Code (no substitutions): {processed_expr_str}")

    # Handle trivial cases after substitution
    if processed_expr_str in ["(0)", "0"]:
        return smt.Bool(False)
    if processed_expr_str in ["(1)", "1"]:
        return smt.Bool(True)

    # --- Use the Revised Parser ---
    lexer = Lexer(processed_expr_str)
    parser = Parser(lexer, variable_type=variable_type)

    try:
        expr_node = parser.parse() # This is the SMT formula node
        # parsed_vars = parser.get_parsed_variables() # Get variables if needed
        # print_log(f"Parsed Variables: {parsed_vars}")
        return expr_node
    except (ValueError, SyntaxError, NotImplementedError) as e: # Catch lexer/parser errors
        print_log(f"Error parsing expression '{processed_expr_str}': {e}")
        return None # Indicate failure

# --- group_patches function (should be compatible) ---
# (Make sure it handles None return from code_to_formula gracefully)
def group_patches(subject_dir: str):
    meta_path = os.path.join(subject_dir, "meta-program-original.json")
    equiv_path = os.path.join(subject_dir,  "group-patches-original.json")
    os.makedirs(os.path.dirname(equiv_path), exist_ok=True)

    try:
        with open(meta_path, "r") as f:
            meta = json.load(f)
    except FileNotFoundError:
        print_log(f"Error: Meta file not found at {meta_path}")
        return
    except json.JSONDecodeError:
         print_log(f"Error: Could not decode JSON from {meta_path}")
         return

    patches_data = [] # Store parsed patch info: {id, formula, vars, name}
    correct_patch_id = -1

    print_log("Starting patch parsing...")
    for patch in meta.get("patches", []):
        id_ = patch.get("id")
        code = patch.get("code")
        name = patch.get("name")

        if id_ is None or code is None:
            print_log(f"Warning: Skipping patch with missing id or code: {patch}")
            continue

        if name == "correct":
            correct_patch_id = id_

        # --- Use the updated code_to_formula ---
        # Specify INT type since the problem context implies integers
        formula = code_to_formula(code, variable_type=INT)

        if formula is None:
            print_log(f"Warning: Failed to generate formula for Patch {id_}. Skipping.")
            continue

        # Get free variables *after* successful parsing
        try:
             # Handle constant formulas explicitly
             if formula.is_constant():
                 free_vars = set()
             else:
                free_vars = formula.get_free_variables()
        except Exception as e:
             # This might happen if formula is not a standard FNode, though unlikely here
             print_log(f"Error getting free variables for Patch {id_} formula: {e}")
             free_vars = set() # Default to empty set on error

        patches_data.append({"id": id_, "formula": formula, "vars": free_vars, "name": name})
        print_log(f"Patch {id_}: Parsed successfully.") # Formula serialization can be long

    print_log(f"\nParsed {len(patches_data)} patches successfully.")
    print_log("Starting equivalence check...")

    equivalences = {} # Map leader_id -> [list_of_equivalent_ids]
    processed_ids = set()

    for i in range(len(patches_data)):
        patch1 = patches_data[i]
        id1 = patch1["id"]
        formula1 = patch1["formula"]
        vars1 = patch1["vars"]

        if id1 in processed_ids:
            continue

        # Start a new equivalence group with patch1 as the leader
        current_equiv_group = [id1]
        equivalences[id1] = current_equiv_group
        processed_ids.add(id1)

        for j in range(i + 1, len(patches_data)):
            patch2 = patches_data[j]
            id2 = patch2["id"]
            formula2 = patch2["formula"]
            vars2 = patch2["vars"]

            if id2 in processed_ids:
                continue

            # --- Conditions for Equivalence Check ---
            # 1. Same set of free variables

            # 2. Formulas are logically equivalent (using SMT solver)
            try:
                 # Check trivial cases first
                 if formula1.is_constant() and formula2.is_constant():
                      are_equivalent = (formula1.constant_value() == formula2.constant_value())
                 else:
                    # Check F <=> G is valid (i.e., Not(F <=> G) is unsatisfiable)
                    equivalence_formula = smt.Iff(formula1, formula2)
                    # is_valid can be slow for complex formulas, might need a timeout
                    are_equivalent = smt.is_valid(equivalence_formula)
                    # Alternative using solver explicitly:
                    # with smt.Solver() as solver:
                    #    solver.add_assertion(smt.Not(equivalence_formula))
                    #    are_equivalent = not solver.solve() # UNSAT means equivalent

            except Exception as e:
                 print_log(f"SMT Error checking equivalence between {id1} and {id2}: {e}")
                 are_equivalent = False # Assume not equivalent on error

            # --- Record Equivalence ---
            if are_equivalent:
                print_log(f"  Equivalence Found: {id1} == {id2} ({formula1.serialize()} == {formula2.serialize()})")
                current_equiv_group.append(id2)
                processed_ids.add(id2) # Mark id2 as processed

    print_log(f"\nFinal Equivalence Groups Identified: {len(equivalences)}")

    # Format output for JSON
    result_list = list(equivalences.values())

    output_obj = {
        "equivalences": result_list,
        "correct_patch_id": correct_patch_id
    }

    try:
        with open(equiv_path, "w") as f:
            json.dump(output_obj, f, indent=2)
        print_log(f"Equivalences saved to {equiv_path}")
    except IOError as e:
         print_log(f"Error writing equivalences to {equiv_path}: {e}")
    
def main():
    parser = argparse.ArgumentParser(description="Symbolic Input Feasibility Analysis")
    parser.add_argument("cmd", help="Command to run", choices=["fuzz", "fuzz-seeds", "check", "fuzz-build", "val-build", "build", "extractfix-build", "vulmaster-build", "vulmaster-extractfix-build", "collect-inputs", "group-patches", "val", "feas", "analyze"])
    parser.add_argument("subject", help="Subject to run", default="")
    parser.add_argument("-i", "--input", help="Input file", default="")
    parser.add_argument("-o", "--output", help="Output file", default="")
    parser.add_argument("-d", "--debug", help="Debug mode", action="store_true")
    parser.add_argument("-s", "--symvass-prefix", help="SymVass prefix", default="uni-m-out")
    parser.add_argument("-v", "--val-prefix", help="Val prefix", default="")
    parser.add_argument("-p", "--prefix", help="Prefix of fuzzer out: default aflrun-multi-out", default="aflrun-multi-out")
    # parser.add_argument("-s", "--subject", help="Subject", default="")
    args = parser.parse_args(sys.argv[1:])
    subject = get_metadata(args.subject)
    subject_dir = os.path.join(ROOT_DIR, "patches", subject["benchmark"], subject["subject"], subject["bug_id"])
    val_prefix = args.val_prefix if args.val_prefix != "" else args.symvass_prefix
    if args.cmd == "fuzz":
        run_fuzzer(subject, subject_dir, args.debug)
    elif args.cmd == "fuzz-seeds":
        run_fuzzer_multi(subject, subject_dir, args.debug)
    elif args.cmd == "check":
        # parse_smt2_file(args.input)
        out_no = find_num(os.path.join(subject_dir, "patched"), args.symvass_prefix) - 1
        target_dir = os.path.join(ROOT_DIR, "patches", "tmp", subject["benchmark"], subject["subject"], subject["bug_id"], "patched")
        os.system(f"mv {target_dir}/high-snapshot {target_dir}/snapshot-high")
        # os.makedirs(target_dir, exist_ok=True)
        # os.system(f"rsync -avzh {os.path.join(subject_dir, 'patched', f'{args.symvass_prefix}-{out_no}')}/ {target_dir}/high-0/")
        # os.system(f"rsync -avzh {os.path.join(subject_dir, 'patched', f'snapshot-{args.symvass_prefix}')}/ {target_dir}/high-snapshot")
        # os.system(f"rsync -avzh {os.path.join(subject_dir, 'patched', 'filter')} {target_dir}/")
        # uc_no = find_num(os.path.join(subject_dir, "patched"), "uc") - 1
        # os.system(f"rsync -avzh {os.path.join(subject_dir, 'patched', f'uc-{uc_no}')}/ {target_dir}/uc-0")
    elif args.cmd == "fuzz-build":
        subprocess.run(f"./aflrun.sh", cwd=subject_dir, shell=True)
    elif args.cmd == "val-build":
        symvass_prefix = args.symvass_prefix
        if symvass_prefix == "":
            print_log(f"SymVass prefix not found (use -s or --symvass-prefix)")
            return 1
        no = find_num(os.path.join(subject_dir, "patched"), symvass_prefix)
        if no == 0:
            print_log(f"SymVass output not found for {symvass_prefix}: did you run symvass?")
            return 1
        out_dir = os.path.join(subject_dir, "patched", f"{symvass_prefix}-{no - 1}")
        env = os.environ.copy()
        env["UNI_KLEE_SYMBOLIC_GLOBALS_FILE_OVERRIDE"] = os.path.join(out_dir, "base-mem.symbolic-globals")
        subprocess.run(f"./val.sh", cwd=subject_dir, shell=True, env=env)
    elif args.cmd == "build":
        subprocess.run(f"./init.sh", cwd=subject_dir, shell=True)
    elif args.cmd == "extractfix-build":
        subprocess.run(f"./extractfix.sh", cwd=subject_dir, shell=True)
    elif args.cmd == "vulmaster-build":
        subprocess.run(f"./init-vulmaster.sh", cwd=subject_dir, shell=True)
    elif args.cmd == "vulmaster-extractfix-build":
        subprocess.run(f"./extractfix-vulmaster.sh", cwd=subject_dir, shell=True)
    elif args.cmd == "collect-inputs":
        out_no = find_num(os.path.join(subject_dir, "runtime"), "aflrun-out")
        collect_val_runtime(subject_dir, os.path.join(subject_dir, "runtime", f"aflrun-out-{out_no - 1}"))
    elif args.cmd == "group-patches":
        # Group patches
        group_patches(subject_dir)
    elif args.cmd == "val":
        run_val(subject, subject_dir, args.symvass_prefix, val_prefix, args.debug)
    elif args.cmd == "feas":
        val_dir = os.path.join(subject_dir, "val-runtime")
        no = find_num(val_dir, val_prefix)
        val_out_dir = os.path.join(val_dir, f"{val_prefix}-{no - 1}")
        parse_val_results(val_out_dir)
    elif args.cmd == "analyze":
        val_dir = os.path.join(subject_dir, "val-runtime")
        no = find_num(val_dir, val_prefix)
        val_out_dir = os.path.join(val_dir, f"{val_prefix}-{no - 1}")
        analyze(subject, val_out_dir, args.output)

if __name__ == "__main__":
    main()