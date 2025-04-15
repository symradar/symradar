import json
import subprocess
import multiprocessing as mp
import sys
import os
from typing import Dict, List, Set, Tuple
import shutil

CORRECT_PATCHES={
    # 'binutils/CVE-2017-15025',
    # 'binutils/CVE-2018-10372',
    # 'coreutils/bugzilla-19784',
    # 'coreutils/bugzilla-26545',
    'coreutils/gnubug-25003':389,
    'coreutils/gnubug-25023':128,
    'jasper/CVE-2016-8691':260,
    # 'jasper/CVE-2016-9387',
    'libjpeg/CVE-2012-2806':261,
    'libjpeg/CVE-2017-15232':2195,
    # 'libjpeg/CVE-2018-14498',
    # 'libjpeg/CVE-2018-19664',
    # 'libtiff/bugzilla-2611',
    'libtiff/bugzilla-2633':131,
    # 'libtiff/CVE-2014-8128',
    # 'libtiff/CVE-2016-3186',
    # 'libtiff/CVE-2016-3623',
    # 'libtiff/CVE-2016-5314',
    'libtiff/CVE-2016-5321':175,
    # 'libtiff/CVE-2016-9273',
    'libtiff/CVE-2016-10094':257,
    # 'libtiff/CVE-2017-7595',
    'libtiff/CVE-2017-7601':188,
    'libxml2/CVE-2012-5134':261,
    # 'libxml2/CVE-2016-1834',
    'libxml2/CVE-2016-1838':389,
    # 'libxml2/CVE-2016-1839',
    # 'libxml2/CVE-2017-5969',
}

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
  
