#!/usr/bin/env python3
import os
ROOTDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def filter_target(file: str) -> bool:
  dirs = ["include", "lib", "tools"]
  exts = [".h", ".hpp", ".c", ".cpp", ".cc", ".cxx"]
  result = False
  for dir in dirs:
    if file.startswith(dir):
      result = True
      break
  if not result:
    return False
  result = False
  for ext in exts:
    if file.endswith(ext):
      result = True
      break
  return result

def main():
  os.chdir(ROOTDIR)
  # Get the list of files changed in the current commit
  files = os.popen("git diff --name-only HEAD").read().splitlines()
  # Filter out files
  files = list(filter(filter_target, files))
  tot = len(files)
  i = 0
  for file in files:
    i += 1
    # Run clang-format on the file
    print(f"[{i}/{tot}] Running clang-format on {file}...")
    os.system(f"clang-format --style=file -i {file}")

if __name__ == '__main__':
  main()