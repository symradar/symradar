#! /usr/bin/env python3
import os
import sys
from typing import List, Set, Dict, Tuple
import json
import subprocess
import argparse
import patch

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
class ConfigFiles:
  root_dir: str
  meta_data: str
  repair_conf: str
  meta_program: str
  patch_dir: str
  work_dir: str
  project_dir: str
  meta_patch_obj_file: str
  benchmark: str
  subject: str
  bid: str
  out_base_dir: str
  out_dir: str
  
  def __init__(self):
    self.root_dir = ROOT_DIR
    self.meta_data = os.path.join(self.root_dir, "patches", "meta-data.json")
  def set(self, bug_info: dict):
    patches_dir = os.path.join(self.root_dir, "patches")
    self.bid = bug_info["bug_id"]
    self.benchmark = bug_info["benchmark"]
    self.subject = bug_info["subject"]
    self.project_dir = os.path.join(patches_dir, self.benchmark, self.subject, self.bid)
    self.work_dir = os.path.join(self.project_dir, "patched")
    self.repair_conf = os.path.join(self.project_dir, "repair.conf")
    self.meta_program = os.path.join(self.project_dir, "meta-program.json")
    self.meta_patch_obj_file = os.path.join(self.project_dir, "concrete", "libuni_klee_runtime.bca")
  def set_out_dir(self, out_dir: str, out_dir_prefix: str):
    if out_dir == "":
      self.out_base_dir = os.path.join(self.root_dir, "out", self.benchmark, self.subject, self.bid)
      self.out_dir = os.path.join(self.out_base_dir)
    elif out_dir == "out":
      self.out_base_dir = self.work_dir
    else:
      self.out_base_dir = out_dir
  
  
  def read_conf_file(self) -> dict:
    with open(self.repair_conf, "r") as f:
      lines = f.readlines()
    result = dict()
    for line in lines:
      line = line.strip()
      if len(line) == 0:
        continue
      if line.startswith("#"):
        continue
      key, value = line.split(":", 1)
      result[key] = value
    return result
  def read_meta_data(self) -> dict:
    with open(self.meta_data, "r") as f:
      return json.load(f)
  def read_meta_program(self) -> dict:
    with open(self.meta_program, "r") as f:
      return json.load(f)

class Config:
  cmd: str
  patch_ids: List[str]
  meta: dict
  bug_info: dict
  query: str
  debug: bool
  outdir: str
  workdir: str
  project_conf: dict
  conf_files: ConfigFiles
  
  def get_bug_info(self, bugid: str) -> dict:
    def check_int(s: str) -> bool:
      try:
        int(s)
        return True
      except ValueError:
        return False
    num = -1
    if check_int(bugid):
      num = int(bugid)
    for data in self.meta:
      bid = data["bug_id"]
      benchmark = data["benchmark"]
      subject = data["subject"]
      id = data["id"]
      if num != -1:
        if num == id:
          return data
      if bugid.lower() in bid.lower():
        return data
    return None
  
  def get_patch_ids(self, patch_ids: list) -> List[str]:
    meta_program = self.conf_files.read_meta_program()
    result = list()
    for patch in meta_program["patches"]:
      if str(patch["id"]) in patch_ids:
        result.append(str(patch["id"]))
      elif patch["name"] in patch_ids:
        result.append(str(patch["id"]))
    return result
  
  def parse_query(self) -> Tuple[dict, list]:
    parsed: List[str] = None
    if ":" in self.query:
      parsed = self.query.rsplit(":", 1)
    else:
      parsed = self.query.rsplit("/", 1)
    bugid = parsed[0]
    self.bug_info = self.get_bug_info(bugid)
    if self.bug_info is None:
      print(f"Cannot find patch for {self.query} - {bugid}")
      sys.exit(1)
    # Set config files
    self.conf_files.set(self.bug_info)
    # Set patch ids
    patchid = "buggy"
    if len(parsed) > 1:
      patchid = parsed[1]
    self.patch_ids = self.get_patch_ids(patchid.split(","))
    print(f"query: {self.query} => bugid {self.bug_info}, patchid {self.patch_ids}")
    
  def init(self):
    self.meta = self.conf_files.read_meta_data()
    self.parse_query()
  
  def __init__(self, cmd: str, query: str, debug: bool):
    self.cmd = cmd
    self.query = query    
    self.debug = debug
    self.conf_files = ConfigFiles()
  
  @staticmethod  
  def parser(argv: List[str]) -> 'Config':
    parser = argparse.ArgumentParser(description="Test script for uni-klee")
    parser.add_argument("cmd", help="Command to execute", choices=["run", "cmp", "fork", "snapshot", "batch", "filter"])
    parser.add_argument("query", help="Query to execute")
    parser.add_argument("-a", "--additional", help="Additional arguments")
    parser.add_argument("-d", "--debug", help="Debug mode", action="store_true")
    parser.add_argument("-o", "--outdir", help="Output directory", default="")
    parser.add_argument("-p", "--outdir-prefix", help="Output directory prefix", default="uni-m-out")
    args = parser.parse_args(argv)
    conf = Config(args.cmd, args.query, args.debug)
    conf.init()
    conf.conf_files.set_out_dir(args.outdir, args.outdir_prefix)
    return conf


