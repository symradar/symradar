#!/usr/bin/env python3
import os
import sys
import json
import struct


def find(data, key) -> dict:
  for mo in data["objects"]:
    if key == mo["name"]:
       return mo
  return None

def main(args: list):
  filename = args[1]
  with open(filename, 'r') as f:
    data = json.load(f)
  exe_env = find(data, "__exe_env")
  format = "<iIQQ"
  for i in range(32):
    data = exe_env["data"][i*24*2:(i+1)*24*2]
    result = struct.unpack(format, bytes.fromhex(data))
    print(f"fd: {result[0]}, flags: {result[1]}, offset: {result[2]}, dfile: {result[3]}")


if __name__ == "__main__":
  main(sys.argv)