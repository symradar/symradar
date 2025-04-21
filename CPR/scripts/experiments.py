#!/usr/bin/env python3
from typing import Union, List, Dict, Tuple, Optional, Set, TextIO
import multiprocessing as mp
import subprocess

import os
import sys
import json
import time
import datetime
import sbsv
import argparse
import psutil
import re

# import importlib
# PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# import module from uni_klee.py
import uni_klee
import symradar

ROOT_DIR = uni_klee.ROOT_DIR
GLOBAL_LOG_DIR = os.path.join(ROOT_DIR, "logs")
OUTPUT_DIR = "out"
PREFIX = ""
SYMRADAR_PREFIX = "uni-m-out"
SNAPSHOT_PREFIX = ""
MODE = "symradar"
VULMASTER_MODE = False

def log_out(msg: str):
  print(msg, file=sys.stderr)

def kill_proc_tree(pid: int, including_parent: bool = True):
  parent = psutil.Process(pid)
  children = parent.children(recursive=True)
  for child in children:
    child.kill()
  psutil.wait_procs(children, timeout=5)
  if including_parent:
    parent.kill()
    parent.wait(5)

def execute(cmd: str, dir: str, log_file: str, log_dir: str, prefix: str, meta: dict):
  log_out(f"Executing: {cmd}")
  start = time.time()
  timeout = 72 * 3600 + 600 # 12 hours + 10 minutes for analysis
  proc = subprocess.Popen(cmd, shell=True, cwd=dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  try:
    stdout, stderr = proc.communicate(timeout=timeout)
  except subprocess.TimeoutExpired:
    log_out(f"Timeout: {cmd}")
    kill_proc_tree(proc.pid)
    stdout, stderr = proc.communicate()
  finally:
    end = time.time()
  log_out(f"Done {prefix}: {end - start}s")
  with open(os.path.join(GLOBAL_LOG_DIR, "time.log"), "a") as f:
    f.write(f"{prefix},{end - start}\n")
  # if log_dir == "exp":
  #   collect_result(meta)
  if proc.returncode != 0:
    log_out(f"Failed to execute: {cmd}")
    log_out(stderr.decode("utf-8", errors="ignore"))
    try:
      if not os.path.exists(os.path.join(GLOBAL_LOG_DIR, log_dir)):
        os.makedirs(os.path.join(GLOBAL_LOG_DIR, log_dir), exist_ok=True)
      with open(os.path.join(GLOBAL_LOG_DIR, log_dir, log_file), "w") as f:
        f.write(stderr.decode("utf-8", errors="ignore"))
        f.write("\n###############\n")
        f.write(stdout.decode("utf-8", errors="ignore"))
    except Exception as e:
      log_out(f"Failed to write log file: {log_file}")
      log_out(e)
    return False
  return True

def execute_wrapper(args):
  return execute(*args)

class RunSingleVulmaster():
  meta: dict = None
  meta_program: dict = None
  conf: dict = None
  vids: List[int] = None
  def __init__(self, id: int):
    res = uni_klee.global_config.get_meta_data_info_by_id(id)
    self.meta = res["meta"]
    self.meta_program = res["meta_program"]
    self.conf = res["conf"]
    self.vids = list()
    vulmaster_dir = os.path.join(ROOT_DIR, "patches", self.meta["benchmark"], self.meta["subject"], self.meta["bug_id"], "vulmaster")
    if not os.path.exists(vulmaster_dir):
      log_out(f"Vulmaster dir not found: {vulmaster_dir}")
      return
    for vulmaster_file in os.listdir(vulmaster_dir):
      match = re.search(r'vulmaster-([^.]+)\.c$', vulmaster_file)
      if not match:
        continue
      vid = match.group(1)
      if not vid.isdigit():
        continue
      self.vids.append(int(vid))
    self.vids = sorted(self.vids)
    if self.meta["bug_id"] == "CVE-2017-15025":
      # Use cases that passed filtering
      self.vids = [2, 3, 4]
    else:
      vids = []
      for vid in self.vids:
        filter_result_file = os.path.join(ROOT_DIR, "patches", self.meta["benchmark"], self.meta["subject"], self.meta["bug_id"], "vulmaster-patched", f"filter_{vid}", "filtered.json")
        if not os.path.exists(filter_result_file):
          log_out(f"Filter result file not found: {filter_result_file}")
          vids.append(vid)
          continue
        with open(filter_result_file, "r") as f:
          data = json.load(f)
          if len(data["remaining"]) > 0:
            vids.append(vid)
            log_out(f"Vulmaster {vid} has remaining patches: {data['remaining']}")
          else:
            log_out(f"Vulmaster {vid} has no remaining patches")
      self.vids = vids

        

  def get_clean_cmd(self) -> List[str]:
    return [f"symradar.py clean {self.meta['bug_id']} --vulmaster-id={vid}" for vid in self.vids]
  def get_filter_cmd(self) -> List[str]:
    return [f"symradar.py filter {self.meta['bug_id']} --mode={MODE} --vulmaster-id={vid}" for vid in self.vids]
  def get_analyze_cmd(self, extra: str = "") -> List[str]:
    if extra != "" and extra != "exp":
      return [f"symradar.py {extra} {self.meta['bug_id']} --mode={MODE} --use-last -p {SYMRADAR_PREFIX} --vulmaster-id={vid}" for vid in self.vids]
    return [f"symradar.py analyze {self.meta['bug_id']} --mode={MODE} --use-last -p {SYMRADAR_PREFIX} --vulmaster-id={vid}" for vid in self.vids]
  
  def get_exp_cmd(self, opt: str, extra: str) -> List[str]:
    if "correct" not in self.meta:
      log_out("No correct patch")
      return None
    result = list()
    query = self.meta["bug_id"] + ":0" # ",".join([str(x) for x in patches])
    for vid in self.vids:
      if extra == "snapshot":
        cmd = f"symradar.py snapshot {query} --outdir-prefix={SYMRADAR_PREFIX} --mode={MODE} --vulmaster-id={vid}"
        result.append(cmd)
        continue
      cmd = f"symradar.py rerun {query} --outdir-prefix={SYMRADAR_PREFIX} --mode={MODE} --vulmaster-id={vid}"
      if opt == "run":
        cmd = f"symradar.py run {query} --outdir-prefix={SYMRADAR_PREFIX} --mode={MODE} --vulmaster-id={vid}"
        if SNAPSHOT_PREFIX != "":
          cmd += f" --snapshot-prefix={SNAPSHOT_PREFIX}"
      if extra == "k2-high":
        cmd += " --sym-level=high --additional='--symbolize-bound=2' --max-fork=1024,1024,1024"
      if extra == "high":
        cmd += " --sym-level=high" #  --max-fork=1024,1024,128
      if extra == "k2":
        cmd += " --additional='--symbolize-bound=2' --max-fork=1024,1024,1024"
      if extra == "low":
        cmd += " --sym-level=low" # --max-fork=1024,1024,1024
      if extra == "none":
        cmd += " --sym-level=none"
      result.append(cmd)
    return result
  
  def get_uc_cmd(self, extra: str) -> List[str]:
    if "correct" not in self.meta:
      log_out("No correct patch")
      return None
    if "no" not in self.meta["correct"]:
      for patch in self.meta_program["patches"]:
        if patch["name"] == "correct":
          self.meta["correct"]["no"] = patch["id"]
          break
    if "no" not in self.meta["correct"]:
      log_out("No correct patch")
      return None
    result = list()
    for vid in self.vids:
      cmd = f"symradar.py uc {self.meta['bug_id']}:0 --outdir-prefix={SYMRADAR_PREFIX} --snapshot-prefix=snapshot-{SYMRADAR_PREFIX} --mode={MODE} --vulmaster-id={vid}"
      result.append(cmd)
    return result

  def get_util_cmd(self, extra: str) -> List[str]:
    if extra in ["fuzz", "build", "val-build", "fuzz-build", "extractfix-build", "vulmaster-build", "vulmaster-extractfix-build", "fuzz-seeds", "collect-inputs", "group-patches", "val", "feas", "analyze", "check"]:
      return [f"symutil.py {extra} {self.meta['bug_id']} -s {SYMRADAR_PREFIX}"]
    log_out(f"Unknown extra: {extra}")
    exit(1)
    
  def get_cmd(self, opt: str, extra: str) -> List[str]:
    # if "correct" not in self.meta:
    #   log_out("No correct patch")
    #   return None
    if "no" not in self.meta["correct"]:
      log_out("No correct patch")
      return None
    if opt == "filter":
      return self.get_filter_cmd()
    if opt == "clean":
      return self.get_clean_cmd()
    if opt == "exp" or opt == "run":
      return self.get_exp_cmd(opt, extra)
    if opt == "uc":
      return self.get_uc_cmd(extra)
    if opt == "analyze":
      return self.get_analyze_cmd(extra)
    if opt == "util": # clean with rm -r patches/*/*/*/runtime/aflrun-out-*
      return self.get_util_cmd(extra)
    log_out(f"Unknown opt: {opt}")
    return []

class RunSingle():
  meta: dict = None
  meta_program: dict = None
  conf: dict = None  
  def __init__(self, id: int):
    res = uni_klee.global_config.get_meta_data_info_by_id(id)
    self.meta = res["meta"]
    self.meta_program = res["meta_program"]
    self.conf = res["conf"]
  def get_clean_cmd(self) -> str:
    return f"symradar.py clean {self.meta['bug_id']}"
  def get_filter_cmd(self) -> str:
    return f"symradar.py filter {self.meta['bug_id']} --mode={MODE}"
  def get_analyze_cmd(self, extra: str = "") -> str:
    if extra != "" and extra != "exp":
      return f"symradar.py {extra} {self.meta['bug_id']} --mode={MODE} --use-last -p {SYMRADAR_PREFIX}"
    return f"symradar.py analyze {self.meta['bug_id']} --mode={MODE} --use-last -p {SYMRADAR_PREFIX}"
  def get_exp_cmd(self, opt: str, extra: str = "") -> str:
    if "correct" not in self.meta:
      log_out("No correct patch")
      return None
    if "no" not in self.meta["correct"]:
      for patch in self.meta_program["patches"]:
        if patch["name"] == "correct":
          self.meta["correct"]["no"] = patch["id"]
          break
    if "no" not in self.meta["correct"]:
      log_out("No correct patch")
      return None
    correct = self.meta["correct"]["no"]
    # patches = list()
    # cnt = 0
    # limit = 100
    # for patch in self.meta_program["patches"]:
    #   patches.append(patch["id"])
    #   cnt += 1
    #   if cnt >= limit:
    #     if correct not in patches:
    #       patches.append(correct)
    #       patches.append(correct + 1)
    #       patches.append(correct - 1)
    #     break
    # log_out(patches)
    query = self.meta["bug_id"] + ":0" # ",".join([str(x) for x in patches])
    cmd = f"symradar.py rerun {query} --outdir-prefix={SYMRADAR_PREFIX} --mode={MODE}"
    if opt == "run":
      cmd = f"symradar.py run {query} --outdir-prefix={SYMRADAR_PREFIX} --mode={MODE}"
      if SNAPSHOT_PREFIX != "":
        cmd += f" --snapshot-prefix={SNAPSHOT_PREFIX}"
    if extra == "k2-high":
      cmd += " --sym-level=high --additional='--symbolize-bound=2' --max-fork=1024,1024,1024"
    if extra == "high":
      cmd += " --sym-level=high" #  --max-fork=1024,1024,128
    if extra == "k2":
      cmd += " --additional='--symbolize-bound=2' --max-fork=1024,1024,1024"
    if extra == "low":
      cmd += " --sym-level=low" # --max-fork=1024,1024,1024
    if extra == "none":
      cmd += " --sym-level=none"
    return cmd
  
  def get_uc_cmd(self, extra: str) -> str:
    if "correct" not in self.meta:
      log_out("No correct patch")
      return None
    if "no" not in self.meta["correct"]:
      for patch in self.meta_program["patches"]:
        if patch["name"] == "correct":
          self.meta["correct"]["no"] = patch["id"]
          break
    if "no" not in self.meta["correct"]:
      log_out("No correct patch")
      return None
    cmd = f"symradar.py uc {self.meta['bug_id']}:0 --outdir-prefix={SYMRADAR_PREFIX} --snapshot-prefix=snapshot-{SYMRADAR_PREFIX} " # --max-fork=1024,1024,1024
    return cmd

  def get_util_cmd(self, extra: str) -> str:
    if extra == "exp":
      return f"symutil.py fuzz {self.meta['bug_id']}"
    if extra in ["fuzz", "build", "val-build", "fuzz-build", "extractfix-build", "fuzz-seeds", "collect-inputs", "group-patches", "val", "feas", "analyze", "check"]:
      return f"symutil.py {extra} {self.meta['bug_id']} -s {SYMRADAR_PREFIX}"
    log_out(f"Unknown extra: {extra}")
    exit(1)
    
  def get_cmd(self, opt: str, extra: str) -> str:
    # if "correct" not in self.meta:
    #   log_out("No correct patch")
    #   return None
    if "no" not in self.meta["correct"]:
      log_out("No correct patch")
      return None
    if opt == "filter":
      return self.get_filter_cmd()
    if opt == "clean":
      return self.get_clean_cmd()
    if opt == "exp" or opt == "run":
      return self.get_exp_cmd(opt, extra)
    if opt == "uc":
      return self.get_uc_cmd(extra)
    if opt == "analyze":
      return self.get_analyze_cmd(extra)
    if opt == "util": # clean with rm -r patches/*/*/*/runtime/aflrun-out-*
      return self.get_util_cmd(extra)
    log_out(f"Unknown opt: {opt}")
    return None

def check_correct_exists(meta: dict) -> bool:
  if "correct" not in meta:
    return False
  if "no" not in meta["correct"]:
    return False
  return True

def check_use_high_level(meta: dict) -> bool:
  conf = symradar.Config("analyze", meta["bug_id"], False, "high", "64,64,64")
  conf.init("snapshot", False, "", "f")
  conf.conf_files.set_out_dir("", "uni-m-out", conf.bug_info, "snapshot", "filter", True)
  if not os.path.exists(conf.conf_files.out_dir):
    return False
  if not os.path.exists(os.path.join(conf.conf_files.out_dir, "table.sbsv")):
    # Run analysis
    run_cmd("analyze", [meta], "", "")
  if not os.path.exists(os.path.join(conf.conf_files.out_dir, "table.sbsv")):
    return True
  parser = parse_result(os.path.join(conf.conf_files.out_dir, "table.sbsv"))
  result = parser.get_result()
  if result is None:
    return True
  if len(result["sym-in"]) > 16:
    log_out(f"Skip high level: {meta['bug_id']}")
    return False
  return True

def collect_result(meta: dict):
  conf = symradar.Config("analyze", meta["bug_id"], False, "high", "64,64,64")
  conf.init("snapshot", False, "", "f")
  conf.conf_files.set_out_dir("", SYMRADAR_PREFIX, conf.bug_info, "snapshot", "filter", True)
  save_dir = os.path.join(OUTPUT_DIR, PREFIX, meta["subject"], meta["bug_id"])
  os.makedirs(save_dir, exist_ok=True)
  if not os.path.exists(conf.conf_files.out_dir):
    return False
  if not os.path.exists(os.path.join(conf.conf_files.out_dir, "table.sbsv")):
    return False
  log_out(f"save to {save_dir}")
  save_file = os.path.join(save_dir, f"table.sbsv")
  if os.path.exists(save_file):
    os.unlink(save_file)
  # copy file
  with open(os.path.join(conf.conf_files.out_dir, "table.sbsv"), "r") as f:
    with open(save_file, "w") as f2:
      f2.write(f.read())

def parse_result(file: str) -> sbsv.parser:
  parser = sbsv.parser()
  parser.add_schema("[sym-in] [id: int] [base: int] [test: int] [cnt: int] [patches: str]")
  parser.add_schema("[sym-out] [default] [cnt: int] [patches: str]")
  parser.add_schema("[sym-out] [best] [cnt: int] [patches: str]")
  parser.add_schema("[meta-data] [correct: int] [all-patches: int] [sym-input: int] [correct-input: int]")
  with open(file, "r") as f:
    parser.load(f)
  return parser

def parse_result_v3(file: str) -> sbsv.parser:
  parser = sbsv.parser()
  parser.add_schema("[stat] [states] [original: int] [independent: int]")
  parser.add_schema("[sym-in] [id: int] [base: int] [test: int] [cnt: int] [patches: str]")
  parser.add_schema("[remove] [crash] [id: int] [base: int] [test: int] [exit-loc: str] [exit-res: str] [cnt: int] [patches: str]")
  parser.add_schema("[remain] [crash] [id: int] [base: int] [test: int] [exit-loc: str] [exit-res: str] [cnt: int] [patches: str]")
  parser.add_schema("[strict] [id: int] [base: int] [test: int] [cnt: int] [patches: str]")
  parser.add_schema("[strict-remove] [crash] [id: int] [base: int] [test: int] [exit-loc: str] [exit-res: str] [cnt: int] [patches: str]")
  parser.add_schema("[strict-remain] [crash] [id: int] [base: int] [test: int] [exit-loc: str] [exit-res: str] [cnt: int] [patches: str]")
  parser.add_schema("[sym-out] [default] [inputs: int] [cnt: int] [patches: str]")
  parser.add_schema("[sym-out] [remove-crash] [inputs: int] [cnt: int] [patches: str]")
  parser.add_schema("[sym-out] [strict] [inputs: int] [cnt: int] [patches: str]")
  parser.add_schema("[sym-out] [strict-remove-crash] [inputs: int] [cnt: int] [patches: str]")
  parser.add_schema("[meta-data] [default] [correct: int] [all-patches: int] [sym-input: int] [is-correct: bool] [patches: str]")
  parser.add_schema("[meta-data] [remove-crash] [correct: int] [all-patches: int] [sym-input: int] [is-correct: bool] [patches: str]")
  parser.add_schema("[meta-data] [strict] [correct: int] [all-patches: int] [sym-input: int] [is-correct: bool] [patches: str]")
  parser.add_schema("[meta-data] [strict-remove-crash] [correct: int] [all-patches: int] [sym-input: int] [is-correct: bool] [patches: str]")
  with open(file, "r") as f:
    parser.load(f)
  return parser

def str_to_list(s: str) -> List[int]:
  ss = s.strip('[]')
  res = list()
  for x in ss.split(", "):
    if x.strip() == "":
      continue
    res.append(int(x))
  return res

def find_num(dir: str, prefix: str) -> int:
  result = 0
  dirs = os.listdir(dir)
  while True:
    if f"{prefix}-{result}" in dirs:
      result += 1
    else:
      break
  return result

def symradar_final_result(meta: dict, result_f: TextIO):
  subject = meta["subject"]
  bug_id = meta["bug_id"]
  incomplete = meta["correct"]["incomplete"]
  subject_dir = os.path.join(ROOT_DIR, "patches", meta["benchmark"], subject, bug_id)
  patched_dir = os.path.join(subject_dir, "patched")
  out_dir_no = find_num(patched_dir, SYMRADAR_PREFIX) - 1
  out_file = os.path.join(patched_dir, f"{SYMRADAR_PREFIX}-{out_dir_no}", "table.sbsv")
  if not os.path.exists(out_file):
    log_out(f"File not found: {out_file}")
    result_f.write("\t\t\t\t\t\t\t\t\t\n")
    return
  parser = parse_result(out_file)
  result = parser.get_result()
  if result is None:
    log_out(f"Failed to parse: {out_file}")
    result_f.write("\t\t\t\t\t\t\t\t\t\n")
    return
  
  meta_data = result["meta-data"]
  all_patches = meta_data[0]["all-patches"]
  
  filter_result_file = os.path.join(subject_dir, "patched", "filter", "filtered.json")
  filter_result = set(range(1, all_patches))
  if not os.path.exists(filter_result_file):
    log_out(f"File not found: {filter_result_file}")
  with open(filter_result_file, "r") as f:
    data = json.load(f)
    filter_result = set(data["remaining"])
  
  with open(os.path.join(subject_dir, "group-patches-original.json"), "r") as f:
    group_patches = json.load(f)
  patch_group_tmp = dict()
  correct_patch = group_patches["correct_patch_id"]
  for patches in group_patches["equivalences"]:
    representative = patches[0]
    for patch in patches:
      patch_group_tmp[patch] = representative
  patch_eq_map = dict()
  for patch in filter_result:
    if patch in patch_group_tmp:
      patch_eq_map[patch] = patch_group_tmp[patch]
    else:
      patch_eq_map[patch] = patch
  all_patches = set()
  for patch in filter_result:
    if patch == patch_eq_map[patch]:
      all_patches.add(patch)
  correct_patch = patch_eq_map[correct_patch]
  
  sym_inputs = len(result["sym-in"])
  default_patches = str_to_list(result["sym-out"]["default"][0]["patches"])
  
  symin_map = dict()
  default_patches_new = set(range(all_patches))
  sym_in_new_num = 0
  for sym_in in result["sym-in"]:
    state = sym_in["test"]
    base = sym_in["base"]
    symin_map[state] = sym_in
  for state in symin_map:
    symin = symin_map[state]
    base = symin["base"]
    patches = str_to_list(symin["patches"])
    # auto remove
    removed = True
    for p in patches:
      if p in filter_result:
        removed = False
        break
    if removed:
      continue
    else:
      removed_patches = set()
      for p in default_patches_new:
        if p not in patches:
          removed_patches.add(p)
      default_patches_new = default_patches_new - removed_patches
      sym_in_new_num += 1
    if base in symin_map:
      base_patches = str_to_list(symin_map[base]["patches"])
      if base_patches != patches:
        log_out(f"ERROR in {subject}/{bug_id}!!!!: {state} {base}")
  
  if sym_inputs == 0:
    default_patches = list(range(all_patches))
  default = len(default_patches)
  default_filtered = 0
  for p in default_patches:
    if p in filter_result:
      default_filtered += 1
  default_found = correct_patch in default_patches

  if sym_in_new_num == 0:
    default_patches_new = list(range(all_patches))
  
  default_patches_new_filtered_num = 0
  for p in default_patches_new:
    if p in filter_result:
      default_patches_new_filtered_num += 1
  
  default_patches_new_found = correct_patch in default_patches_new
  
  best_inputs = meta_data[0]["correct-input"]
  best_patches = str_to_list(result["sym-out"]["best"][0]["patches"])
  if best_inputs == 0:
    best_patches = list(range(all_patches))
  best = len(best_patches)
  best_found = correct_patch in best_patches
  best_filtered = 0
  for p in best_patches:
    if p in filter_result:
      best_filtered += 1

  result_f.write(f"{subject}\t{bug_id}\t{correct_patch}\t{all_patches}\t{incomplete}\t{sym_inputs}\t{default}\t{default_filtered}\t{default_found}\t{best_inputs}\t{best}\t{best_filtered}\t{best_found}\t{sym_in_new_num}\t{len(default_patches_new)}\t{default_patches_new_filtered_num}\t{default_patches_new_found}\n")


def symradar_res_to_str(res: dict) -> str:
  patches = str_to_list(res["patches"])
  return f"{res['sym-input']}\t{len(patches)}\t{res['is-correct']}"

def symradar_final_result_vulmaster_v3(meta: dict, result_f: TextIO):
  subject = meta["subject"]
  bug_id = meta["bug_id"]
  incomplete = meta["correct"]["incomplete"]
  subject_dir = os.path.join(ROOT_DIR, "patches", meta["benchmark"], subject, bug_id)
  patched_dir = os.path.join(subject_dir, "vulmaster-patched")
  rsv = RunSingleVulmaster(meta["id"])
  for vid in rsv.vids:
    prefix = f"{SYMRADAR_PREFIX}_{vid}"
    out_dir_no = find_num(patched_dir, prefix) - 1
    out_file = os.path.join(patched_dir, f"{prefix}-{out_dir_no}", "table_v3.sbsv")
    if not os.path.exists(out_file):
      log_out(f"File not found: {out_file}")
      result_f.write("\t\t\t\t\t\t\t\t\t\n")
      return
    parser = parse_result_v3(out_file)
    result = parser.get_result()
    if result is None:
      log_out(f"Failed to parse: {out_file}")
      result_f.write("\t\t\t\t\t\t\t\t\t\n")
      return
    
    filter_result_file = os.path.join(patched_dir, f"filter_{vid}", "filtered.json")
    filter_result = set()
    if not os.path.exists(filter_result_file):
      log_out(f"File not found: {filter_result_file}")
    with open(filter_result_file, "r") as f:
      data = json.load(f)
      filter_result = set(data["remaining"])
    correct_patch = 1 # This is not actually correct - check
    all_patches = set(filter_result)
    
    meta_data_default = result["meta-data"]["default"]
    meta_data_default_remove_crash = result["meta-data"]["remove-crash"]
    meta_data_strict = result["meta-data"]["strict"]
    meta_data_strict_remove_crash = result["meta-data"]["strict-remove-crash"]
    all_patches = meta_data_default[0]["all-patches"]

    default_str = symradar_res_to_str(meta_data_default[0])
    default_remove_crash_str = symradar_res_to_str(meta_data_default_remove_crash[0])
    strict_str = symradar_res_to_str(meta_data_strict[0])
    strict_remove_crash_str = symradar_res_to_str(meta_data_strict_remove_crash[0])
    
    stat = result["stat"]["states"][0]
    
    result_f.write(f"{subject}\t{bug_id}_{vid}\t{correct_patch}\t{all_patches}\t{incomplete}\t{default_str}\t{strict_str}\t{default_remove_crash_str}\t{strict_remove_crash_str}\t{stat['original']}\t{stat['independent']}\n")
    

def get_all_patches(file: str) -> Tuple[Set[int], int]:
  with open(file, "r") as f:
    group_patches = json.load(f)
    patch_group_tmp = dict()
    correct_patch = group_patches["correct_patch_id"]
    for patches in group_patches["equivalences"]:
      representative = patches[0]
      for patch in patches:
        patch_group_tmp[patch] = representative
    patch_eq_map = dict()
    all_patches = set()
    for patch in range(1, correct_patch + 1):
      if patch in patch_group_tmp:
        patch_eq_map[patch] = patch_group_tmp[patch]
      else:
        patch_eq_map[patch] = patch
      all_patches.add(patch_eq_map[patch])
    
    if correct_patch in patch_eq_map:
      correct_patch = patch_eq_map[correct_patch]      
    return all_patches, correct_patch

def symradar_final_result_v3(meta: dict, result_f: TextIO):
  subject = meta["subject"]
  bug_id = meta["bug_id"]
  incomplete = meta["correct"]["incomplete"]
  subject_dir = os.path.join(ROOT_DIR, "patches", meta["benchmark"], subject, bug_id)
  patched_dir = os.path.join(subject_dir, "patched")
  out_dir_no = find_num(patched_dir, SYMRADAR_PREFIX) - 1
  out_file = os.path.join(patched_dir, f"{SYMRADAR_PREFIX}-{out_dir_no}", "table_v3.sbsv")
  if not os.path.exists(out_file):
    log_out(f"File not found: {out_file}")
    result_f.write("\t\t\t\t\t\t\t\t\t\n")
    return
  parser = parse_result_v3(out_file)
  result = parser.get_result()
  if result is None:
    log_out(f"Failed to parse: {out_file}")
    result_f.write("\t\t\t\t\t\t\t\t\t\n")
    return
  
  filter_result_file = os.path.join(subject_dir, "patched", "filter", "filtered.json")
  filter_result = set()
  if not os.path.exists(filter_result_file):
    log_out(f"File not found: {filter_result_file}")
  with open(filter_result_file, "r") as f:
    data = json.load(f)
    filter_result = set(data["remaining"])

  all_patches, correct_patch = get_all_patches(os.path.join(subject_dir, "group-patches-original.json"))
  
  meta_data_default = result["meta-data"]["default"]
  meta_data_default_remove_crash = result["meta-data"]["remove-crash"]
  meta_data_strict = result["meta-data"]["strict"]
  meta_data_strict_remove_crash = result["meta-data"]["strict-remove-crash"]
  # all_patches = meta_data_default[0]["all-patches"]

  default_str = symradar_res_to_str(meta_data_default[0])
  default_remove_crash_str = symradar_res_to_str(meta_data_default_remove_crash[0])
  strict_str = symradar_res_to_str(meta_data_strict[0])
  strict_remove_crash_str = symradar_res_to_str(meta_data_strict_remove_crash[0])
  
  stat = result["stat"]["states"][0]
  
  result_f.write(f"{subject}\t{bug_id}\t{correct_patch}\t{len(all_patches)}\t{incomplete}\t{default_str}\t{strict_str}\t{default_remove_crash_str}\t{strict_remove_crash_str}\t{stat['original']}\t{stat['independent']}\n")
  

def final_analysis(meta_data: List[dict], output: str):
  if output == "":
    output = f"{PREFIX}_{SYMRADAR_PREFIX}_final.csv"
  meta_data = sorted(meta_data, key=lambda x: f"{x['subject']}/{x['bug_id']}")
  result_f = open(os.path.join(OUTPUT_DIR, output), "w")
  # result_f.write(f"project\tbug\tcorrect_patch\tall_patches\tincomplete\tinputs\tdefault_remaining_patches\tdefault_filtered_patches\tdefault_found\tbest_inputs\tbest_remaining_patches\tbest_filtered_patches\tbest_found\tdefault_inputs_new\tdefault_patches_new\tdefault_patches_new_filtered\tdefault_patches_new_found\n")
  result_f.write(f"project\tbug\tcorrect_patch\tall_patches\tincomplete\tdefault_inputs\tdefault_remaining_patches\tdefault_correct\tstrict_inputs\tstrict_remaining_patches\tstrict_correct\tdefault_inputs_heuristic\tdefault_patches_heuristic\tdefault_found_heuristic\tstrict_inputs_heuristic\tstrict_patches_heuristic\tstrict_found_heuristic\tstates\tind_states\n")
  for meta in meta_data:
    if not check_correct_exists(meta):
      continue
    if VULMASTER_MODE:
      symradar_final_result_vulmaster_v3(meta, result_f)
    else:
      symradar_final_result_v3(meta, result_f)
    # print(f"{meta['subject']}\t{meta['bug_id']}")
    # sub_dir = os.path.join(ROOT_DIR, "patches", meta["benchmark"], meta["subject"], meta['bug_id'], "patched")
    # no = find_num(sub_dir, SYMRADAR_PREFIX) - 1
    # symbolic_global = os.path.join(sub_dir, f"{SYMRADAR_PREFIX}-{no}", "base-mem.symbolic-globals")
    # if os.path.exists(symbolic_global):
    #   with open(symbolic_global, 'r') as f:
    #     print(f.read())
  result_f.close()
  log_out(f"Final result saved to {os.path.join(OUTPUT_DIR, output)}")

def read_tmp():
  result = dict()
  with open("/root/projects/CPR/out/tmp.table", "r") as f:
    lines = f.readlines()
    for line in lines:
      no, bug_id = line.strip().split()
      result[no] = bug_id
  meta_data = uni_klee.global_config.get_meta_data_list()
  for res in result:
    meta = uni_klee.global_config.get_bug_info(res)
    if meta is None:
      continue
    # Run rsync
    os.system(f"rsync -avz -e 'ssh -p 1601' root@10.20.26.23:/home/yuntong/vulnfix/data/{result[res]}/cludafl_out/out-cludafl-energy-reset/queue /root/projects/CPR/patches/extractfix/{meta['subject']}/{meta['bug_id']}/runtime/cludafl-queue")
    os.system(f"rsync -avz -e 'ssh -p 1601' root@10.20.26.23:/home/yuntong/vulnfix/data/{result[res]}/cludafl_out/out-cludafl-energy-reset/memory/input /root/projects/CPR/patches/extractfix/{meta['subject']}/{meta['bug_id']}/runtime/cludafl-memory")
  return result

def run_clean(meta_data: List[dict]):
  for meta in meta_data:
    if not check_correct_exists(meta):
      continue
    runtime = os.path.join(ROOT_DIR, "patches", "extractfix", meta["subject"], meta["bug_id"], "runtime")
    files = os.listdir(runtime)
    for file in files:
      full_name = os.path.join(runtime, file)
      if os.path.isdir(full_name):
        continue
      if file.endswith(".aflrun"):
        continue
      os.remove(full_name)

def run_cmd(opt: str, meta_data: List[dict], extra: str, additional: str):
  args_list = list()
  for meta in meta_data:
    if not check_correct_exists(meta):
      continue
    if VULMASTER_MODE:
      rsv = RunSingleVulmaster((meta["id"]))
      cmds = rsv.get_cmd(opt, extra)
      for cmd in cmds:
        args_list.append((cmd, ROOT_DIR, f"{meta['bug_id']}.log", opt, f"{opt},{meta['subject']}/{meta['bug_id']}", meta))
      continue
    rs = RunSingle(meta["id"])
    cmd = rs.get_cmd(opt, extra)
    if cmd is None:
      continue
    if additional != "":
      cmd = f"{cmd} {additional}"
    args_list.append((cmd, ROOT_DIR, f"{meta['bug_id']}.log", opt, f"{opt},{meta['subject']}/{meta['bug_id']}", meta))
  log_out(f"Total {opt}: {len(args_list)}")
  core = len(args_list)
  pool = mp.Pool(core)
  pool.map(execute_wrapper, args_list)
  pool.close()
  pool.join()
  log_out(f"{opt} done")

def run_cmd_seq(opt: str, meta_data: List[dict], extra: str, additional: str, output: str):
  meta_data = sorted(meta_data, key=lambda x: f"{x['subject']}/{x['bug_id']}")
  for meta in meta_data:
    if not check_correct_exists(meta):
      continue
    rs = RunSingle(meta["id"])
    cmd = rs.get_cmd(opt, extra)
    if cmd is None:
      continue
    if additional != "":
      cmd = f"{cmd} {additional}"
    if output != "":
      cmd = f"{cmd} >> {OUTPUT_DIR}/{output}"
    else:
      cmd = f"{cmd} >> {OUTPUT_DIR}/{PREFIX}.log"
    execute(cmd, ROOT_DIR, f"{meta['bug_id']}.log", opt, f"{opt},{meta['subject']}/{meta['bug_id']}", meta)
  log_out(f"{opt} done")

def main(argv: List[str]):
  parser = argparse.ArgumentParser(description="Run SymRadar experiments")
  parser.add_argument("cmd", type=str, help="Command to run", choices=["filter", "exp", "run", "uc", "analyze", "final", "util", "clean"], default="exp")
  parser.add_argument("-e", "--extra", type=str, help="Subcommand", default="exp")
  parser.add_argument("-o", "--output", type=str, help="Output file", default="", required=False)
  parser.add_argument("-p", "--prefix", type=str, help="Output prefix", default="", required=False)
  parser.add_argument("-s", "--symradar-prefix", type=str, help="SymRadar prefix", default="", required=False)
  parser.add_argument("--snapshot-prefix", type=str, help="Snapshot prefix", default="", required=False)
  parser.add_argument("-a", "--additional", type=str, help="Additional arguments", default="", required=False)
  parser.add_argument("-m", "--mode", type=str, help="Mode", choices=["symradar", "extractfix"], default="symradar")
  parser.add_argument("-v", "--vulmaster", action="store_true", help="Run vulmaster", default=False)
  parser.add_argument("--seq", action="store_true", help="Run sequentially", default=False)
  args = parser.parse_args(argv)
  global OUTPUT_DIR, PREFIX, SYMRADAR_PREFIX, MODE, VULMASTER_MODE, SNAPSHOT_PREFIX
  VULMASTER_MODE = args.vulmaster
  MODE = args.mode
  SNAPSHOT_PREFIX = args.snapshot_prefix
  OUTPUT_DIR = os.path.join(ROOT_DIR, "out")
  if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR, exist_ok=True)
  if args.prefix != "":
    PREFIX = args.prefix
  else:
    PREFIX = time.strftime("%Y-%m-%d_%H-%M-%S", time.localtime())
  if args.cmd == "exp":
    if args.extra != "exp":
      SYMRADAR_PREFIX = args.extra
  if args.cmd == "filter":
    SYMRADAR_PREFIX = "filter"
  if args.cmd == "uc":
    SYMRADAR_PREFIX = "uc"
  if args.symradar_prefix != "":
    SYMRADAR_PREFIX = args.symradar_prefix
  meta_data = uni_klee.global_config.get_meta_data_list()
  if args.cmd == "final":
    final_analysis(meta_data, args.output)
    return
  if args.cmd == "clean":
    run_clean(uni_klee.global_config.get_meta_data_list())
    return
  with open(os.path.join(GLOBAL_LOG_DIR, "time.log"), "a") as f:
    f.write(f"\n#{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
  log_out(f"Total meta data: {len(meta_data)}")
  if args.seq:
    run_cmd_seq(args.cmd, meta_data, args.extra, args.additional, args.output)
  else:
    run_cmd(args.cmd, meta_data, args.extra, args.additional)

if __name__ == "__main__":
  main(sys.argv[1:])