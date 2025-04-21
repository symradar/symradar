#!/usr/bin/env python3
import os
import sys
import argparse
from typing import List, Tuple, Set, Dict, TextIO
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
import psutil

VULMASTER_MODE = False
VULMASTER_ID = 0
EXTRACTFIX_MODE = False

def print_log(msg: str):
    print(msg, file=sys.stderr)

def print_out(msg: str):
    print(msg, file=sys.stdout)

def get_trace(dir: str, id: int):
    # dp = SymRadarDataLogSbsvParser(dir)
    # dp.read_data_log("data.log")
    # result = dp.generate_table_v2(dp.cluster())
    # graph = result["graph"]
    state_id = id
    parent_states = set() # dp.get_parent_states(graph, state_id)
    state_filter = set(parent_states)
    state_filter.add(state_id)
    done = set()
    prev = -1
    trace = list()
    pattern = re.compile(r"\[state (\d+)\]")
    with open(os.path.join(dir, "trace.log"), "r") as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("[state") and "[B]" not in line:
                matched = pattern.search(line)
                if matched is None:
                    continue
                state = int(matched.group(1))
                if state in state_filter:
                    if state in done:
                        continue
                    if prev != state:
                        if prev > state:
                            continue
                        trace.append(f"[state {prev}] -> [state {state}]")
                        done.add(prev)
                        prev = state
                    trace.append(line.strip().replace("/root/projects/CPR/patches/extractfix/libtiff/bugzilla-2633/src/", ""))
    with open(os.path.join(dir, f"trace-{id}.log"), "w") as f:
        for line in trace:
            f.write(line + "\n")

def convert_to_sbsv(dir: str):
    with open(f"{dir}/run.stats", "r") as f:
        lines = f.readlines()
        header = lines[0].strip().replace("(", "").replace(")", "").split(",")
        with open(f"{dir}/run.stats.sbsv", "w") as f:
            for i in range(1, len(lines)):
                line = lines[i].strip().replace("(", "").replace(")", "")
                tokens = line.split(",")
                f.write(
                    f"[stats] [inst {tokens[0]}] [fb {tokens[1]}] [pb {tokens[2]}] [nb {tokens[3]}] [ut {tokens[4]}] [ns {tokens[5]}] [mu {tokens[6]}] [nq {tokens[7]}] [nqc {tokens[8]}] [no {tokens[9]}] [wt {tokens[10]}] [ci {tokens[11]}] [ui {tokens[12]}] [qt {tokens[13]}] [st {tokens[14]}] [cct {tokens[15]}] [ft {tokens[16]}] [rt {tokens[17]}] [qccm {tokens[18]}] [ccch {tokens[19]}]\n"
                )


class ConfigFiles(uni_klee.ConfigFiles):
    def __init__(self):
        self.root_dir = uni_klee.ROOT_DIR
        self.global_config = uni_klee.global_config

    def set(self, bug_info: dict):
        patches_dir = os.path.join(self.root_dir, "patches")
        self.bid = bug_info["bug_id"]
        self.benchmark = bug_info["benchmark"]
        self.subject = bug_info["subject"]
        self.project_dir = os.path.join(
            patches_dir, self.benchmark, self.subject, self.bid
        )
        if VULMASTER_MODE:
            self.work_dir = os.path.join(self.project_dir, "vulmaster-patched")
            self.meta_patch_obj_file = os.path.join(
                self.project_dir, "concrete", "libuni_klee_runtime_vulmaster.bca"
            )
        else:
            self.work_dir = os.path.join(self.project_dir, "patched")
            self.meta_patch_obj_file = os.path.join(
                self.project_dir, "concrete", "libuni_klee_runtime_new.bca"
            )
        self.repair_conf = os.path.join(self.project_dir, "repair.conf")
        self.meta_program = os.path.join(self.project_dir, "meta-program-original.json")
        sympatch.compile(os.path.join(self.project_dir, "concrete"))
        

    def set_out_dir(self, out_dir: str, out_dir_prefix: str, bug_info: dict, snapshot_prefix: str, filter_prefix: str, use_last: bool):
        self.out_dir_prefix = out_dir_prefix
        self.snapshot_prefix = snapshot_prefix
        self.filter_prefix = filter_prefix
        if out_dir == "":
            self.out_base_dir = self.work_dir
        elif out_dir == "out":
            self.out_base_dir = os.path.join(self.root_dir, "out", self.benchmark, self.subject, self.bid)
        else:
            self.out_base_dir = out_dir
        os.makedirs(self.out_base_dir, exist_ok=True)
        no = self.find_num(self.out_base_dir, out_dir_prefix)
        if use_last:
            self.out_dir = os.path.join(self.out_base_dir, f"{out_dir_prefix}-{no-1}")
        else:
            self.out_dir = os.path.join(self.out_base_dir, f"{out_dir_prefix}-{no}")
        self.snapshot_dir = os.path.join(self.out_base_dir, self.snapshot_prefix)
        if VULMASTER_MODE:
            self.filter_dir = os.path.join(self.out_base_dir, f"{filter_prefix}_{VULMASTER_ID}")
        else:
            self.filter_dir = os.path.join(self.out_base_dir, filter_prefix)
        # print_log(f"Use snapshot {self.bid} snapshot-last.json ...")
        self.snapshot_file = os.path.join(self.snapshot_dir, "snapshot-last.json")

