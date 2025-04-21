import subprocess
import os
import json
from typing import Dict
import multiprocessing as mp
import sys

CORRECT_PATCHES=(
    'binutils/cve_2017_15025',
    'binutils/cve_2018_10372',
    'coreutils/bugzilla_19784',
    # 'coreutils/bugzilla_26545',
    'coreutils/gnubug_25003',
    'coreutils/gnubug_25023',
    'jasper/cve_2016_8691',
    # 'jasper/cve_2016_9387',
    'libjpeg/cve_2012_2806',
    # 'libjpeg/cve_2017_15232',
    # 'libjpeg/cve_2018_14498',
    'libjpeg/cve_2018_19664',
    'libtiff/bugzilla_2611',
    # 'libtiff/bugzilla_2633',
    # 'libtiff/cve_2014_8128',
    'libtiff/cve_2016_3186',
    # 'libtiff/cve_2016_3623',
    'libtiff/cve_2016_5314',
    # 'libtiff/cve_2016_5321',
    # 'libtiff/cve_2016_9273',
    'libtiff/cve_2016_10094',
    'libtiff/cve_2017_7595',
    # 'libtiff/cve_2017_7601',
    'libxml2/cve_2012_5134',
    # 'libxml2/cve_2016_1834',
    # 'libxml2/cve_2016_1838',
    'libxml2/cve_2016_1839',
    # 'libxml2/cve_2017_5969',
)

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
                results[int(patch.split('-')[-1].split('.')[0])]=data[0]['security']
            except:
                # print(f'Terrible output in {patch_file}')
                pass
    log_file.close()
    with open(f'{cur_subject}/output/results.json','w') as f:
        json.dump(results,f,indent=4)
    print(f'Finished processing {cur_subject}')

pool=mp.Pool(int(sys.argv[1]))

for s in CORRECT_PATCHES:
    pool.apply_async(run,(*s.split('/'),))
    # run(project,version)

pool.close()
pool.join()