import subprocess
import os
import json
from typing import Dict
import multiprocessing as mp
import sys

def run(project:str, version:str):
    cur_subject=f'{project}/{version}'
    print(f'Processing {cur_subject}')
    os.makedirs(f'{cur_subject}/output',exist_ok=True)

    log_file=open(f'{cur_subject}/output/log.txt','w')
    results:Dict[str,bool]=dict()
    for patch in os.listdir(f'{cur_subject}/patches'):
        if patch.startswith('preprocessed'): continue
        patch_file=f'{cur_subject}/patches/{patch}'
        old_file=f'{cur_subject}/{list(filter(lambda x: x.startswith("buggy-"), os.listdir(cur_subject)))[0]}'
        res=subprocess.run(['java','-jar','../../executable.jar','-verbose','-output',f'{cur_subject}/output/.temp.json',
                            f'{old_file}',f'{patch_file}'],stdout=log_file,stderr=log_file)
        if res.returncode!=0:
            print(f'Error in {patch_file}')
            print(f'Error in {patch_file}',file=log_file)
            results[patch]=False
        else:
            with open(f'{cur_subject}/output/.temp.json','r') as f:
                data=json.load(f)
            try:
                results[int(patch.split('-')[0])]=data[0]['security']
            except:
                # print(f'Terrible output in {patch_file}')
                pass
    log_file.close()
    with open(f'{cur_subject}/output/results.json','w') as f:
        json.dump(results,f,indent=4)
    print(f'Finished processing {cur_subject}')

pool=mp.Pool(int(sys.argv[1]))

for project in os.listdir('.'):
    if os.path.isdir(project):
        for version in os.listdir(project):
            if os.path.isdir(f'{project}/{version}'):
                pool.apply_async(run,(project,version,))
                # run(project,version)

pool.close()
pool.join()