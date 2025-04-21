#!/usr/bin/env python3
import os
import sys
import json
import bisect
import difflib

def addr_str_to_int(addr_str: str) -> int:
  return int.from_bytes(bytes.fromhex(addr_str), byteorder="little")

def compare(from_data, to_data):
  from_map = dict()
  to_map = dict()
  for mo in from_data["objects"]:
    addr = addr_str_to_int(mo["addr"])
    from_map[addr] = mo
  for mo in to_data["objects"]:
    addr = addr_str_to_int(mo["addr"])
    to_map[addr] = mo
  from_addr_set = set(from_map.keys())
  to_addr_set = set(to_map.keys())
  added = to_addr_set - from_addr_set
  removed = from_addr_set - to_addr_set
  common = from_addr_set & to_addr_set
  changed = 0
  access_change = 0
  for addr in common:
    if from_map[addr]["data"] != to_map[addr]["data"]:
      changed += 1
      print(f"Changed: {addr:016x} {from_map[addr]['name']}")
      "{from_map[addr]['data']} -> {to_map[addr]['data']}"
      if from_map[addr]["addr"] in from_data["readAccessMemoryGraph"]:
        access_change += 1
        print(f"Access changed: {addr:016x} {from_map[addr]['name']}")
  for addr in added:
    print(f"Added: {addr:016x} {to_map[addr]['name']} {to_map[addr]['data']}")
  for addr in removed:
    print(f"Removed: {addr:016x} {from_map[addr]['name']} {from_map[addr]['data']}")
  print(f"Added: {len(added)} / Removed: {len(removed)} / Common: {len(common)} / Changed: {changed}")
  for addr in from_data["readAccessMemoryGraph"]:
    address = addr_str_to_int(addr)
    if address in removed:
      print(f"Read access removed: {address:016x} {from_map[address]['name']}")
  for addr in from_data["writeAccessMemoryGraph"]:
    address = addr_str_to_int(addr)
    if address in removed:
      print(f"Write access removed: {address:016x} {from_map[address]['name']}")
  # trace = difflib.unified_diff(from_data["trace"], to_data["trace"])
  # for line in trace:
  #   print(line)
  
def main(args: list):
  if len(args) < 2:
    print("Usage: compare-snapshots.py <filename>")
    return
  from_file = args[1]
  to_file = args[2]
  with open(from_file) as f:
    from_data = json.load(f)
  with open(to_file) as f:
    to_data = json.load(f)
  compare(from_data, to_data)

if __name__ == '__main__':
  main(sys.argv)