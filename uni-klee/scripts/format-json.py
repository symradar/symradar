#!/usr/bin/env python3
import os
import sys
import json

filename = sys.argv[1]
with open(filename, 'r') as f:
    data = json.load(f)
if filename.endswith('.json'):
    filename = filename[:-5]
new_filename = filename + '-new.json'
with open(new_filename, 'w') as f:
    json.dump(data, f, indent=2)
print(f"{filename} -> {new_filename}")