def run(sub:str):
    try:
        log_file=open(f'{sub}/dafl-test.log', 'w')
        inputs=os.listdir(f'{sub}/concrete-inputs')
        inputs=[x for x in inputs if os.path.isfile(os.path.join(f'{sub}/concrete-inputs', x))]
        if 'coreutils' in sub:
            cmd='./dafl-patched/bin < <exploit> '
        else:
            cmd='./dafl-patched/bin '
            if os.path.exists(f'{sub}/config'):
                with open(f'{sub}/config','r') as f:
                    for line in f:
                        if line.startswith('cmd='):
                            cmd+=' '.join(line[4:].strip().split())
            else:
                with open(f'{sub}/repair.conf','r') as f:
                    for line in f:
                        if line.startswith('test_input_list:'):
                            cmd+=' '.join(line[17:].strip().replace('$POC','<exploit>').split())

        # Run original program with inputs
        print(f'Running {sub} with {len(inputs)} inputs...')
        orig_returncode:Dict[str, int]=dict()
        orig_conditions:Dict[str, List[int]]=dict()
        env=os.environ.copy()
        env['LD_LIBRARY_PATH']=f'{os.getcwd()}/{sub}/dafl-src'
        env['DAFL_PATCH_ID']='0'
        env['DAFL_RESULT_FILE']=f'{os.getcwd()}/{sub}/dafl-condition.log'
        for input in inputs:
            input_path=os.path.join(os.getcwd(),sub,'concrete-inputs', input)
            if '.' in input:
                extension=input.split('.')[-1]
                temp_input_path=os.path.join(os.getcwd(),sub,'dafl-patched',f'input.{extension}')
            else:
                temp_input_path=os.path.join(os.getcwd(),sub,'dafl-patched','input')
            shutil.copy(input_path, temp_input_path)
            cur_cmd=cmd.replace('<exploit>', temp_input_path)

            print(cur_cmd,file=log_file)
            res=subprocess.run(cur_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=sub, env=env,shell=True)
            os.remove(temp_input_path)
            print(f'{input} returns {res.returncode} with patch 0',file=log_file)
            orig_returncode[input]=res.returncode
            if res.returncode != 0 and res.returncode != 1: # If return code is 1, it is not a vulnerability
                print(f'Program crashed with input {input}',file=log_file)
            else:
                print(f'Program executed successfully with input {input}',file=log_file)
            try:
                print(res.stderr.decode('utf-8'),file=log_file)
            except UnicodeDecodeError:
                print('Error decoding stderr',file=log_file)

            # Parse the condition at the location
            if os.path.exists(f'{os.getcwd()}/{sub}/dafl-condition.log'):
                with open(f'{os.getcwd()}/{sub}/dafl-condition.log', 'r') as f:
                    line=f.readline()
                    orig_conditions[input]=[int(x) for x in line.strip().split()]
                os.remove(f'{os.getcwd()}/{sub}/dafl-condition.log')
                print(f'Successfully parse original condition log for {input}: {orig_conditions[input]}',file=log_file)
            else:
                print(f'No original condition log for {input}',file=log_file)
                orig_conditions[input]=[]

        print(f'Original {sub} returns {len(list(filter(lambda x: orig_returncode[x] != 0 and orig_returncode[x] != 1, inputs)))} crashing inputs')
            
        # Run patched program with non-crashing inputs and compare branches, filter out if the patch crashes or covers different branches
        filtered_out_patches=set()
        all_patches, correct_patch = get_all_patches(f'{sub}/group-patches-original.json')
        print(f'Running {sub} with non-crashing inputs...')
        for id in all_patches:
            print(f'Running {sub} with non-crashing input with patch {id}...',file=log_file)
            for input in inputs:
                if orig_returncode[input] != 0 and orig_returncode[input] != 1:
                    continue
                env['DAFL_PATCH_ID']=str(id)
                input_path=os.path.join(os.getcwd(),sub,'concrete-inputs', input)
                if '.' in input:
                    extension=input.split('.')[-1]
                    temp_input_path=os.path.join(os.getcwd(),sub,'dafl-patched',f'input.{extension}')
                else:
                    temp_input_path=os.path.join(os.getcwd(),sub,'dafl-patched','input')
                shutil.copy(input_path, temp_input_path)
                cur_cmd=cmd.replace('<exploit>', temp_input_path)

                print(cur_cmd,file=log_file)
                res=subprocess.run(cur_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=sub, env=env,shell=True)
                os.remove(temp_input_path)
                print(f'{input} returns {res.returncode} with patch {id}',file=log_file)
                if res.returncode != 0 and res.returncode != 1:
                    # Filter out if the patch crashes
                    print(f'Program crashed with input {input} with patch {id}',file=log_file)
                    filtered_out_patches.add(id)
                    break

                if os.path.exists(f'{os.getcwd()}/{sub}/dafl-condition.log'):
                    with open(f'{os.getcwd()}/{sub}/dafl-condition.log', 'r') as f:
                        line=f.readline()
                        new_cond=[int(x) for x in line.strip().split()]
                    os.remove(f'{os.getcwd()}/{sub}/dafl-condition.log')
                    print(f'Successfully parse non-crashing condition log for {input}: {orig_conditions[input]}',file=log_file)
                else:
                    print(f'No non-crashing condition log for {input}',file=log_file)
                    new_cond=[]
                if new_cond!=orig_conditions[input]:
                    # Filter out if the patch covers different branches
                    print(f'Program covers different branches with input {input} with patch {id}',file=log_file)
                    print(f'Original condition: {orig_conditions[input]}',file=log_file)
                    print(f'New condition: {new_cond}',file=log_file)
                    filtered_out_patches.add(id)
                    break

        # Run patched program with crashing inputs, filter out if the patch still crashes
        print(f'Running {sub} with crashing inputs...')
        for id in all_patches:
            if id in filtered_out_patches:
                continue

            print(f'Running {sub} with crashing input with patch {id}...',file=log_file)
            for input in inputs:
                if orig_returncode[input] == 0 or orig_returncode[input] == 1:
                    continue
                env['DAFL_PATCH_ID']=str(id)
                input_path=os.path.join(os.getcwd(),sub,'concrete-inputs', input)
                if '.' in input:
                    extension=input.split('.')[-1]
                    temp_input_path=os.path.join(os.getcwd(),sub,'dafl-patched',f'input.{extension}')
                else:
                    temp_input_path=os.path.join(os.getcwd(),sub,'dafl-patched','input')
                shutil.copy(input_path, temp_input_path)
                cur_cmd=cmd.replace('<exploit>', temp_input_path)

                print(cur_cmd,file=log_file)
                res=subprocess.run(cur_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=sub, env=env,shell=True)
                os.remove(temp_input_path)
                print(f'{input} returns {res.returncode} with patch {id}',file=log_file)
                if res.returncode != 0 and res.returncode != 1 and orig_returncode[input] != res.returncode:
                    # Filter out if the patch still crashes and change the behavior
                    print(f'Program crashed with input {input} with patch {id}',file=log_file)
                    filtered_out_patches.add(id)
                    break
                if os.path.exists(f'{os.getcwd()}/{sub}/dafl-condition.log'):
                    os.remove(f'{os.getcwd()}/{sub}/dafl-condition.log')

        final_patches=all_patches-filtered_out_patches
        print(f'Final patches for {sub}: total: {len(all_patches)}, remained: {len(final_patches)}, remained: {list(final_patches)}')
        log_file.close()
    except Exception as e:
        import traceback
        traceback.print_exc(file=log_file)
        traceback.print_tb(e.__traceback__, file=log_file)
        log_file.close()
        print(f'Error running {sub}: {e}')

pool=mp.Pool(int(sys.argv[1]))
for sub in CORRECT_PATCHES.keys():
    pool.apply_async(run, args=(sub,))
pool.close()
pool.join()