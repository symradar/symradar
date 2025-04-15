#!/usr/bin/env python3
import sys
import os
import re
from typing import List, Set, Dict, Tuple
import json
import toml
import multiprocessing as mp

UNI_KLEE_RUNTIME = ""
UNI_KLEE_RUNTIME_H = ""

def formula_to_code(formula: str, concrete_range: list, vars: List[str]) -> List[str]:
  variable_names = set(re.findall(r'\b([a-zA-Z_]\w*)\b', formula))
  code = ""
  for i in range(len(vars)):
    code += f"  long long {vars[i]} = rvals[{i}];\n"
  result = list()
  if len(concrete_range) == 0:
    code += f"  result = {formula};\n"
    result.append(code)
    return result
  for i in concrete_range:
    tmp = f"{code}  long long constant_a = {i};\n"
    tmp += f"  result = {formula};\n"
    result.append(tmp)
  return result

def get_basics(vars: List[str]) -> str:
  code = ""
  for i in range(len(vars)):
    code += f"  long long {vars[i]} = rvals[{i}];\n"
  code += "  long long constant_a;\n"
  return code

def get_concrete(formula: str, concrete_range: list) -> List[str]:
  result = list()
  if len(concrete_range) == 0:
    code = f"  result = {formula};\n"
    result.append(code)
  for i in concrete_range:
    tmp = f"  constant_a = {i};\n"
    tmp += f"  result = {formula};\n"
    result.append(tmp)
  return result

def write_meta_program(meta_program: list, conc_dir: str):
  print(f"Writing meta program to {conc_dir}")
  os.makedirs(conc_dir, exist_ok=True)
  base_code = meta_program["base"]
  patches = meta_program["patches"]
  lines = UNI_KLEE_RUNTIME.splitlines()
  codes = list()
  codes.append(f"  int patch_results[{len(patches)}];\n")
  for patch in patches:
    codes.append(f"  // Patch {patch['name']} # {patch['id']}\n")
    codes.append(patch['code'])
    codes.append(f"  uni_klee_add_patch(patch_results, {patch['id']}, result);\n")
  contents = list()
  for line in lines:
    if "// REPLACE" in line:
      contents.append(base_code)
      contents.extend(codes)
    else:
      contents.append(line + "\n")
  with open(f"{conc_dir}/uni_klee_runtime_new.c", "w") as f:
    f.writelines(contents)
  with open(f"{conc_dir}/uni_klee_runtime.h", "w") as f:
    f.write(UNI_KLEE_RUNTIME_H)
  compile(conc_dir)

def to_meta_program(patch_list: list, meta: dict) -> dict:
  base_code = get_basics(meta["vars"])
  patches = list()
  id = 1
  meta_program = { "base": base_code, "patches": patches }
  if "buggy" in meta:
    formula = meta["buggy"]["code"]
    obj = { "name": "buggy", "id": 0, "num": 0, "local_id": 0, "code": f"  result = {formula};\n" }
    patches.append(obj)
  for patch in patch_list:
    remove_zero = False
    if "/ constant_a" in patch["patch"]:
      remove_zero = True
    concrete_range = list()
    if "Partition" in patch:
      for part in patch["Partition"]:
        if "Range" in part:
          range_str = part["Range"]
          start, con, end = range_str.split("<=")
          for i in range(int(start), int(end) + 1):
            if remove_zero and i == 0:
              print(f"Skip zero: {patch['num']}")
              continue
            concrete_range.append(i)
    concretes = get_concrete(patch["patch"], concrete_range)
    local_id = 0
    for conc in concretes:
      obj = { "name": f"{patch['num']}-{local_id}", "id": id, "num": patch["num"], 
             "local_id": local_id, "code": conc }
      id += 1
      local_id += 1
      patches.append(obj)
  if "correct" in meta:
    formula = meta["correct"]["code"]
    obj = { "name": "correct", "id": id, "num": 0, "local_id": 1, "code": f"  result = {formula};\n" }
    patches.append(obj)
  return meta_program


def to_concrete_patch(patch: dict, meta: dict) -> dict:
  num = patch["num"]
  lid = patch["lid"]
  formula = patch["patch"]
  result = { "num": num, "lid": lid, "patch": formula }
  concrete_range = list()
  if "Partition" in patch:
    for part in patch["Partition"]:
      if "Range" in part:
        range_str = part["Range"]
        start, con, end = range_str.split("<=")
        for i in range(int(start), int(end) + 1):
          concrete_range.append(i)
  code = formula_to_code(formula, concrete_range, meta["vars"])
  result["codes"] = code
  return result