def select_from_meta(meta: dict, query: str) -> dict:
  def check_int(s: str) -> bool:
    try:
      int(s)
      return True
    except ValueError:
      return False
  num = -1
  if check_int(query):
    num = int(query)
  for data in meta:
    bid = data["bug_id"]
    benchmark = data["benchmark"]
    subject = data["subject"]
    id = data["id"]
    if num != -1:
      if num == id:
        return data
    if query.lower() in bid.lower():
      return data
  return None

def read_conf_file(conf_file: str) -> dict:
  with open(conf_file, "r") as f:
    lines = f.readlines()
  result = dict()
  for line in lines:
    line = line.strip()
    if len(line) == 0:
      continue
    if line.startswith("#"):
      continue
    key, value = line.split(":", 1)
    result[key] = value
  return result

def find_num(dir: str, name: str) -> int:
  result = 0
  dirs = os.listdir(dir)
  while True:
    if f"{name}-{result}" in dirs:
      result += 1
    else:
      break
  return result

def execute(cmd: str, dir: str, env: dict = None) -> int:
  print(f"Change directory to {dir}")
  print(f"Executing: {cmd}")
  if env is None:
    env = os.environ
  proc = subprocess.run(cmd, shell=True, cwd=dir, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  print(f"Exit code: {proc.returncode}")
  return proc.returncode

def init(root_dir: str, bug_info: dict, reset: bool = False) -> None:
  bug_dir = os.path.join(root_dir, "patches", bug_info["benchmark"], bug_info["subject"], bug_info["bug_id"])
  if reset or not os.path.exists(os.path.join(bug_dir, "patched")):
    execute("./init.sh", bug_dir)

def get_meta_program(file_name: str, patch_ids: list) -> str:
  with open(file_name, "r") as f:
    meta_program = json.load(f)
  result = list()
  for patch in meta_program["patches"]:
    if str(patch["id"]) in patch_ids:
      result.append(str(patch["id"]))
    elif patch["name"] in patch_ids:
      result.append(str(patch["id"]))
  return ",".join(result)

def run(root_dir: str, bug_info: dict, patchid: str, outdir: str, cmd: str, additional: dict):
  bid = bug_info["bug_id"]
  benchmark = bug_info["benchmark"]
  subject = bug_info["subject"]
  id = bug_info["id"]
  target_function = bug_info["target"]
  snapshot_file = "snapshot-last.json"
  if "snapshot" in bug_info:
    snapshot_file = bug_info["snapshot"]
  subdir = os.path.join(benchmark, subject, bid)
  output_dir = os.path.join(outdir, subdir, patchid)
  os.makedirs(output_dir, exist_ok=True)
  patches = os.path.join(root_dir, "patches")
  meta_program_json = os.path.join(patches, subdir, "meta-program.json")
  if not os.path.exists(meta_program_json):
    print(f"Cannot find {meta_program_json}")
    return
  patch_ids = get_meta_program(meta_program_json, patchid.split(","))
  patch_dir = os.path.join(patches, subdir, "concrete")
  patch.compile(patch_dir)
  patch_file = os.path.join(patch_dir, "libuni_klee_runtime.bca")
  repair_conf_file = os.path.join(patches, subdir, "repair.conf")
  file_check = [patch_file, repair_conf_file]
  for file in file_check:
    if not os.path.exists(file):
      print(f"Cannot find {file}")
      return
  conf = read_conf_file(repair_conf_file)
  no = find_num(output_dir, "uni-m-out")
  snapshot_dir = os.path.join(output_dir, "snapshot")
  uni_out_dir = os.path.join(output_dir, f"uni-m-out-{no}")
  SNAPSHOT_DEFAULT_OPTS = f"--patch-id={patch_ids} --output-dir={snapshot_dir} --dump-states-on-halt=0 --write-smt2s --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --log-trace --target-function={target_function}"
  UNI_KLEE_DEFAULT_OPTS = f"--patch-id={patch_ids} --output-dir={uni_out_dir} --dump-states-on-halt=0 --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --no-exit-on-error --dump-snapshot --log-trace --simplify-sym-indices --make-lazy --target-function={target_function} --snapshot={snapshot_dir}/{snapshot_file}"
  if cmd != "fork":
    UNI_KLEE_DEFAULT_OPTS += " --start-from-snapshot"
  link_opts = f"--link-llvm-lib={patch_file}"
  if "klee_flags" in conf:
    link_opts += f" {conf['klee_flags']}"
  data_dir = os.path.join(patches, subdir, "patched")
  bin_file = os.path.basename(conf["binary_path"])
  # data_dir = os.path.join(root_dir, "data", subdir)
  # if cmd == "init" or not os.path.exists(os.path.join(data_dir, target)):
  #   init(data_dir, conf["src_directory"], conf["binary_path"])
  poc_path = "exploit"
  if "poc_path" in conf:
    poc_path = os.path.basename(conf["poc_path"])
  target = os.path.join(bin_file + ".bc")
  test_cmd = f"{target} "
  if "test_input_list" in conf:
    test_cmd += conf['test_input_list'].replace("$POC", poc_path)
  if cmd == "snapshot":
    os.system(f"rm -rf {snapshot_dir}")
  if not os.path.exists(snapshot_dir):
    execute(f"uni-klee {link_opts} {SNAPSHOT_DEFAULT_OPTS} {test_cmd}", data_dir)
  if cmd in ["run", "all", "fork"]:
    execute(f"uni-klee {link_opts} {UNI_KLEE_DEFAULT_OPTS} {test_cmd}", data_dir)
  elif cmd in ["cmp"]:
    new_patch_id = additional["patch"]
    patch_ids = get_meta_program(meta_program_json, new_patch_id.split(","))
    new_output_dir = os.path.join(outdir, subdir, new_patch_id)
    os.makedirs(new_output_dir, exist_ok=True)
    new_no = find_num(new_output_dir, "uni-cmp-out")
    new_snapshot_dir = os.path.join(new_output_dir, "snapshot")
    new_uni_out_dir = os.path.join(new_output_dir, f"uni-cmp-out-{new_no}")
    NEW_UNI_KLEE_DEFAULT_OPTS = f"--patch-id={patch_ids} {link_opts} --make-lazy --output-dir={new_uni_out_dir} --write-smt2s --write-kqueries --libc=uclibc --allocate-determ --posix-runtime --external-calls=all --no-exit-on-error --dump-snapshot --log-trace --simplify-sym-indices --target-function={target_function} --snapshot={snapshot_dir}/{snapshot_file}"
    execute(f"uni-klee {NEW_UNI_KLEE_DEFAULT_OPTS} {test_cmd}", data_dir)

def filter_patch(root_dir: str, bug_info: dict):
  bid = bug_info["bug_id"]
  benchmark = bug_info["benchmark"]
  subject = bug_info["subject"]
  subdir = os.path.join(benchmark, subject, bid)
  patches = os.path.join(root_dir, "patches")
  data_dir = os.path.join(patches, subdir, "patched")
  repair_conf_file = os.path.join(patches, subdir, "repair.conf")
  file_check = [repair_conf_file]
  for file in file_check:
    if not os.path.exists(file):
      print(f"Cannot find {file}")
      sys.exit(1)
  conf = read_conf_file(repair_conf_file)
  bin_file = os.path.basename(conf["binary_path"])
  failed_patches = list()
  success_patches = list()
  for patchid in os.listdir(os.path.join(patches, "concrete", subdir)):
    patch_dir = os.path.join(patches, "concrete", subdir, patchid)
    if os.path.isdir(patch_dir):
      env = os.environ.copy()
      env["LD_LIBRARY_PATH"] = f"{patch_dir}:{os.path.join(root_dir, 'lib')}"
      print(f"Testing patch {env['LD_LIBRARY_PATH']}")
      poc_path = "exploit"
      if "poc_path" in conf:
        poc_path = os.path.basename(conf["poc_path"])
      cmd = f"./{bin_file} "
      if "test_input_list" in conf:
        cmd += conf['test_input_list'].replace("$POC", poc_path)
      res = execute(cmd, data_dir, env=env)
      if res != 0:
        print(f"Patch {patchid} failed!!!")
        failed_patches.append(patchid)
      else:
        print(f"Patch {patchid} succeeded")
        success_patches.append(patchid)
  print(f"Failed patches: {failed_patches}")
  print(f"Success patches: {success_patches}")
  obj = { "failed": failed_patches, "success": success_patches }
  with open(os.path.join(data_dir, "patch_result.json"), "w") as f:
    json.dump(obj, f, indent=2)

def batch_cmp(root_dir: str, bug_info: dict, out_dir: str):
  bid = bug_info["bug_id"]
  benchmark = bug_info["benchmark"]
  subject = bug_info["subject"]
  subdir = os.path.join(benchmark, subject, bid)
  patches = os.path.join(root_dir, "patches")
  data_dir = os.path.join(patches, subdir, "patched")
  repair_conf_file = os.path.join(patches, subdir, "repair.conf")
  file_check = [repair_conf_file]
  for file in file_check:
    if not os.path.exists(file):
      print(f"Cannot find {file}")
      sys.exit(1)
  conf = read_conf_file(repair_conf_file)
  run(root_dir, bug_info, "buggy", out_dir, "cmp", {"patch": "buggy,correct,1,2"})


def main(args: List[str]):
  root_dir = ROOT_DIR
  os.chdir(root_dir)
  if len(args) < 3:
    print(f"Usage: {args[0]} <cmd> <query>")
    print("cmd: run, cmp, snapshot, batch, filter")
    print("query: <bugid>:<patchid>")
    print("Ex) test.py run 5321:buggy")
    print("Ex) test.py cmp 5321:buggy 1-0")
    print("Ex) test.py snapshot 5321:buggy")
    sys.exit(1)
  cmd = args[1]
  query = args[2]
  additional = dict()
  if cmd == "cmp":
    if len(args) < 4:
      print(f"Usage: {args[0]} cmp <query> <patchid>")
      print("Ex) test.py cmp 5321:buggy 1-0")
      sys.exit(1)
    additional["patch"] = args[3]
  patches = os.path.join(root_dir, "patches")
  outdir = os.path.join(root_dir, "out")
  print(f"outdir: {outdir}")
  with open(f"{patches}/meta-data.json", "r") as f:
    data = json.load(f)
  if cmd == "batch":
    for meta in data:
      if meta["benchmark"] != "extractfix":
        continue
      if query == meta["subject"]:
        bug_info = meta
        init(root_dir, bug_info)
        print(f"Running batch for {bug_info['subject']}/{bug_info['bug_id']}")
        batch_cmp(root_dir, bug_info, outdir)
    sys.exit(0)
  if ":" in query:
    parsed = query.rsplit(":", 1)
  else:
    parsed = query.rsplit("/", 1)
  bugid = parsed[0]
  if len(parsed) < 2:
    patchid = "buggy"
  else:
    patchid = parsed[1]
  bug_info = select_from_meta(data, bugid)
  if bug_info is None:
    print(f"Cannot find patch for {query} - {bugid}")
    sys.exit(1)
  print(f"query: {query} => bugid {bugid}, patchid {patchid}")
  if cmd == "batch":
    batch_cmp(root_dir, bug_info, outdir)
    # for bug_info in data:
    #   if "buggy" not in bug_info:
    #     continue
    #   additional = {"patch": bug_info["correct"]["id"]}
    #   run(root_dir, bug_info, "buggy", outdir, "cmp", additional)
  if cmd == "filter":
    filter_patch(root_dir, bug_info)
  else:
    run(root_dir, bug_info, patchid, outdir, cmd, additional)


if __name__ == "__main__":
  main(sys.argv)