class Config(uni_klee.Config):
    conf_files: ConfigFiles
    mode: str

    def __init__(
        self, cmd: str, query: str, debug: bool, sym_level: str, max_fork: str, mode: str
    ):
        self.cmd = cmd
        self.query = query
        self.debug = debug
        self.sym_level = sym_level
        self.max_fork = max_fork
        self.mode = mode
        self.conf_files = ConfigFiles()

    def append_snapshot_cmd(self, cmd: List[str]):
        snapshot_dir = self.conf_files.snapshot_dir
        patch_str = ",".join(self.snapshot_patch_ids)
        if self.cmd == "filter":
            cmd.append("--no-exit-on-error")
            snapshot_dir = self.conf_files.filter_dir
            all_patches = [str(patch["id"]) for patch in self.meta_program["patches"]]
            patch_str = ",".join(all_patches)
            if VULMASTER_MODE:
                cmd.append(f"--patch-filtering")
        cmd.append(f"--output-dir={snapshot_dir}")
        cmd.append(f"--patch-id={patch_str}")
    
    def find_last_loc(self, dir: str, target_function: str) -> int:
        # parse cfg.sbsv to get instruction range of target function
        crash_loc = 0
        cfg_parser = sbsv.parser()
        cfg_parser.add_schema("[fn-start] [name: str]")
        cfg_parser.add_schema("[fn-end] [name: str]")
        cfg_parser.add_schema("[bb] [id: int] [start: int] [size: int] [line: int]")
        cfg_parser.add_group("fn", "[fn-start]", "[fn-end]")
        with open(os.path.join(dir, "cfg.sbsv"), "r") as f:
            cfg_parser.load(f)
        idx = cfg_parser.get_group_index("fn")
        start_loc = -1
        end_loc = 0
        for i in idx:
            fn = cfg_parser.get_result_by_index("[fn-start]", i)
            if fn[0]["name"] == target_function:
                print_log(f"[info] [find function {target_function}]")
                bbs = cfg_parser.get_result_by_index("[bb]", i)
                for bb in bbs:
                    start = bb["start"]
                    end = start + bb["size"] - 1
                    if end > end_loc:
                        end_loc = end
                    if start < start_loc or start_loc == -1:
                        start_loc = start
        # Read data from snapshot
        dp = SymRadarDataLogSbsvParser(dir, schema=["[stack-trace] [state: int] [reg?: str] [passed-crash-loc: bool]"])
        stack_trace_str = dp.data["stack-trace"][0]["reg"]
        stack_trace = stack_trace_str.split(",")
        for stack in stack_trace:
            tokens = stack.strip().split(":")
            if len(tokens) == 4:
                loc = int(tokens[3])
                if crash_loc == 0:
                    crash_loc = loc
                else:
                    if loc >= start_loc and loc <= end_loc:
                        crash_loc = loc
        print_log(f"crash_loc: {crash_loc}")
        return crash_loc

    def append_cmd(self, cmd: List[str], patch_str: str, opts: List[str]):
        out_dir = self.conf_files.out_dir
        default_opts = [
            "--no-exit-on-error",
            "--simplify-sym-indices",
            f"--symbolize-level={self.sym_level}",
            f"--max-forks-per-phases={self.max_fork}",
        ]
        if self.cmd == "uc":
            cmd.append("--no-exit-on-error")
            cmd.append(f"--output-dir={out_dir}")
            cmd.append(f"--patch-id={patch_str}")
            cmd.append(f"--max-forks-per-phases={self.max_fork}")
            cmd.append("--no-snapshot")
            return
        if EXTRACTFIX_MODE:
            exit_loc = self.find_last_loc(self.conf_files.snapshot_dir, self.bug_info["target"])
            cmd.append("--use-extractfix")
            cmd.append(f"--crash-loc={exit_loc}")
        cmd.extend(default_opts)
        cmd.extend(opts)
        cmd.append(f"--output-dir={out_dir}")
        cmd.append(f"--patch-id={patch_str}")
        cmd.append(f"--snapshot={self.conf_files.snapshot_file}")

    def get_cmd_opts(self, is_snapshot: bool) -> str:
        target_function = self.bug_info["target"]
        link_opt = f"--link-llvm-lib={self.conf_files.meta_patch_obj_file}"
        result = [
            "uni-klee",
            "--libc=uclibc",
            "--posix-runtime",
            "--external-calls=all",
            "--allocate-determ",
            "--write-smt2s",
            "--write-kqueries",
            "--log-trace",
            "--max-memory=0",
            "--lazy-patch",
            "--max-solver-time=10s",
            f"--target-function={target_function}",
            link_opt,
        ]
        if "klee_flags" in self.project_conf:
            link_opt = self.project_conf["klee_flags"]
            result.append(link_opt)
        if self.timeout != "":
            result.append(f"--max-time={self.timeout}")
        if self.additional != "":
            result.extend(self.additional.split(" "))
        if is_snapshot:
            self.append_snapshot_cmd(result)
        else:
            add_opts = list()
            add_opts.append("--start-from-snapshot")
            self.append_cmd(result, ",".join(self.patch_ids), add_opts)
        bin_file = os.path.basename(self.project_conf["binary_path"])
        if VULMASTER_MODE:
            target = f"{bin_file}-{VULMASTER_ID}.bc"
        else:
            target = bin_file + ".bc"
        result.append(target)
        if "test_input_list" in self.project_conf:
            poc_path = "exploit"
            if "poc_path" in self.project_conf:
                poc_path = self.project_conf["poc_path"]
            target_cmd = self.project_conf["test_input_list"].replace("$POC", poc_path)
            result.append(target_cmd)
        return " ".join(result)
    
    def get_cmd_for_original(self, poc: str) -> str:
        if "test_input_list" in self.project_conf:
            target_cmd = self.project_conf["test_input_list"].replace("$POC", poc)
            return target_cmd
        return ""

class SymRadarDataLogSbsvParser():
    dir: str
    parser: sbsv.parser
    data: Dict[str, List[dict]]

    def __init__(self, dir: str, name: str = "data.log", schema: List[str] = []):
        self.dir = dir
        self.parser = sbsv.parser()
        self.set_schema(self.parser, schema)
        with open(os.path.join(dir, name), "r") as f:
            self.data = self.parser.load(f)

    def set_schema(self, parser: sbsv.parser, schema: List[str]):
        parser.add_schema("[meta-data] [state: int] [crashId: int] [patchId: int] [stateType: str] [isCrash: bool] [actuallyCrashed: bool] [use: bool] [exitLoc: str] [exit: str]")
        parser.add_schema("[fork] [state$from: int] [state$to: int]")
        parser.add_schema("[fork-map] [fork] [state$from: int] [type$from: str] [base: int] [base-type: str] [state$to: int] [type$to: str] [fork-count: str]")
        parser.add_schema("[fork-map] [sel-patch] [state$base: int] [state$base_after: int]")
        parser.add_schema("[fork-map] [fork-parent] [state: int] [type: str]")
        parser.add_schema("[fork-map] [merge] [state$base: int] [state$crash_test: int] [patch: int]")
        parser.add_schema("[fork-loc] [br] [state$from: int] [loc$from: str] -> [state$to_a: int] [loc$to_a: str] [state$to_b: int] [loc$to_b: str]")
        parser.add_schema("[fork-loc] [sw] [state$from: int] [loc$from: str] -> [state$to_a: int] [loc$to_a: str] [state$to_b: int] [loc$to_b: str]")
        parser.add_schema("[fork-loc] [lazy] [state$from: int] [state$to: int] [loc: str] [name?: str]")
        parser.add_schema("[patch-base] [trace] [state: int] [res: bool] [iter: int] [patches: str]")
        parser.add_schema("[patch-base] [fork] [state$true: int] [state$false: int] [iter: int]")
        parser.add_schema("[regression-trace] [state: int] [n: int] [res: bool] [loc: str]")
        parser.add_schema("[patch] [trace] [state: int] [iter: int] [res: bool] [patches: str]")
        parser.add_schema("[patch] [trace-rand] [state: int] [iter: int] [res: bool] [patches: str]")
        parser.add_schema("[patch] [fork] [state$true: int] [state$false: int] [iter: int] [patches: str]")
        parser.add_schema("[regression] [state: int] [reg?: str]")
        parser.add_schema("[lazy-trace] [state: int] [reg?: str] [patches?: str] [patch-eval?: str]")
        parser.add_schema("[stack-trace] [state: int] [reg?: str] [passed-crash-loc: bool]")
        for s in schema:
            parser.add_schema(s)
    def get_data(self) -> Dict[str, List[dict]]:
        return self.data


