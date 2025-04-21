import os
import sys
import multiprocessing


def run_joern((old_file, new_file, output_json, raw_output)):
    keyname = os.path.dirname(os.path.dirname(old_file))
    print "[*] Trying to run on:" + keyname
    base_cmd = "LD_LIBRARY_PATH=/home/machiry/projects/safe-patches/joern/lib java -Xmx6144m -jar /home/machiry/Desktop/statementloopchecker.jar"
    to_run_cmd = base_cmd + " " + old_file + " " + new_file + " " + output_json + " > " + raw_output + " 2>&1"
    if os.system(to_run_cmd):
        print "[-] Failed for:" + keyname
    else:
        print "[+] Success:" + keyname


def is_commit_single_file(commit_path):
    new_fol = os.path.join(commit_path, "new")
    return len(os.listdir(new_fol)) == 1


def get_commit_files(commit_path):
    new_fol = os.path.join(commit_path, "new")
    old_fol = os.path.join(commit_path, "old_latest")
    filen = os.listdir(new_fol)[0]
    return os.path.join(new_fol, filen), os.path.join(old_fol, filen)

cve_folder = sys.argv[1]
output_dir = sys.argv[2]
if not os.path.exists(output_dir):
    os.makedirs(output_dir)
target_cmds = []
for curr_dir in os.listdir(cve_folder):
    cve_full_path = os.path.join(cve_folder, curr_dir)
    raw_output_file = os.path.join(output_dir, curr_dir + ".raw")
    output_json_file = os.path.join(output_dir, curr_dir + ".json")
    if is_commit_single_file(cve_full_path):
        new_file, old_file = get_commit_files(cve_full_path)
        target_cmds.append((old_file, new_file, output_json_file, raw_output_file))

print "Running:" + str(len(target_cmds)) + " commands."
p = multiprocessing.Pool()
p.map(run_joern, target_cmds)
print "Finished running all command."