def apply_patch_to_file(outdir, code):
  lines = UNI_KLEE_RUNTIME.splitlines()
  os.system(f"rm -rf {outdir}")
  os.makedirs(outdir, exist_ok=True)
  contents = list()
  print(lines)
  for line in lines:
    if "// REPLACE" in line:
      contents.append(code)
    else:
      contents.append(line + "\n")
  with open(os.path.join(outdir, "uni_klee_runtime_new.c"), "w") as f:
    f.writelines(contents)
  with open(os.path.join(outdir, "uni_klee_runtime.h"), "w") as f:
    f.write(UNI_KLEE_RUNTIME_H)

def save_to_file(dir: str, patches: list):
  for patch in patches:
    patch_no = patch["num"]
    codes = patch["codes"]
    for i in range(len(codes)):
      patch_id = f"{patch_no}-{i}"
      outdir = os.path.join(dir, patch_id)
      os.makedirs(outdir, exist_ok=True)
      apply_patch_to_file(outdir, codes[i])

def lazy_compile(dir: str, cmd: str, file_a: str, file_b: str):
  cwd = os.getcwd()
  os.chdir(dir)
  if os.path.exists(file_b):
    if os.path.getmtime(file_a) <= os.path.getmtime(file_b):
      os.chdir(cwd)
      # print(f"Skip {cmd}")
      return
  print(f"Run {cmd}")
  os.system(f"{cmd}")
  os.chdir(cwd)

def compile(dir: str):
  KLEE_INCLUDE_PATH = "/root/projects/uni-klee/include"
  cmd = f"wllvm -g -fPIC -O0 -c -o uni_klee_runtime_new.o uni_klee_runtime_new.c -I{KLEE_INCLUDE_PATH}"
  lazy_compile(dir, cmd, "uni_klee_runtime_new.c", "uni_klee_runtime_new.o")
  cmd = "llvm-ar rcs libuni_klee_runtime_new.a uni_klee_runtime_new.o"
  lazy_compile(dir, cmd, "uni_klee_runtime_new.o", "libuni_klee_runtime_new.a")
  cmd = "extract-bc libuni_klee_runtime_new.a"
  lazy_compile(dir, cmd, "libuni_klee_runtime_new.a", "libuni_klee_runtime_new.bca")
  cmd = "wllvm -fPIC -shared -o libcpr_runtime_new.so uni_klee_runtime_new.o"
  lazy_compile(dir, cmd, "uni_klee_runtime_new.o", "libcpr_runtime_new.so")
  
  if not os.path.exists(os.path.join(dir, "uni_klee_runtime_vulmaster.c")):
    print(f"\nWARNING!!! {dir}/uni_klee_runtime_vulmaster.c does not exist\n", file=sys.stderr)
    return
  cmd = "wllvm -g -fPIC -O0 -c -o uni_klee_runtime_vulmaster.o uni_klee_runtime_vulmaster.c -I{KLEE_INCLUDE_PATH}"
  lazy_compile(dir, cmd, "uni_klee_runtime_vulmaster.c", "uni_klee_runtime_vulmaster.o")
  cmd = "llvm-ar rcs libuni_klee_runtime_vulmaster.a uni_klee_runtime_vulmaster.o"
  lazy_compile(dir, cmd, "uni_klee_runtime_vulmaster.o", "libuni_klee_runtime_vulmaster.a")
  cmd = "extract-bc libuni_klee_runtime_vulmaster.a"
  lazy_compile(dir, cmd, "libuni_klee_runtime_vulmaster.a", "libuni_klee_runtime_vulmaster.bca")
  cmd = "wllvm -fPIC -shared -o libcpr_runtime_vulmaster.so uni_klee_runtime_vulmaster.o"
  lazy_compile(dir, cmd, "uni_klee_runtime_vulmaster.o", "libcpr_runtime_vulmaster.so")
  
  
def move_files(meta_data: dict, experiments: str, patches: str):
  for meta in meta_data:
    bug_id = meta["bug_id"]
    benchmark = meta["benchmark"]
    subject = meta["subject"]
    exp_dir = os.path.join(experiments, benchmark, subject, bug_id)
    pat_dir = os.path.join(patches, benchmark, subject, bug_id)
    if os.path.exists(f'{exp_dir}/results.tar.gz'):
      print(f"tar -xzf {exp_dir}/results.tar.gz -C {exp_dir}")
      os.system(f"tar -xzf {exp_dir}/results.tar.gz -C {exp_dir}")
      print(f"cp {exp_dir}/results/output/patch-set-ranked {pat_dir}/patch-set-ranked")
      os.system(f"cp {exp_dir}/results/output/patch-set-ranked {pat_dir}/patch-set-ranked")

