from dataclasses import dataclass
import json
import os
import shutil
from typing import Dict, List, Set, Tuple


@dataclass
class SubjectInfo:
    subject:str
    var_map:Dict[str,str]

SUBJECT_INFO = {
    'binutils/cve_2017_15025': SubjectInfo('binutils/cve_2017_15025', 
                                           {'x': 'lh.line_range','y': 'lh.maximum_ops_per_insn'}),
    'binutils/cve_2018_10372': SubjectInfo('binutils/cve_2018_10372',
                                           {'x': 'limit','y':'sizeof(uint64_t)','z': 'ph'}),
    'coreutils/bugzilla_19784': SubjectInfo('coreutils/bugzilla_19784',
                                            {'size': 'size','i':'i'}),
    'coreutils/bugzilla_26545': SubjectInfo('coreutils/bugzilla_26545',
                                            {'size': 'size','i':'i'}),
    'coreutils/gnubug_25003': SubjectInfo('coreutils/gnubug_25003',
                                          {'start': 'start','initial_read':'initial_read','bufsize':'bufsize'}),
    'coreutils/gnubug_25023': SubjectInfo('coreutils/gnubug_25023',
                                            {'col_sep_length': 'col_sep_length'}),
    'jasper/cve_2016_8691': SubjectInfo('jasper/cve_2016_8691',
                                        {'x': 'siz->comps[i].hsamp','y':'siz->comps[i].vsamp'}),
    'jasper/cve_2016_9387': SubjectInfo('jasper/cve_2016_9387',
                                        {'x': 'dec->yend','y':'dec->tileyoff','z':'SIZE_MAX'}),
    'libjpeg/cve_2012_2806': SubjectInfo('libjpeg/cve_2012_2806',
                                        {'x': 'i','t':'MAX_COMPS_IN_SCAN'}),
    'libjpeg/cve_2017_15232': SubjectInfo('libjpeg/cve_2017_15232',
                                        {'x': 'output_buf','t':'num_rows'}),
    'libjpeg/cve_2018_14498': SubjectInfo('libjpeg/cve_2018_14498',
                                        {'x': 'source->cmap_length','t':'t'}),
    'libjpeg/cve_2018_19664': SubjectInfo('libjpeg/cve_2018_19664',
                                        {'x': 'cinfo->quantize_colors','y':'dest->pub.put_pixel_rows'}),
    'libtiff/bugzilla_2611': SubjectInfo('libtiff/bugzilla_2611',
                                        {'x': 'sp->bytes_per_line','y':'cc'}),
    'libtiff/bugzilla_2633': SubjectInfo('libtiff/bugzilla_2633',
                                        {'x': 'es','y':'breaklen'}),
    'libtiff/cve_2014_8128': SubjectInfo('libtiff/cve_2014_8128',
                                        {'x': 'nrows','y':'256'}),
    'libtiff/cve_2016_3186': SubjectInfo('libtiff/cve_2016_3186',
                                        {'x': 'count','y':'status'}),
    'libtiff/cve_2016_3623': SubjectInfo('libtiff/cve_2016_3623',
                                        {'x': 'horizSubSampling','y':'vertSubSampling'}),
    'libtiff/cve_2016_5314': SubjectInfo('libtiff/cve_2016_5314',
                                        {'x': 'sp->tbuf_size','y':'sp->stream.avail_out','z':'nsamples'}),
    'libtiff/cve_2016_5321': SubjectInfo('libtiff/cve_2016_5321',
                                        {'x': 's','y':'MAX_SAMPLES'}),
    'libtiff/cve_2016_9273': SubjectInfo('libtiff/cve_2016_9273',
                                        {'x': 'td->td_nstrip','y':'td->td_rowsperstrip'}),
    'libtiff/cve_2016_10094': SubjectInfo('libtiff/cve_2016_10094',
                                        {'x': 'count','y':'t2p->tiff_datasize'}),
    'libtiff/cve_2017_7595': SubjectInfo('libtiff/cve_2017_7595',
                                        {'x': 'sp->v_sampling','y':'sp->h_sampling'}),
    'libtiff/cve_2017_7601': SubjectInfo('libtiff/cve_2017_7601',
                                        {'x': 'td->td_bitspersample','y':'sp->h_sampling'}),
    'libxml2/cve_2012_5134': SubjectInfo('libxml2/cve_2012_5134',
                                        {'x': 'len','y':'buf'}),
    'libxml2/cve_2016_1834': SubjectInfo('libxml2/cve_2016_1834',
                                        {'x': 'size','y':'len'}),
    'libxml2/cve_2016_1838': SubjectInfo('libxml2/cve_2016_1838',
                                        {'x': 'ctxt->input->end - ctxt->input->cur','y':'tlen','z':'ctxt->input->end'}),
    'libxml2/cve_2016_1839': SubjectInfo('libxml2/cve_2016_1839',
                                        {'x': 'ctxt->input->cur','y':'ctxt->input->base','z':'len'}),
    'libxml2/cve_2017_5969': SubjectInfo('libxml2/cve_2017_5969',
                                        {'x': 'content->c2','y':'NULL'}),
}

with open('filtered.json','r') as f:
    filtered=json.load(f)

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

for subject, info in SUBJECT_INFO.items():
    print(f"Subject: {info.subject}")
    vars=info.var_map

    with open(f'{subject}/meta-program-original.json','r') as f:
        patches = json.load(f)['patches']
    all_patches, correct_patch = get_all_patches(f'{subject}/group-patches-original.json')
    new_sub=subject.replace('_','-')
    if 'cve' in new_sub:
        new_sub=new_sub.replace('cve','CVE')
    new_sub=new_sub.split('/')[1]
    all_patches = set(filter(lambda x: x in filtered[new_sub], list(all_patches)))

    print(f'{subject}: {len(all_patches)} patches, correct patch: {correct_patch}')
    template_file=list(filter(lambda x: x.startswith('template-'), os.listdir(f'{subject}')))[0]
    with open(f'{subject}/{template_file}','r') as f:
        template_code=f.read()
    if '<placeholder>' not in template_code:
        raise ValueError(f'Placeholder not found in template code for {subject}')
    _index=template_file.find('-')
    orig_filename=template_file[_index+1:].strip()
    
    if os.path.exists(f'{subject}/patches'):
        shutil.rmtree(f'{subject}/patches')
    for patch in patches:
        if patch['name']=='buggy': continue
        id=patch['id']
        if id not in all_patches: continue
        code=patch['code']
        code_list:List[str]=code.splitlines()
        for i,line in enumerate(code_list.copy()):
            if 'constant_a = ' in line:
                const = line.split('=')[1].strip()[:-1] # Remove semicolon
                code_list.remove(line) # Remove temp statement
            elif 'result = ' in line:
                _index=line.find('=')
                patched_code=line[_index+1:].strip()[:-1]
                patched_code_list=patched_code.split()
                # Replace temp variables to actual vars
                for j,c in enumerate(patched_code_list):
                    for var, var_name in vars.items():
                        if var in c:
                            patched_code_list[j] = c.replace(var, var_name)
                            break
                    if 'constant_a' in c:
                        patched_code_list[j] = c.replace('constant_a', const)
                cur_patch=' '.join(patched_code_list)
        
        cur_code=template_code.replace('<placeholder>',f'/* spider */ ({cur_patch})')
        os.makedirs(f'{subject}/patches',exist_ok=True)
        with open(f'{subject}/patches/{id}-{orig_filename}','w') as f:
            f.write(cur_code)