class DataAnalyzer():
    dp: SymRadarDataLogSbsvParser
    data: Dict[str, List[dict]]
    meta_data: Dict[int, dict]
    graph: nx.DiGraph
    symbolic_inputs: Dict[int, List[int]]
    
    def __init__(self, dp: SymRadarDataLogSbsvParser):
        self.dp = dp
        self.data = self.dp.get_data()
        self.meta_data = dict()
        self.graph = nx.DiGraph()
        self.symbolic_inputs = dict()
        
    def analyze(self):
        self.construct_graph()
        self.draw_graph()

    def to_list(self, s: str) -> List[int]:
        ls = s.strip("[]").strip(", ")
        ls = ls.split(", ")
        result = list()
        for x in ls:
            try:
                if len(x) > 0:
                    result.append(int(x))
            except:
                pass
        return result
    
    def to_map(self, s: str) -> Dict[int, str]:
        ls = s.strip("[]").strip(", ")
        ls = ls.split(", ")
        result = dict()
        for x in ls:
            try:
                if len(x) > 0:
                    key, value = x.split(":")
                    result[int(key)] = value
            except:
                pass
        return result

    def construct_graph(self):
        meta_data = self.meta_data
        graph = self.graph
        for meta in self.data["meta-data"]:
            state = meta["state"]
            state_type = meta["stateType"]
            exit_type = meta["exit"]
            use = meta["use"]
            graph.add_node(state, fork_parent=False)
            meta_data[state] = meta.data
            meta_data[state]["sel-patch"] = False
            meta_data[state]["input"] = False
            meta_data[state]["patches"] = None
            meta_data[state]["patch-eval"] = dict()
            meta_data[state]["reg"] = list()
            if state_type == "2": # Add base states, even if not tested
                if state not in self.symbolic_inputs:
                    self.symbolic_inputs[state] = list()
        for lt in self.data["lazy-trace"]:
            state = lt["state"]
            if state not in meta_data:
                continue
            reg = lt["reg"]
            patches = lt["patches"]
            patch_eval = lt["patch-eval"]
            if patches is None:
                patches = list()
            else:
                patches = self.to_list(patches)
            if reg is None:
                reg = list()
            else:
                reg = self.to_list(reg)
            meta_data[state]["patches"] = patches
            meta_data[state]["reg"] = reg
            meta_data[state]["patch-eval"] = self.to_map(patch_eval)
        
        for stack in self.data["stack-trace"]:
            state = stack["state"]
            if state not in meta_data:
                continue
            meta_data[state]["stack-trace"] = stack

        for fp in self.data["fork-map"]["fork-parent"]:
            state = fp["state"]
            state_type = fp["type"]
            graph.add_node(state, fork_parent=True)
        # Add edges
        for fork in self.data["fork-map"]["fork"]:
            fork_from = fork["state$from"]
            type_from = fork["type$from"]
            fork_to = fork["state$to"]
            type_to = fork["type$to"]
            graph.add_edge(fork_from, fork_to, type="fork")
        for sel in self.data["fork-map"]["sel-patch"]:
            base = sel["state$base"]
            base_after = sel["state$base_after"]
            if base not in meta_data:
                if base_after not in meta_data:
                    continue
                meta_data[base] = meta_data[base_after].copy()
                meta_data[base]["state"] = base
                meta_data[base]["stateType"] = "1"
                meta_data[base]["sel-patch"] = True
            graph.add_edge(base, base_after, type="sel-patch")
        for merge in self.data["fork-map"]["merge"]:
            base = merge["state$base"]
            crash_test = merge["state$crash_test"]
            patch = merge["patch"]
            graph.add_edge(base, crash_test, type="merge", patch=patch)
            if base not in meta_data:
                continue
            meta_data[base]["input"] = True
            if base not in self.symbolic_inputs:
                self.symbolic_inputs[base] = list()
            self.symbolic_inputs[base].append(crash_test)

    def draw_graph(self):
        if len(self.graph.nodes()) > 1024:
            return
        dot = graphviz.Digraph()
        for state in self.graph.nodes():
            fillcolor = "white"
            color = "black"
            if state in self.meta_data:
                meta = self.meta_data[state]
                if meta["actuallyCrashed"]:
                    color = "red"
                else:
                    color = "green"
                if "early" in meta["exit"]:
                    color = "grey"
                    fillcolor = "grey"
                else:
                    if meta["sel-patch"]:
                        fillcolor = "pink"
                    elif meta["input"]:
                        fillcolor = "blue"
                    elif meta["stateType"] == "1":
                        fillcolor = "red"
                    elif meta["stateType"] == "2":
                        fillcolor = "skyblue"
                    elif meta["stateType"] == "4":
                        fillcolor = "yellow"
            shape = "ellipse"
            if "fork_parent" in self.graph.nodes[state]:
                if self.graph.nodes[state]["fork_parent"]:
                    shape = "box"
            dot.node(str(state), shape=shape, style="filled", color=color, fillcolor=fillcolor)
        
        for edge in self.graph.edges():
            src = edge[0]
            dst = edge[1]
            style = "solid"
            color = "black"
            if "type" in self.graph[src][dst]:
                if self.graph[src][dst]["type"] == "merge":
                    style = "dashed"
                    color = "red"
                if self.graph[src][dst]["type"] == "sel-patch":
                    style = "dotted"
                    color = "blue"
            dot.edge(str(src), str(dst), style=style, color=color)
        
        dot.render("fork-graph", self.dp.dir, format="png")
        dot.render("fork-graph", self.dp.dir, format="pdf")
    
    def count_states(self, patches: Set[int]) -> Tuple[int, int]:
        count = len(self.meta_data)
        original_count = 0
        type_count = {"1": 0, "2": 0, "3": 0, "4": 0}
        base_states = dict()
        multi_patch_count = 0
        sym_inputs = list()
        sym_reruns = list()
        for state in self.meta_data:
            meta = self.meta_data[state]
            if meta["stateType"] == "1":
                type_count["1"] += 1
            elif meta["stateType"] == "2":
                type_count["2"] += 1
                sym_inputs.append(state)
            elif meta["stateType"] == "3":
                type_count["3"] += 1
            elif meta["stateType"] == "4":
                type_count["4"] += 1
                sym_reruns.append(state)
        original_count = type_count["1"] + type_count["2"]
        multi_patch_count = type_count["1"] + type_count["2"]
        
        for state in sym_reruns:
            if state not in self.meta_data:
                continue
            meta = self.meta_data[state]
            if meta["patches"] is None:
                continue
            patch_set = set(meta["patches"])
            has_valid = False
            for patch in patches:
                if patch in patch_set:
                    has_valid = True
                    multi_patch_count += 1
            if has_valid:
                original_count += 1
        
        for edge in self.graph.edges():
            src = edge[0]
            dst = edge[1]
            type = self.graph[src][dst]["type"]
            if type == "fork":
                pass
            elif type == "sel-patch":
                # In independent patch mode, sel-patch will actually fork states per patches
                multi_patch_count += len(patches)
                original_count += 1
            elif type == "merge":
                # Get patch num
                pass
            else:
                print_log(f"[error] [unknown edge type {type}]")
        print_log(f"[info] [original-count {original_count}] [independent-count {multi_patch_count}]")
        return original_count, multi_patch_count


class PatchSorter:
    # dir: str
    # patch_ids: list
    # meta_data: dict
    dp: DataAnalyzer
    dp_filter: DataAnalyzer
    cluster: Dict[int, Dict[int, List[Tuple[int, bool]]]]
    patch_ids: List[int]
    patch_filter: Set[int]

    def __init__(
        self, dp: DataAnalyzer, dp_filter: DataAnalyzer = None
    ):
        # self.dir = dir
        # self.patch_ids = patch_ids
        # self.meta_data = config.bug_info
        self.dp = dp
        self.dp_filter = dp_filter
        self.cluster = dict()
        self.patch_ids = list()

    def check_correctness(self, base: dict, patch: dict) -> bool:
        if base["isCrash"]:
            return not patch["actuallyCrashed"]
        base_trace = base["lazyTrace"].split() if "lazyTrace" in base else []
        patch_trace = patch["lazyTrace"].split() if "lazyTrace" in patch else []
        return base_trace == patch_trace

    def filter_patch(self) -> Dict[int, bool]:
        self.patch_filter = dict()
        result = dict()
        for state_id, data in self.dp_filter.meta_data.items():
            patch_id = data["patchId"]
            if data["stateType"] == "3":
                self.patch_filter[patch_id] = data
                result[patch_id] = not data["actuallyCrashed"]
        # original = self.patch_filter[0]
        # if not original["actuallyCrashed"]:
        #     print_log("Original patch does not crash")
        return result

    def analyze_cluster(self) -> Dict[int, Dict[int, List[Tuple[int, bool]]]]:
        self.cluster: Dict[int, Dict[int, List[Tuple[int, bool]]]] = dict()
        self.cluster[0] = dict()
        filter_result = self.filter_patch()
        patch_set = set()
        for patch_id, result in filter_result.items():
            if result:
                patch_set.add(patch_id)
            self.cluster[0][patch_id] = [(-1, result)]
            print_log(f"Patch {patch_id} crashed: {result}")
        temp_patches = set()
        for crash_id, data_list in self.dp.cluster().items():
            if crash_id == 0:
                continue
            self.cluster[crash_id] = dict()
            base = data_list[0]
            for data in data_list:
                if data["stateType"] == "2":
                    base = data
                    break
            for data in data_list:
                if data["stateType"] == "2":
                    continue
                patch = data["patchId"]
                temp_patches.add(patch)
                id = data["state"]
                check = self.check_correctness(base, data)
                if patch not in self.cluster[crash_id]:
                    self.cluster[crash_id][patch] = list()
                self.cluster[crash_id][patch].append((id, check))
        if len(self.patch_ids) == 0:
            self.patch_ids = list(temp_patches)
            self.patch_ids.sort()
        return self.cluster

    def get_score(self, patch: int) -> float:
        score = 0.0
        for crash_id, patch_map in self.cluster.items():
            patch_score = 0.0
            if patch not in patch_map:
                print_log(f"Patch {patch} not found in crash {crash_id}")
                continue
            for patch_result in patch_map[patch]:
                if patch_result[1]:
                    patch_score += 1.0
            score += patch_score / len(patch_map[patch])
        return score

    def sort(self) -> List[Tuple[int, float]]:
        scores: List[Tuple[int, float]] = list()
        for patch in self.patch_ids:
            score = self.get_score(patch)
            scores.append((patch, score))
        scores.sort(key=lambda x: x[1], reverse=True)
        return scores


