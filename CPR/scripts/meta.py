#! /usr/bin/env python3
import os
import sys
from typing import List, Set, Dict, Tuple
import json
import re

def parse_setup(setup: str, dat: dict) -> dict:
  with open(setup, "r") as f:
    lines = f.readlines()
  print(f"setup: {setup}")
  for line in lines:
    line = line.strip()
    if line.startswith("#"):
      continue
    if "__cpr_choice" in line:
      print(line)
      pattern = r'\(char\*\[\]\)\{([^}]+)\}'
      vars = re.search(pattern, line)
      if vars:
        vars = vars.group(1).replace('"', '').split(",")
        print(vars)
      final = list()
      for var in vars:
        final.append(var.strip())
      dat["vars"] = final
      break
  print("==============END============")
  return dat

def main(args: List[str]):
  experiments = args[1]
  with open(f"{experiments}/meta-data", "r") as f:
    data = json.load(f)
  new_data = list()
  for dat in data:
    bid = dat["bug_id"]
    benchmark = dat["benchmark"]
    subject = dat["subject"]
    id = dat["id"]
    exp_dir = f"{experiments}/{benchmark}/{subject}/{bid}"
    result_file = f"{exp_dir}/results.tar.gz"
    # os.system(f"tar -xzf {result_file} -C {exp_dir}")
    setup_file = f"{exp_dir}/setup.sh"
    new_data.append(parse_setup(setup_file, dat))
  print(f"Dump to {experiments}/meta-data.json")
  with open(f"{experiments}/meta-data.json", "w") as f:
    json.dump(new_data, f, indent=2)    


main(sys.argv)