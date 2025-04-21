import json
import os
from typing import Dict


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

final_result:Dict[str,Dict[str,int]]=dict() # Each result for each subject
final_total_patches=0 # Total patches
final_safe=0 # Total correct patches that Spider identified as safe
final_unsafe=0 # Total incorrect patches that Spider identified as unsafe

for subject in CORRECT_PATCHES:
    if not os.path.exists(f'{subject}/output/results.json'):
        continue
    print(f'{subject}')
    with open(f'{subject}/output/results.json','r') as f:
        results=json.load(f)

    safe=0
    unsafe=0
    for _id, result in results.items():
            if result:
                # Correct patch, and the tool says it is correct
                safe+=1
                final_safe+=1
            else:
                # Correct patch, but the tool says it is incorrect
                unsafe+=1
                final_unsafe+=1

    total_patches=len(results)
    final_total_patches+=total_patches

    final_result[subject]={
        'safe':safe, # Spider identified the safe
        'unsafe':unsafe, # Spider identified the unsafe
        'total_patches':total_patches,
    }

with open('final_result.json','w') as f:
    json.dump({
        'result_each_subject':final_result,
        'total_patches':final_total_patches,
        'result_overall':{
            'safe':final_safe,
            'unsafe':final_unsafe
        }
    },f,indent=4)

with open('final_result.csv','w') as f:
    print('Subject,Safe,Unsafe,Total Patches,',file=f)
    for subject in final_result:
        print(f'{subject},{final_result[subject]["safe"]},{final_result[subject]["unsafe"]},'+\
              f'{final_result[subject]["total_patches"]},',file=f)