import json
import os
from typing import Dict


CORRECT_PATCHES={
    'binutils/cve_2017_15025': [12,76,203],
    'binutils/cve_2018_10372': [72,],
    'coreutils/bugzilla_19784': [486,717,],
    'coreutils/bugzilla_26545': [1184,1858,],
    'coreutils/gnubug_25003': [153,],
    'coreutils/gnubug_25023': [12,],
    'jasper/cve_2016_8691': [261],
    'jasper/cve_2016_9387': [42,66],
    'libjpeg/cve_2012_2806': [111,],
    'libjpeg/cve_2017_15232': [330,351,2195,],
    'libjpeg/cve_2018_14498': [197,],
    'libjpeg/cve_2018_19664': [11,119,203,],
    'libtiff/bugzilla_2611': [12,119,203,],
    'libtiff/bugzilla_2633': [25,],
    'libtiff/cve_2014_8128': [111,],
    'libtiff/cve_2016_3186': [96,182,257,],
    'libtiff/cve_2016_3623': [257],
    'libtiff/cve_2016_5314': [152,],
    'libtiff/cve_2016_5321': [24,],
    'libtiff/cve_2016_9273': [56,261,],
    'libtiff/cve_2016_10094': [101,187,],
    'libtiff/cve_2017_7595': [257],
    'libtiff/cve_2017_7601': [90,153,],
    'libxml2/cve_2012_5134': [13,99,261],
    'libxml2/cve_2016_1834': [56,142,],
    'libxml2/cve_2016_1838': [131,],
    'libxml2/cve_2016_1839': [14,18,],
    'libxml2/cve_2017_5969': [196,],
}

final_result:Dict[str,Dict[str,int]]=dict() # Each result for each subject
statistic_for_each_subject:Dict[str,Dict[str,int]]=dict() # Success ratio for each subject
final_total_patches=0 # Total patches
final_true_correct=0 # Total correct patches that Spider success to identify
final_true_incorrect=0 # Total incorrect patches that Spider success to identify
final_false_correct=0 # Total correct patches that Spider failed to identify
final_false_incorrect=0 # Total incorrect patches that Spider failed to identify

for subject, correct_ids in CORRECT_PATCHES.items():
    if not os.path.exists(f'{subject}/output/results.json'):
        continue
    print(f'{subject}')
    with open(f'{subject}/output/results.json','r') as f:
        results=json.load(f)

    true_correct=0
    true_incorrect=0
    false_correct=0
    false_incorrect=0
    for _id, result in results.items():
        cur_id=int(_id)
        if cur_id in correct_ids:
            if result:
                # Correct patch, and the tool says it is correct
                true_correct+=1
                final_true_correct+=1
            else:
                # Correct patch, but the tool says it is incorrect
                false_correct+=1
                final_false_correct+=1
        else:
            if not result:
                # Incorrect patch, and the tool says it is incorrect
                true_incorrect+=1
                final_true_incorrect+=1
            else:
                # Incorrect patch, but the tool says it is correct
                false_incorrect+=1
                final_false_incorrect+=1

    total_patches=len(results)
    final_total_patches+=total_patches

    final_result[subject]={
        'true_correct':true_correct, # Spider identified the correct patch
        'true_incorrect':true_incorrect, # Spider identified the incorrect patch
        'false_correct':false_correct, # Spider failed to identify the correct patch
        'false_incorrect':false_incorrect, # Spider failed to identify the incorrect patch
        'total_patches':total_patches,
    }
    statistic_for_each_subject[subject]={
        'correct_rate':true_correct/(true_correct+false_correct if true_correct+false_correct!=0 else 1), # Ratio of correct patches that Spider success to identify
        'incorrect_rate':true_incorrect/(true_incorrect+false_incorrect if true_incorrect+false_incorrect!=0 else 1), # Ratio of incorrect patches that Spider success to identify
    }

with open('final_result.json','w') as f:
    json.dump({
        'result_each_subject':final_result,
        'statistic_each_subject':statistic_for_each_subject,
        'total_patches':final_total_patches,
        'result_overall':{
            'true_correct':final_true_correct,
            'true_incorrect':final_true_incorrect,
            'false_correct':final_false_correct,
            'false_incorrect':final_false_incorrect,
        },
        'statistic_overall':{
            'correct_rate':final_true_correct/(final_true_correct+final_false_correct),
            'incorrect_rate':final_true_incorrect/(final_true_incorrect+final_false_incorrect),
        }
    },f,indent=4)

with open('final_result.csv','w') as f:
    print('Subject,True Correct,False Correct,True Incorrect,False Incorrect,Total Patches,'+\
          'Correct Success Rate,Incorrect Success Rate',file=f)
    for subject in final_result:
        print(f'{subject},{final_result[subject]["true_correct"]},{final_result[subject]["false_correct"]},'+\
              f'{final_result[subject]["true_incorrect"]},{final_result[subject]["false_incorrect"]},'+\
              f'{final_result[subject]["total_patches"]},'+\
              f'{statistic_for_each_subject[subject]["correct_rate"]},{statistic_for_each_subject[subject]["incorrect_rate"]}',file=f)
    print(f'Total,{final_true_correct},{final_false_correct},{final_true_incorrect},{final_false_incorrect},'+\
          f'{final_total_patches},{final_true_correct/(final_true_correct+final_false_correct)},'+\
            f'{final_true_incorrect/(final_true_incorrect+final_false_incorrect)}',file=f)