def search_files(meta_data: dict, patches: str):
  for meta in meta_data:
    bug_id = meta["bug_id"]
    benchmark = meta["benchmark"]
    subject = meta["subject"]
    id = meta["id"]
    pat_dir = os.path.join(patches, benchmark, subject, bug_id)
    if not os.path.exists(f'{pat_dir}/patch-set-ranked'):
      print(f"Patch file does not exist: id ({id}) {pat_dir}/patch-set-ranked")


def get_abstract_patches(patch_file: str) -> list:
  # option == concrete
  if not os.path.exists(patch_file):
    print(f"Patch file does not exist: {patch_file}")
    return []
  with open(patch_file, "r") as f:
    lines = f.readlines()
  patch_num = 0
  patch_list = list()
  pattern = r'^L?\d+'
  patch = dict()
  partition_indent = 0
  partition = dict()
  for line in lines:
    indent = len(line) - len(line.lstrip())
    line = line.strip()
    if line.startswith("Patch #"):
      patch_num = int(line.split("#")[1])
      patch = dict()
      patch_list.append(patch)
      patch["num"] = patch_num
    elif bool(re.search(pattern, line)):
      patch["patch"] = line.split(":")[1].strip()
      patch["lid"] = line.split(":")[0].strip()
    elif line.startswith("Partition: "):
      partition_indent = indent
      if "Partition" not in patch:
        patch["Partition"] = list()
      partition = dict()
      patch["Partition"].append(partition)
      partition["id"] = int(line.split(":")[1].strip())
    elif indent > partition_indent:
      key, val = line.split(":")
      partition[key.strip()] = val.strip()
    else:
      key, val = line.split(":")
      patch[key.strip()] = val.strip()
  return patch_list
  # with open(f"{outdir}/abs-patches.json", "w") as f:
  #   print(f"Writing to {outdir}/abs-patches.json")
  #   json.dump(patch_list, f, indent=2)

def main(args: List[str]):
  if len(args) != 3:
    print("Usage: patch.py <opt> <patch-dir>")
    sys.exit(1)
  opt_map = {
    "init": "Initialize the patch directory",
    "single": "Compile a single patch",
    "compile": "Compile all patches",
    "concrete": "Generate concrete patches",
    "buggy": "Generate buggy patches from meta",
    "meta": "Generate patches as meta program",
    "reset": "Remove concrete/",
  }
  root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
  opt = args[1]
  if opt not in opt_map:
    print("Invalid option")
    for k, v in opt_map.items():
      print(f"  {k}: {v}")
    sys.exit(1)
  global UNI_KLEE_RUNTIME, UNI_KLEE_RUNTIME_H
  with open(os.path.join(root_dir, "lib", "uni_klee_runtime_new.c"), "r") as f:
    UNI_KLEE_RUNTIME = f.read()
  with open(os.path.join(root_dir, "lib", "uni_klee_runtime.h"), "r") as f:
    UNI_KLEE_RUNTIME_H = f.read()
  patch_dir = args[2]
  pool = mp.Pool(mp.cpu_count() * 2 // 3)
  if opt == "single":
    compile(patch_dir)
    sys.exit(0)
  with open(os.path.join(patch_dir, "meta-data.toml"), "r") as f:
    data = toml.load(f)
  meta_data = data["meta_data"]
  # search_files(meta_data, patch_dir)
  # exit(0)
  for meta in meta_data:
    bug_id = meta["bug_id"]
    benchmark = meta["benchmark"]
    subject = meta["subject"]
    outdir = os.path.join(patch_dir, benchmark, subject, bug_id)
    if not os.path.exists(outdir + "/patch-set-ranked"):
      continue
    if "vars" not in meta:
      continue
    if opt == "reset":
      os.system(f"rm -rf {outdir}/concrete")
      continue
    if opt == "compile":
      compile(f"{outdir}/concrete")
      continue
    vars = meta["vars"]
    if opt == "meta":
      patch_list = get_abstract_patches(f"{outdir}/patch-set-gen")
      meta_program = to_meta_program(patch_list, meta)
      # with open(f"{outdir}/meta-program-original.json", "w") as f:
      #   print(f"Writing to {outdir}/meta-program-original.json")
      #   json.dump(meta_program, f, indent=2)
      #   continue
      write_meta_program(meta_program, os.path.join(outdir, "concrete"))
      # with open(f"{outdir}/meta-program.json", "w") as f:
      #   print(f"Writing to {outdir}/meta-program.json")
      #   json.dump(meta_program, f, indent=2)

if __name__ == "__main__":
  main(sys.argv)