class SymRadarAnalyzer:
    dir: str
    filter_dir: str
    bug_info: dict

    def __init__(self, dir: str, filter_dir: str, bug_info: dict):
        self.dir = dir
        self.filter_dir = filter_dir
        self.bug_info = bug_info

    def save_sorted_patches(
        self,
        dp: DataAnalyzer,
        sorted_patches: List[Tuple[int, float]],
        cluster: Dict[int, Dict[int, List[Tuple[int, bool]]]],
    ):
        patch_list = sorted([patch for patch, score in sorted_patches])
        with open(os.path.join(self.dir, "patch-rank.md"), "w") as f:
            f.write("## Patch Rank\n")
            f.write(f"| Rank | Patch | Score |\n")
            f.write(f"|------|-------|-------|\n")
            rank = 1
            patch_filter = set()
            for patch_id, result in cluster[0].items():
                if result[0][1]:
                    patch_filter.add(patch_id)
            for patch, score in sorted_patches:
                if patch in patch_filter:
                    f.write(f"| {rank} | {patch} | {score:.2f} |\n")
                    rank += 1
            f.write(f"\n## Removed patch list\n")
            f.write(f"| Rank | Patch | Score |\n")
            f.write(f"|------|-------|-------|\n")
            rank = 1
            for patch, score in sorted_patches:
                if patch not in patch_filter:
                    f.write(f"| {rank} | {patch} | {score:.2f} |\n")
                    rank += 1
            f.write("\n## Patch Result\n")
            cluster_list = list(cluster.items())
            cluster_list.sort(key=lambda x: x[0])
            for crash_id, patch_map in cluster_list:
                f.write(f"### Input {crash_id}\n")
                f.write(f"| Input id | Patch id | state id | Correctness | crashed |\n")
                f.write(f"|----------|----------|----------|-------------|---------|\n")
                for patch in patch_list:
                    result = patch_map[patch]
                    local_input_id = 0
                    for patch_result in result:
                        state = patch_result[0]
                        if state < 0:
                            f.write(
                                f"| {patch}-{local_input_id} | {patch} | {state} | {'O' if patch_result[1] else 'X'} | - |\n"
                            )
                        else:
                            actually_crashed = dp.meta_data[state]["actuallyCrashed"]
                            f.write(
                                f"| {patch}-{local_input_id} | {patch} | {state} | {'O' if patch_result[1] else 'X'} | {actually_crashed} |\n"
                            )
                        local_input_id += 1
        print_log(f"Saved to {os.path.join(self.dir, 'patch-rank.md')}")
        
    def cluster(self, analyzer: DataAnalyzer) -> Dict[int, Set[int]]:
        cluster: Dict[int, Set[int]] = dict()
        for crash_id in analyzer.symbolic_inputs:
            cluster[crash_id] = set()
            for crash_test in analyzer.symbolic_inputs[crash_id]:
                successors = set(nx.dfs_preorder_nodes(analyzer.graph, crash_test))
                cluster[crash_id].update(successors)
        return cluster
    
    def get_predecessors(self, analyzer: DataAnalyzer, state: int):
        trace = list()
        state_type = set()
        if state not in analyzer.meta_data:
            return []
        stop_type = ""
        if analyzer.meta_data[state]["stateType"] == "4":
            stop_type = "2"
        stack = [state]
        while stack:
            node = stack.pop()
            preds = analyzer.graph.predecessors(node)
            stack += preds
            if len(list(preds)) > 1:
                print_log(f"[warn] [state {node}] [preds {preds}]")
            for pred in preds:
                if pred in analyzer.meta_data and analyzer.meta_data[pred]["stateType"] in state_type:
                    trace.append(pred)
        return trace
    
    def symbolic_trace(self, analyzer: DataAnalyzer) -> Dict[int, List[Tuple[bool, int, List[Tuple[int, str]]]]]:
        patch_data = analyzer.dp.parser.get_result_in_order(["patch$trace", "patch$trace-rand", "patch$fork"])
        trace_original = dict()
        for patch in reversed(patch_data):
            patches_str = patch["patches"]
            patches_str = patches_str.strip("[]").strip(", ")
            pairs = patches_str.split(", ")
            patches_result = list()
            for pair in pairs:
                key, value = pair.split(":")
                patches_result.append((int(key), value))
            iter = patch["iter"]
            if patch.get_name() == "patch$fork":
                true_state = patch["state$true"]
                false_state = patch["state$false"]
                if true_state not in trace_original:
                    trace_original[true_state] = list()
                if false_state not in trace_original:
                    trace_original[false_state] = list()
                trace_original[true_state].append((True, iter, patches_result))
                trace_original[false_state].append((False, iter, patches_result))
            else:
                res = patch["res"]
                state = patch["state"]
                if state not in trace_original:
                    trace_original[state] = list()
                trace_original[state].append((res, iter, patches_result))
        trace = dict()
        for state in trace_original:
            trace[state] = list()
            check_1 = False
            for res, iter, patches in trace_original[state]:
                if iter == 1:
                    check_1 = True
                    break
            if check_1:
                trace[state] = trace_original[state]
            else:
                pred = self.get_predecessors(analyzer, state)
                cur_iter = -1
                for p in pred:
                    if p not in trace_original:
                        continue
                    for res, iter, patches in trace_original[p]:
                        if cur_iter > 0 and iter >= cur_iter:
                            continue
                        if cur_iter > 0 and cur_iter != iter + 1:
                            print_log(f"[warn] [error] [state {state}] [iter {cur_iter-1}] [actual {iter}]")
                        cur_iter = iter
                        trace[state].append((res, iter, patches))
            # print_log(f"[state {state}] {trace[state]}")
        return trace
    
    def buggy_trace(self, analyzer: DataAnalyzer):
        trace_original = dict()
        bt = analyzer.dp.parser.get_result_in_order(["patch-base$trace", "patch-base$fork"])
        for patch_base in reversed(bt):
            iter = patch_base["iter"]
            if patch_base.get_name() == "patch-base$trace":
                state = patch_base["state"]
                res = patch_base["res"]
                if state not in trace_original:
                    trace_original[state] = list()
                trace_original[state].append((res, iter))
            else:
                true_state = patch_base["state$true"]
                false_state = patch_base["state$false"]
                if true_state not in trace_original:
                    trace_original[true_state] = list()
                if false_state not in trace_original:
                    trace_original[false_state] = list()
                trace_original[true_state].append((True, iter))
                trace_original[false_state].append((False, iter))
        trace = dict()
        for state in trace_original:
            trace[state] = list()
            check_1 = False
            for res, iter in trace_original[state]:
                if iter == 1:
                    check_1 = True
                    break
            if check_1:
                trace[state] = trace_original[state]
            else:
                pred = self.get_predecessors(analyzer, state)
                cur_iter = -1
                for p in pred:
                    if p not in trace_original:
                        continue
                    for res, iter in trace_original[p]:
                        if cur_iter > 0 and iter >= cur_iter:
                            continue
                        if cur_iter > 0 and cur_iter != iter + 1:
                            print_log(f"[warn] [error] [state {state}] [iter {cur_iter-1}] [actual {iter}]")
                        cur_iter = iter
                        trace[state].append((res, iter))
        print_log(trace)
        return trace

    def get_patch(self, state: int, st: List[Tuple[bool, int, List[Tuple[int, str]]]]) -> List[int]:
        trace = dict()
        patches = set()
        for s in st:
            trace[s[1]] = s[0]
            for p in s[2]:
                if s[0] and p[1] != "0":
                    patches.add(p[0])
                elif not s[0] and p[1] != "1":
                    patches.add(p[0])
        for s in st:
            for p in s[2]:
                if s[0] and p[1] == "0":
                    patches.remove(p[0])
                elif not s[0] and p[1] == "1":
                    patches.remove(p[0])
        l = list()
        for i in range(len(trace)):
            l.append(trace[i+1])
        print_log(f"[state {state}] [trace {l}] [patches {patches}]")
        return patches
    
    def generate_table(self, cluster: Dict[int, Set[int]], result: List[Tuple[int, int, int, List[int]]]) -> str:
        with open(os.path.join(self.dir, "table.sbsv"), "w") as f:
            all_patches = set()
            for res in result:
                crash_id, base, test, patches = res
                for patch in patches:
                    all_patches.add(patch)
                f.write(f"[sym-in] [id {crash_id}] [base {base}] [test {test}] [cnt {len(patches)}] [patches {patches}]\n")
            # Current result: assume all are feasible
            current_result = all_patches.copy()
            for res in result:
                crash_id, base, test, patches = res
                res_patches = set(patches)
                for patch in all_patches:
                    if patch not in res_patches and patch in current_result:
                        current_result.remove(patch)
            current_result_list = sorted(list(current_result))
            f.write(f"[sym-out] [default] [cnt {len(current_result_list)}] [patches {current_result_list}]\n")
            # Best result: use symbolic inputs those do not filter out correct patches
            if self.bug_info is not None:
                correct_patch = self.bug_info["correct"]["no"]
                best_result = all_patches.copy()
                correct_input_num = 0
                for res in result:
                    crash_id, base, test, patches = res
                    res_patches = set(patches)
                    if correct_patch not in res_patches:
                        continue
                    correct_input_num += 1
                    for patch in all_patches:
                        if patch not in res_patches and patch in best_result:
                            best_result.remove(patch)
                best_result_list = sorted(list(best_result))
                f.write(f"[sym-out] [best] [cnt {len(best_result_list)}] [patches {best_result_list}]\n")
                meta_data_info = uni_klee.global_config.get_meta_data_info_by_id(self.bug_info["id"])
                f.write(f"[meta-data] [correct {correct_patch}] [all-patches {len(meta_data_info['meta_program']['patches'])}] [sym-input {len(result)}] [correct-input {correct_input_num}]\n")

        all_patches = len(meta_data_info['meta_program']['patches'])
        sym_input_id = 0
        with open(os.path.join(self.dir, "table.md"), "w") as md:
            md.write("# SymRadar Result\n")
            md.write(f"| symbolic input | ")
            for p in range(all_patches):
                md.write(f"p{p} | ")
            md.write("\n")
            md.write(f"| --- | ")
            for p in range(all_patches):
              md.write("--- | ")
            md.write("\n")
            for res in result:
                crash_id, base, test, patches = res
                md.write(f"| i{sym_input_id} | ")
                sym_input_id += 1
                for p in range(all_patches):
                    if p in patches:
                        md.write(f"O | ")
                    else:
                        md.write("X | ")
                md.write("\n")
            md.write("\n")
    
    def analyze_v2(self):
        dp = SymRadarDataLogSbsvParser(self.dir)
        analyzer = DataAnalyzer(dp)
        analyzer.analyze()
        cluster = self.cluster(analyzer)
        # symbolic_trace = self.symbolic_trace(analyzer)
        # buggy_trace = self.buggy_trace(analyzer)
        result = list()
        for crash_state in cluster:
            base_meta = analyzer.meta_data[crash_state]
            if not base_meta["use"]:
                continue
            crash_id = base_meta["crashId"]
            base = base_meta["patches"]
            base_reg = base_meta["reg"]
            is_crash = base_meta["isCrash"]
            if not is_crash:
                result.append((crash_id, base_meta["state"], base_meta["state"], base))
            for crash_test in cluster[crash_state]:
                if crash_test not in analyzer.meta_data:
                    continue
                crash_meta = analyzer.meta_data[crash_test]
                if not crash_meta["use"]:
                    continue
                crash = crash_meta["patches"]
                crash_reg = crash_meta["reg"]
                crashed = crash_meta["actuallyCrashed"]
                # If input is feasible:
                # crash -> not crash
                # not crash -> not crash + preserve behavior
                if is_crash:
                    if not crashed:
                        no_reg = True
                        for i in range(len(base_reg) - 1):
                            if len(crash_reg) <= i:
                                no_reg = False
                                break
                            if base_reg[i] != crash_reg[i]:
                                no_reg = False
                                break # Regression error
                        if no_reg:
                            result.append((crash_id, base_meta["state"], crash_meta["state"], crash))
                else:
                    if not crashed and base_reg == crash_reg:
                        result.append((crash_id, base_meta["state"], crash_meta["state"], crash))
        self.generate_table(cluster, result)
    
    def analyze_v3(self):
        if not os.path.exists(os.path.join(self.filter_dir, "filtered.json")):
            print_log(f"[error] {os.path.join(self.filter_dir, 'filtered.json')} not found")
            exit(1)
        dp_filter = SymRadarDataLogSbsvParser(self.filter_dir)
        with open(os.path.join(self.filter_dir, "filtered.json"), "r") as f:
            filtered = json.load(f)
        subject_dir = os.path.join(uni_klee.ROOT_DIR, "patches", self.bug_info["benchmark"], self.bug_info["subject"], self.bug_info["bug_id"])
        with open(os.path.join(subject_dir, "group-patches-original.json"), "r") as f:
            group_patches = json.load(f)
        patch_group_tmp = dict()
        correct_patch = group_patches["correct_patch_id"]
        for patches in group_patches["equivalences"]:
            representative = patches[0]
            for patch in patches:
                patch_group_tmp[patch] = representative
        patch_eq_map = dict()
        for patch in filtered["remaining"]:
            if patch in patch_group_tmp:
                patch_eq_map[patch] = patch_group_tmp[patch]
            else:
                patch_eq_map[patch] = patch
        all_patches = set()
        for patch in filtered["remaining"]:
            if patch == patch_eq_map[patch]:
                all_patches.add(patch)
        if correct_patch in patch_eq_map:
            correct_patch = patch_eq_map[correct_patch]
        
        if VULMASTER_MODE:
            all_patches = set(filtered["remaining"])
            correct_patch = 1 # This is mostly wrong, but we need any correct patch
        # Get exit location in filter
        filter_metadata = dp_filter.parser.get_result()["meta-data"][0]
        exit_loc = filter_metadata["exitLoc"]
        exit_res = filter_metadata["exit"]
        # Analyze
        dp = SymRadarDataLogSbsvParser(self.dir)
        analyzer = DataAnalyzer(dp)
        analyzer.analyze()
        cluster = self.cluster(analyzer)
        result = list()
        for crash_state in cluster:
            base_meta = analyzer.meta_data[crash_state]
            if not base_meta["use"]:
                continue
            crash_id = base_meta["crashId"]
            base = base_meta["patches"]
            if base is None:
                continue
            base_reg = base_meta["reg"]
            is_crash = base_meta["isCrash"]
            if not is_crash:
                if EXTRACTFIX_MODE:
                    if not base_meta["stack-trace"]["passed-crash-loc"]:
                        continue
                result.append((crash_id, base_meta["state"], base_meta["state"], base))
            for crash_test in cluster[crash_state]:
                if crash_test not in analyzer.meta_data:
                    continue
                crash_meta = analyzer.meta_data[crash_test]
                if not crash_meta["use"]:
                    continue
                crash = crash_meta["patches"]
                if crash is None:
                    continue
                crash_reg = crash_meta["reg"]
                crashed = crash_meta["actuallyCrashed"]
                # If input is feasible:
                # crash -> not crash
                # not crash -> not crash + preserve behavior
                # plus, should not remove all possibly correct patches
                if is_crash:
                    if not crashed:
                        no_reg = True
                        for i in range(len(base_reg) - 1):
                            if len(crash_reg) <= i:
                                no_reg = False
                                break
                            if base_reg[i] != crash_reg[i]:
                                no_reg = False
                                break # Regression error
                        if no_reg:
                            result.append((crash_id, base_meta["state"], crash_meta["state"], crash))
                else:
                    if not crashed and base_reg == crash_reg:
                        result.append((crash_id, base_meta["state"], crash_meta["state"], crash))
        
        with open(os.path.join(self.dir, "table_v3.sbsv"), "w") as f:
            original_count, independent_count = analyzer.count_states(all_patches)
            f.write(f"[stat] [states] [original {original_count}] [independent {independent_count}]\n")
            default_removed = set()
            remaining_inputs = list()
            for res in result:
                crash_id, base, test, patches = res
                res_patches = set(patches)
                removed = all_patches - res_patches
                remaining = all_patches & res_patches
                # if len(remaining) == 0:
                #     # Skip if all patches are removed -> most likely infeasible input
                #     continue
                default_removed = default_removed | removed
                remaining_inputs.append(res)
                f.write(f"[sym-in] [id {crash_id}] [base {base}] [test {test}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
            all_patches_default = all_patches - default_removed
            output = list()
            meta_out = list()
            output.append(f"[sym-out] [default] [inputs {len(remaining_inputs)}] [cnt {len(all_patches_default)}] [patches {sorted(list(all_patches_default))}]\n")
            meta_out.append(f"[meta-data] [default] [correct {correct_patch}] [all-patches {len(all_patches)}] [sym-input {len(remaining_inputs)}] [is-correct {correct_patch in all_patches_default}] [patches {sorted(list(all_patches_default))}]\n")
            
            # Further analysis with exit loc
            new_removed = set()
            new_remaining_inputs = list()
            for res in remaining_inputs:
                crash_id, base, test, patches = res
                res_patches = set(patches)
                remaining = all_patches & res_patches
                removed = all_patches - res_patches
                meta = analyzer.meta_data[test]
                meta_base = analyzer.meta_data[base]
                if meta_base["isCrash"]:
                    if meta_base["exitLoc"] != exit_loc:
                        f.write(f"[remove] [crash] [id {crash_id}] [base {base}] [test {test}] [exit-loc {meta_base['exitLoc']}] [exit-res {meta_base['exit']}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
                    else:
                        new_removed = new_removed | removed
                        new_remaining_inputs.append(res)
                else:
                    if EXTRACTFIX_MODE:
                        if not meta["stack-trace"]["passed-crash-loc"]:
                            f.write(f"[remove] [crash] [id {crash_id}] [base {base}] [test {test}] [exit-loc {meta_base['exitLoc']}] [exit-res {meta_base['exit']}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
                            continue
                    new_removed = new_removed | removed
                    new_remaining_inputs.append(res)

            for res in new_remaining_inputs:
                crash_id, base, test, patches = res
                res_patches = set(patches)
                remaining = all_patches & res_patches
                removed = all_patches - res_patches
                meta_base = analyzer.meta_data[base]
                f.write(f"[remain] [crash] [id {crash_id}] [base {base}] [test {test}] [exit-loc {meta_base['exitLoc']}] [exit-res {meta_base['exit']}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
            print_log(f"{len(new_remaining_inputs)}, {len(new_removed)}")
            all_patches_crash = all_patches - new_removed
            output.append(f"[sym-out] [remove-crash] [inputs {len(new_remaining_inputs)}] [cnt {len(all_patches_crash)}] [patches {sorted(list(all_patches_crash))}]\n")
            meta_out.append(f"[meta-data] [remove-crash] [correct {correct_patch}] [all-patches {len(all_patches)}] [sym-input {len(new_remaining_inputs)}] [is-correct {correct_patch in all_patches_crash}] [patches {sorted(list(all_patches_crash))}]\n")
            
            strict_remaining_inputs = list()
            strict_removed = set()
            for res in result:
                crash_id, base, test, patches = res
                res_patches = set()
                patch_eval = analyzer.meta_data[test]["patch-eval"]
                for patch in patch_eval:
                    if patch_eval[patch] == "pass":
                        res_patches.add(patch)
                removed = all_patches - res_patches
                remaining = all_patches & res_patches
                # if len(remaining) == 0:
                #     # Skip if all patches are removed -> most likely infeasible input
                #     continue
                strict_removed = strict_removed | removed
                strict_remaining_inputs.append(res)
                f.write(f"[strict] [id {crash_id}] [base {base}] [test {test}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
            all_patches_strict = all_patches - strict_removed
            output.append(f"[sym-out] [strict] [inputs {len(strict_remaining_inputs)}] [cnt {len(all_patches_strict)}] [patches {sorted(list(all_patches_strict))}]\n")
            meta_out.append(f"[meta-data] [strict] [correct {correct_patch}] [all-patches {len(all_patches)}] [sym-input {len(strict_remaining_inputs)}] [is-correct {correct_patch in all_patches_strict}] [patches {sorted(list(all_patches_strict))}]\n")
            
            strict_new_remaining_inputs = list()
            strict_new_removed = set()
            for res in strict_remaining_inputs:
                crash_id, base, test, patches = res
                res_patches = set()
                patch_eval = analyzer.meta_data[test]["patch-eval"]
                for patch in patch_eval:
                    if patch_eval[patch] == "pass":
                        res_patches.add(patch)
                remaining = all_patches & res_patches
                if len(remaining) == 0:
                    continue
                removed = all_patches - res_patches
                meta = analyzer.meta_data[test]
                meta_base = analyzer.meta_data[base]
                if meta_base["isCrash"]:
                    if meta_base["exitLoc"] != exit_loc:
                        f.write(f"[strict-remove] [crash] [id {crash_id}] [base {base}] [test {test}] [exit-loc {meta_base['exitLoc']}] [exit-res {meta_base['exit']}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
                    else:
                        strict_new_removed = strict_new_removed | removed
                        strict_new_remaining_inputs.append(res)
                else:
                    strict_new_removed = strict_new_removed | removed
                    strict_new_remaining_inputs.append(res)
            all_patches_strict_new = all_patches - strict_new_removed
            for res in strict_new_remaining_inputs:
                crash_id, base, test, patches = res
                res_patches = set()
                patch_eval = analyzer.meta_data[test]["patch-eval"]
                for patch in patch_eval:
                    if patch_eval[patch] == "pass":
                        res_patches.add(patch)
                remaining = all_patches & res_patches
                if len(remaining) == 0:
                    continue
                removed = all_patches - res_patches
                meta_base = analyzer.meta_data[base]
                f.write(f"[strict-remain] [crash] [id {crash_id}] [base {base}] [test {test}] [exit-loc {meta_base['exitLoc']}] [exit-res {meta_base['exit']}] [cnt {len(remaining)}] [patches {sorted(list(remaining))}]\n")
            output.append(f"[sym-out] [strict-remove-crash] [inputs {len(strict_new_remaining_inputs)}] [cnt {len(all_patches_strict_new)}] [patches {sorted(list(all_patches_strict_new))}]\n")
            meta_out.append(f"[meta-data] [strict-remove-crash] [correct {correct_patch}] [all-patches {len(all_patches)}] [sym-input {len(strict_new_remaining_inputs)}] [is-correct {correct_patch in all_patches_strict_new}] [patches {sorted(list(all_patches_strict_new))}]\n")
            
            for out in output:
                f.write(out)
            for meta in meta_out:
                f.write(meta)
                
                
    def mem_file_parser(self, filename: str) -> sbsv.parser:
        parser = sbsv.parser()
        parser.add_schema("[node] [addr: int] [base: int] [size: int] [value: int]")
        parser.add_schema("[sym] [arg] [index: int] [size: int] [name: str]")
        parser.add_schema("[sym] [heap] [type: str] [addr: int] [base: int] [size: int] [name: str]")
        with open(filename, "r") as f:
            parser.load(f)
        return parser
    
    def cluster_symbolic_inputs(self):
        if not os.path.exists(os.path.join(self.dir, "table_v3.sbsv")):
            self.analyze_v3()
        parser = sbsv.parser()
        parser.add_schema("[sym-in] [id: int] [base: int] [test: int] [cnt: int] [patches: str]")
        with open(os.path.join(self.dir, "table_v3.sbsv"), "r") as f:
            result = parser.load(f)
        # symbolic_trace = self.symbolic_trace(analyzer)
        # buggy_trace = self.buggy_trace(analyzer)
        mem_cluster: Dict[frozenset, List[int]] = dict()
        # Cluster collected symbolic inputs
        for sym_in in result["sym-in"]:
            state = sym_in["test"]
            base_id = sym_in["base"]
            # filename: 1 -> test000001.mem (6 digits)
            mem_file = os.path.join(self.dir, f"test{state:06d}.mem")
            parser = self.mem_file_parser(mem_file)
            data = parser.get_result()
            nodes = list()
            for node in data["node"]:
                addr = node["addr"]
                base = node["base"]
                size = node["size"]
                value = node["value"]
                nodes.append((addr, base, size, value))
            key = frozenset(nodes)
            if key not in mem_cluster:
                mem_cluster[key] = list()
            mem_cluster[key].append(state)

        # Save cluster
        data = dict()
        ser_mem_cluster = list()
        data["mem_cluster"] = ser_mem_cluster
        for key, value in mem_cluster.items():
            ser_mem_cluster.append({"file": os.path.join(self.dir, f"test{value[0]:06d}.mem"), "nodes": value})
        with open(os.path.join(self.dir, "symin-cluster.json"), "w") as f:
            json.dump(data, f, indent=2)
    
    def verify_feasibility(self, config: Config, inputs_dir: str, output_dir: str):
        if not os.path.exists(os.path.join(self.dir, "symin-cluster.json")):
            self.cluster_symbolic_inputs()
        with open(os.path.join(self.dir, "symin-cluster.json"), "r") as f:
            mem_cluster = json.load(f)
        inputs = os.listdir(inputs_dir)
        runner = Runner(config)
        bin_file = os.path.basename(config.project_conf["binary_path"])
        validation_runtime = os.path.join(config.conf_files.project_dir, "val-runtime")
        validation_binary = os.path.join(validation_runtime, bin_file)
        group_id = 0
        timeout = config.timeout
        config.timeout = "10s"
        for cluster in mem_cluster:
            group_id += 1
            group_out_dir = os.path.join(output_dir, f"group{group_id}")
            os.makedirs(group_out_dir, exist_ok=True)
            nodes = cluster["nodes"]
            mem_file = cluster["file"]
            # First, run val binary with concrete inputs + mem_file
            env = os.environ.copy()
            env["UNI_KLEE_MEM_BASE_FILE"] = os.path.join(self.dir, "base-mem.graph")
            env["UNI_KLEE_MEM_FILE"] = mem_file
            input_id = 0
            for cinput in inputs:
                c_file = os.path.join(inputs_dir, cinput)
                if os.path.isdir(c_file):
                    continue
                if cinput == "README.txt":
                    continue
                input_id += 1
                env_local = env.copy()
                local_out_file = os.path.join(group_out_dir, f"out-{input_id}.txt")
                env_local["UNI_KLEE_MEM_RESULT"] = local_out_file
                print_log(f"[val] [group {group_id}] [input {input_id}] [out {local_out_file}]")
                opts = config.get_cmd_for_original(c_file)
                cmd = f"{validation_binary} {opts}"
                # if cmd == "": input as stdin
                runner.execute(cmd, validation_runtime, "quiet", env_local)
                if os.path.exists(local_out_file):
                    with open(local_out_file, "a") as f:
                        f.write(f"[input] [id {input_id}] [symgroup {group_id}] [file {cinput}]")
        config.timeout = timeout
    
    def analyze_filtered(self):
        dp = SymRadarDataLogSbsvParser(self.dir)
        if VULMASTER_MODE:
            analyzer = DataAnalyzer(dp)
            analyzer.analyze()
            exit_res_set = {"ret", "exit", "assert.err", "abort.err",
                            "bad_vector_access.err", "free.err", "overflow.err",
                            "overshift.err", "ptr.err", "div.err", "readonly.err",
                             "extractfix"}
            removed = set()
            default_exit_loc = ""
            all_patches = set()
            default_reg = list()
            for state in analyzer.meta_data:
                state_info = analyzer.meta_data[state]
                patches = set(state_info["patches"])
                print_log(f"patches {state} {patches}")
                if 0 in patches:
                    default_reg = state_info["reg"]
                    default_exit_loc = state_info["exitLoc"]
                all_patches = all_patches | patches
            print_log(f"Default reg: {default_reg}")
            for state in analyzer.meta_data:
                state_info = analyzer.meta_data[state]
                patches = set(state_info["patches"])
                exit_res = state_info["exit"]
                exit_loc = state_info["exitLoc"]
                reg = state_info["reg"]
                reg_err = False
                for i in range(len(default_reg) - 1):
                    if len(reg) <= i:
                        reg_err = True
                        break
                    else:
                        if default_reg[i] != reg[i]:
                            reg_err = True
                            break
                if reg_err or 0 in patches:
                    removed = removed | patches
                else:
                    if exit_res in exit_res_set:
                        if exit_res.endswith(".err"):
                            if exit_loc == default_exit_loc:
                                removed = removed | patches
            result = dict()
            result["remaining"] = sorted(list(all_patches - removed))
            with open(os.path.join(self.dir, "filtered.json"), "w") as f:
                json.dump(result, f, indent=2)
                return
        
        patch_trace = dp.parser.get_result()["patch-base"]["trace"]
        remaining_patches = list()
        if len(patch_trace) > 0:
            last_trace = patch_trace[-1]
            patches_str = last_trace["patches"]
            patches = patches_str.strip("[]").strip(", ")
            patch_map = dict()
            for patch in patches.split(", "):
                key, value = patch.split(":")
                patch_map[int(key)] = value
            if 0 in patch_map:
                res = patch_map[0]
                for patch in patch_map:
                    if patch_map[patch] != res:
                        remaining_patches.append(patch)
        result = dict()
        result["remaining"] = remaining_patches
        with open(os.path.join(self.dir, "filtered.json"), "w") as f:
            json.dump(result, f, indent=2)
        print_log(f"Remaining patches: {remaining_patches}")

def arg_parser(argv: List[str]) -> Config:
    # Remaining: c, e, h, i, j, n, q, t, u, v, w, x, y
    parser = argparse.ArgumentParser(description="Test script for uni-klee")
    parser.add_argument("cmd", help="Command to execute", choices=["run", "rerun", "snapshot", "clean", "kill", "filter", "uc", "analyze", "symgroup", "symval"])
    parser.add_argument("query", help="Query for bugid and patch ids: <bugid>[:<patchid>] # ex) 5321:1,2,3,r5-10")
    parser.add_argument("-a", "--additional", help="Additional arguments", default="")
    parser.add_argument("-d", "--debug", help="Debug mode", action="store_true")
    parser.add_argument("-o", "--outdir", help="Output directory", default="")
    parser.add_argument("-p", "--outdir-prefix", help="Output directory prefix(\"out\" for out dir)", default="uni-m-out")
    parser.add_argument("-b", "--snapshot-base-patch", help="Patches for snapshot", default="buggy")
    parser.add_argument("-s", "--snapshot-prefix", help="Snapshot directory prefix", default="")
    parser.add_argument("-l", "--sym-level", help="Symbolization level", default="medium")
    parser.add_argument("-m", "--max-fork", help="Max fork", default="256,256,64")
    parser.add_argument("-t", "--timeout", help="Timeout", default="12h")
    parser.add_argument("-k", "--lock", help="Handle lock behavior", default="i", choices=["i", "w", "f"])
    parser.add_argument("-r", "--rerun", help="Rerun last command with same option", action="store_true")
    parser.add_argument("-z", "--analyze", help="Analyze SymRadar data", action="store_true")
    parser.add_argument("-g", "--use-last", help="Use last output directory", action="store_true")
    parser.add_argument("--mode", help="mode", choices=["symradar", "extractfix"], default="symradar")
    parser.add_argument("--vulmaster-id", help="Vulmaster id", type=int, default=0)
    args = parser.parse_args(argv[1:])
    global VULMASTER_MODE, VULMASTER_ID, EXTRACTFIX_MODE
    if args.mode == "extractfix":
        EXTRACTFIX_MODE = True
    if args.vulmaster_id > 0:
        VULMASTER_MODE = True
        VULMASTER_ID = args.vulmaster_id
        args.outdir_prefix = f"{args.outdir_prefix}_{VULMASTER_ID}"
    if args.snapshot_prefix == "":
        args.snapshot_prefix = f"snapshot-{args.outdir_prefix}"
    conf = Config(args.cmd, args.query, args.debug, args.sym_level, args.max_fork, args.mode)
    if args.analyze:
        conf.conf_files.out_dir = args.query
        return conf
    conf.init(args.snapshot_base_patch, args.rerun, args.additional, args.lock, args.timeout)
    conf.conf_files.set_out_dir(args.outdir, args.outdir_prefix, conf.bug_info, args.snapshot_prefix, "filter", args.use_last)
    return conf


class Runner(uni_klee.Runner):
    config: Config

    def __init__(self, conf: Config):
        self.config = conf
    
    def kill_proc_tree(self, pid: int, including_parent: bool = True):
        parent = psutil.Process(pid)
        children = parent.children(recursive=True)
        for child in children:
            child.kill()
        psutil.wait_procs(children, timeout=5)
        if including_parent:
            parent.kill()
            parent.wait(5)

    def execute(self, cmd: str, dir: str, log_prefix: str, env: dict = None):
        cmd = cmd.replace('-S$(printf \'\\t\\t\\t\')', '-S$(printf "\\t\\t\\t")')
        if "/tmp/out.tiff" in cmd and os.path.exists("/tmp/out.tiff"):
            os.remove("/tmp/out.tiff")
        print_log(f"Change directory to {dir}")
        print_log(f"Executing: {cmd}")
        if env is None:
            env = os.environ
        timeout = 43200
        if self.config.timeout != "":
            if "h" in self.config.timeout:
                timeout = int(self.config.timeout.strip("h")) * 3600
            elif "m" in self.config.timeout:
                timeout = int(self.config.timeout.strip("m")) * 60
            else:
                timeout = int(self.config.timeout.strip("s"))
        timeout += 60
        stdout = subprocess.PIPE # if self.config.debug else subprocess.DEVNULL
        proc = subprocess.Popen(cmd, shell=True, cwd=dir, env=env, stdout=stdout, stderr=subprocess.PIPE)
        timeout_reached = False
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            timeout_reached = True
            print_log(f"Timeout {timeout} seconds {cmd}")
            self.kill_proc_tree(proc.pid)
            stdout, stderr = proc.communicate()
        if log_prefix != "quiet" and (self.config.debug or (proc.returncode != 0 and not timeout_reached)):
            log_file = os.path.join(
                self.config.conf_files.get_log_dir(), f"{log_prefix}.log"
            )
            if proc.returncode != 0:
                print_log("!!!!! Error !!!!")
                print_log("Save error log to " + log_file)
            try:
                print_log(stderr.decode("utf-8", errors="ignore"))
                os.makedirs(self.config.conf_files.get_log_dir(), exist_ok=True)
                with open(os.path.join(self.config.conf_files.get_log_dir(), log_file), "w") as f:
                    f.write(stderr.decode("utf-8", errors="ignore"))
                    f.write("\n###############\n")
                    if stdout:
                        f.write(stdout.decode("utf-8", errors="ignore"))
                print_log(
                    f"Save error log to {self.config.conf_files.get_log_dir()}/{log_prefix}.log"
                )
            except Exception as e:
                print_log(f"Failed to save error: {str(e)}")
        return proc.returncode

    def execute_snapshot(self, cmd: str, dir: str, env: dict = None):
        if self.config.cmd in ["rerun", "snapshot"]:
            self.execute("rm -rf " + self.config.conf_files.snapshot_dir, dir, "rm")
        if self.config.cmd == "filter":
            self.execute("rm -rf " + self.config.conf_files.filter_dir, dir, "rm")
            self.execute(cmd, dir, "filter", env)
            return
        if self.config.cmd == "uc":
            return
        if not os.path.exists(self.config.conf_files.snapshot_file):
            if self.config.debug:
                print_log(
                    f"snapshot file {self.config.conf_files.snapshot_file} does not exist"
                )
            self.execute(cmd, dir, "snapshot", env)
    
    def print_list(self, l: List[Tuple[str, int]]):
        for item, index in l:
            print_log(f"{index}) {item}")

    def interactive_select(self, l: List[Tuple[str, int]], msg: str) -> Tuple[str, int]:
        print_log("Select from here: ")
        self.print_list(l)
        default = l[-1][1]
        while True:
            tmp = input(f"Select {msg}(default: {default}): ").strip()
            res = default
            if tmp == "q":
                return ("", -1)
            if tmp != "":
                res = int(tmp)
            for item, index in l:
                if res == index:
                    return (item, index)

    def get_dir(self):
        print_log("Set_dir")
        dir = self.config.conf_files.out_dir
        # self.filter_dir = self.config.conf_files.filter_dir
        if not os.path.exists(dir):
            print_log(f"{dir} does not exist")
            out_dirs = self.config.conf_files.find_all_nums(
                self.config.conf_files.out_base_dir,
                self.config.conf_files.out_dir_prefix,
            )
            out_dir = self.interactive_select(out_dirs, "dir")[0]
            if out_dir == "":
                print_log("Exit")
                return
            dir = os.path.join(self.config.conf_files.out_base_dir, out_dir)
        print_log(f"Using {dir}")
        return dir

    def run(self):

        if self.config.cmd in ["analyze", "symgroup", "symval"]:
            if self.config.conf_files.out_dir_prefix.startswith("filter"):
                analyzer = SymRadarAnalyzer(self.config.conf_files.filter_dir, self.config.conf_files.filter_dir, self.config.bug_info)
                analyzer.analyze_filtered()
                return
            analyzer = SymRadarAnalyzer(self.get_dir(), self.config.conf_files.filter_dir, self.config.bug_info)
            if self.config.cmd == "analyze":
                analyzer.analyze_v3()
            elif self.config.cmd == "symgroup":
                analyzer.cluster_symbolic_inputs()
            else:
                val_dir = os.path.join(self.config.conf_files.project_dir, "val-runtime")
                val_out_dir = os.path.join(val_dir, "val-out-" + str(self.config.conf_files.find_num(val_dir, "val-out")))
                conc_inputs_dir = self.config.additional
                if conc_inputs_dir == "":
                    conc_inputs_dir = os.path.join(val_dir, "concrete-inputs")
                analyzer.verify_feasibility(self.config, conc_inputs_dir, val_out_dir)
            return
        
        if self.config.cmd in ["clean", "kill"]:
            # 1. Find all processes
            processes = uni_klee.global_config.get_current_processes()
            for proc in processes:
                if proc == self.config.bug_info["id"]:
                    with open(uni_klee.global_config.get_lock_file(self.config.bug_info["bug_id"]),"r") as f:
                        lines = f.readlines()
                        if len(lines) > 1:
                            print_log(f"Kill process {lines[0]}")
                            try:
                                os.kill(int(lines[0]), signal.SIGTERM)
                            except OSError as e:
                                print_log(e.errno)
                    # 2. Remove lock file
                    os.remove(
                        uni_klee.global_config.get_lock_file(
                            self.config.bug_info["bug_id"]
                        )
                    )
            # 3. Remove output directory
            if self.config.cmd == "clean":
                out_dirs = self.config.conf_files.find_all_nums(
                    self.config.conf_files.out_base_dir,
                    self.config.conf_files.out_dir_prefix,
                )
                for out_dir in out_dirs:
                    print_log(f"Remove {out_dir[0]}")
                    os.system(
                        f"rm -rf {os.path.join(self.config.conf_files.out_base_dir, out_dir[0])}"
                    )
            return

        # lock_file = uni_klee.global_config.get_lock_file(self.config.bug_info["bug_id"])
        # lock = uni_klee.acquire_lock(lock_file, self.config.lock, self.config.conf_files.out_dir)
        try:
            cmd = self.config.get_cmd_opts(True)
            self.execute_snapshot(cmd, self.config.workdir)
            if self.config.cmd not in ["snapshot", "filter"]:
                cmd = self.config.get_cmd_opts(False)
                self.execute(cmd, self.config.workdir, "uni-klee")
                analyzer = SymRadarAnalyzer(self.get_dir(), self.config.conf_files.filter_dir, self.config.bug_info)
                analyzer.analyze_v3()
            elif self.config.cmd == "filter":
                analyzer = SymRadarAnalyzer(self.config.conf_files.filter_dir, self.config.conf_files.filter_dir, self.config.bug_info)
                analyzer.analyze_filtered()
        except Exception as e:
            print_log(f"Exception: {e}")
            print_log(traceback.format_exc())
        # finally:
            # uni_klee.release_lock(lock_file, lock)


def main():
    os.chdir(uni_klee.ROOT_DIR)
    cmd = sys.argv[1]
    if cmd != "trace":
        conf = arg_parser(sys.argv)
        runner = Runner(conf)
        runner.run()
    elif cmd == "trace":
        get_trace(sys.argv[2], int(sys.argv[3]))


if __name__ == "__main__":
    main()
