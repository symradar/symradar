
### ------CWE name and description -------
import json
with open('./treevul_cwe_tree.json') as f:
    cwe_description_data = json.load(f)

cwe_id_2_descriptions_from_treevul = {}
for key, value_dict in cwe_description_data.items():
    considered_fileds = ['Name', 'Description', 'Related Weaknesses', 'Observed Examples', 'Potential Mitigations']
    cwe_meta_data = ''
    for filed in considered_fileds:
        cwe_meta_data += filed + ': ' + value_dict[filed] + ' \t'
    if key not in cwe_id_2_descriptions_from_treevul:
        cwe_id_2_descriptions_from_treevul['CWE-'+str(key)] = cwe_meta_data



### ------ manually CWE collected examples -------
# import random

import pandas as pd
import difflib,sys
import os
from collections import Counter
from read_cwe_tree import line2key_values, find_near_child_and_parent_and_peer
import json

with open('./cwe_tree.json') as f:
    cwe_tree_relation_data = f.readlines()
parent_child_dict = dict()
for i in range(len(cwe_tree_relation_data)):
    parent, childs = line2key_values(cwe_tree_relation_data[i].strip()[1:-1])
    parent_child_dict[parent] = childs


df_ = pd.read_csv('./CWE_examples_GPT35_generate_fixes_full.csv')
print(df_)

column_names = df_.columns.tolist()
def diff2_before_after_str(diff):
    lines = diff.split('\n')
    lines = [l.strip() for l in lines]

    befores, afters = [],[]
    for l in lines:
        if l.startswith('- '):
            befores.append(l[1:].strip())
        elif l.startswith('+ '):
            afters.append(l[1:].strip())
        else:
            befores.append(l.strip())
            afters.append(l.strip())

    new_befores, new_afters = [],[]
    for l in befores:
        if len(l.strip()) == 0:
            continue
        else:
            new_befores.append(l.strip())

    for l in afters:
        if len(l.strip()) == 0:
            continue
        else:
            new_afters.append(l.strip())
    befores = new_befores
    afters  = new_afters

    return '\n'.join(befores), '\n'.join(afters)



import difflib,sys
from evaluator.CodeBLEU import bleu, weighted_ngram_match, syntax_match, dataflow_match
from evaluator.CodeBLEU.parser import DFG_python, DFG_java, DFG_ruby, DFG_go, DFG_php, DFG_javascript, DFG_csharp
from evaluator.CodeBLEU.parser import (remove_comments_and_docstrings,
                                       tree_to_token_index,
                                       index_to_code_token,
                                       tree_to_variable_index)
from tree_sitter import Language, Parser
import os
dfg_function = {
    'python': DFG_python,
    'java': DFG_java,
    'ruby': DFG_ruby,
    'go': DFG_go,
    'php': DFG_php,
    'javascript': DFG_javascript,
    'c_sharp': DFG_csharp,
}
def parse_code_tokens(candidate, lang):
    root_dir = './evaluator/CodeBLEU'
    LANGUAGE = Language(root_dir + '/parser/my-languages.so', lang)
    parser = Parser()
    parser.language=LANGUAGE
    parser = [parser, dfg_function[lang]]
    match_count = 0
    total_count = 0

    candidate = remove_comments_and_docstrings(candidate, 'c_sharp')

    code = candidate
    tree = parser[0].parse(bytes(code, 'utf8'))
    root_node = tree.root_node
    tokens_index = tree_to_token_index(root_node)
    code = code.split('\n')
    code_tokens = [index_to_code_token(x, code) for x in tokens_index]
    return code_tokens
def get_token_pair_diff(pre_version_file_str,
                        post_version_file_str, num_tokens):
    try:
        pre_token_per_line = pre_version_file_str.replace(' ', '\n')
        post_token_per_line = post_version_file_str.replace(' ', '\n')

        diff = list(difflib.unified_diff(pre_token_per_line.splitlines(True), post_token_per_line.splitlines(True),
                                         fromfile='1.txt', tofile='1_1.txt', n=1000000))
        if diff:
            if num_tokens == 1000:
                return (pre_version_file_str, post_version_file_str)
            state = 0;
            src = ""
            src_line = ""
            bugtag = False
            tgt = ""
            pre_tokens = ["<S2SV_null>"] * num_tokens
            post_tokens = ["<S2SV_null>"] * num_tokens
            for t in diff:
                t = t.replace('\n', '')
                if t.startswith("--- ") or t.startswith("+++ ") or t.startswith("@@ "):
                    if state > 2:
                        print(f'ERROR: preamble line {t} occurred in unexpected location')
                    state += 1  # State will be 3 at start of real tokens
                elif state < 3:
                    print(f'ERROR: token line {t} occurred before preamble done')
                elif t.startswith(" "):
                    if t != " //<S2SV>":
                        src_line += t[1:] + ' '
                    elif bugtag:
                        src += "<S2SV_StartBug> " + src_line + "<S2SV_EndBug> "
                        bugtag = False
                        src_line = ""
                        continue
                    else:
                        src += src_line
                        src_line = ""
                        continue
                    if state == 3:  # Continue idle state
                        pre_tokens = pre_tokens[1:num_tokens] + [t[1:]]
                    elif state % 100 == num_tokens - 1:
                        post_tokens = post_tokens[1:num_tokens] + [t[1:]]
                        if state >= 300:  # addition
                            tgt += '<S2SV_ModStart> ' + ' '.join(pre_tokens) + ' ' + ' '.join(new_tokens) + ' '
                        elif state >= 200:  # modify
                            tgt += '<S2SV_ModStart> ' + ' '.join(pre_tokens) + ' ' + ' '.join(
                                new_tokens) + ' <S2SV_ModEnd> ' + ' '.join(post_tokens) + ' '
                        elif state >= 100:  # delete
                            tgt += '<S2SV_ModStart> ' + ' '.join(pre_tokens) + ' <S2SV_ModEnd> ' + ' '.join(
                                post_tokens) + ' '
                        state = 3
                        pre_tokens = post_tokens
                    else:
                        state += 1  # Advance post_token count
                        post_tokens = post_tokens[1:num_tokens] + [t[1:]]
                elif t.startswith("-"):
                    if t != "-//<S2SV>":
                        src_line += t[1:] + ' '
                    elif bugtag:
                        src += "<S2SV_StartBug> " + src_line + "<S2SV_EndBug> "
                        bugtag = False
                        src_line = ""
                        continue
                    else:
                        src += src_line
                        src_line = ""
                        continue
                    if state == 3:  # Enter from idle state
                        bugtag = True
                        state = 100  # Assume delete at first
                        new_tokens = []
                    elif state >= 300:  # Addition changes to modification
                        new_tokens += post_tokens[num_tokens - (state % 100):num_tokens]
                        state = 200
                    elif state >= 200:  # Accumulate any post tokens we may have
                        new_tokens += post_tokens[num_tokens - (state % 100):num_tokens]
                        state = 200
                    elif state > 100:  # Post count after delete changes to modification
                        new_tokens += post_tokens[num_tokens - (state % 100):num_tokens]
                        state = 200
                    post_tokens = ["<S2SV_null>"] * num_tokens

                elif t.startswith("+"):
                    if t == "+//<S2SV>":
                        continue
                    if state == 3:  # Enter from idle state
                        bugtag = True
                        state = 300  # Assume addition at first
                        new_tokens = [t[1:]]
                    elif state >= 300:
                        new_tokens += post_tokens[num_tokens - (state % 100):num_tokens] + [t[1:]]
                        if state > 300:  # Check if we started accumulating post tokens
                            state = 200
                    elif state >= 200:  # accumulate any post tokens we may have
                        new_tokens += post_tokens[num_tokens - (state % 100):num_tokens] + [t[1:]]
                        state = 200  # Modified
                    elif state >= 100:  # delete changes to modify
                        new_tokens += post_tokens[num_tokens - (state % 100):num_tokens] + [t[1:]]
                        state = 200  # Change to modified
                    post_tokens = ["<S2SV_null>"] * num_tokens

            # Fix end-of-file post tokens by putting <S2SV_null> at end
            post_tokens = post_tokens[num_tokens - (state % 100):num_tokens] + \
                          post_tokens[0:num_tokens - (state % 100)]

            if state >= 300:  # addition
                tgt += '<S2SV_ModStart> ' + ' '.join(pre_tokens) + ' ' + ' '.join(new_tokens) + ' '
            elif state >= 200:  # modify
                tgt += '<S2SV_ModStart> ' + ' '.join(pre_tokens) + ' ' + ' '.join(
                    new_tokens) + ' <S2SV_ModEnd> ' + ' '.join(post_tokens) + ' '
            elif state >= 100:  # delete
                tgt += '<S2SV_ModStart> ' + ' '.join(pre_tokens) + ' <S2SV_ModEnd> ' + ' '.join(post_tokens) + ' '
            if not tgt:
                print(f'ERROR: {pre_version_file_str} found no target changes in {diff}')
            return (src.strip(), tgt.strip())
        else:
            print(f'No diff found for {pre_version_file_str}')
            sys.exit(2)
    except Exception as e:
        print("Get token pair fail: " + str(e))
def add_special_space_token (src):
    src_tokens = parse_code_tokens(src, 'c_sharp')
    new_src_tokens = []
    for token in src_tokens:
        if ' ' in token:
            new_src_tokens.append(token.replace(' ', '<S2SV_blank>'))
        else:
            new_src_tokens.append(token)
    src_tokens = new_src_tokens
    new_src_tokens = []
    for token in src_tokens:
        if token in [';', '}', '{', '...']:
            token = token+'\n'
        new_src_tokens.append(token)
    return ' '.join(new_src_tokens)
def from_string_to_input_output(src, tgt):
    new_src = add_special_space_token (src)
    new_tgt = add_special_space_token (tgt)
    src = new_src
    tgt = new_tgt
    src = src.split('\n')
    src = [' //<S2SV>'+s for s in src]
    tgt = tgt.split('\n')
    tgt = [' //<S2SV>'+s for s in tgt]
    src = '\n'.join(src)
    tgt = '\n'.join(tgt)
    src, tgt = get_token_pair_diff(src, tgt, 3)
    return src, tgt
all_source_vulnerable_codes , all_target_fixed_codes, all_vulnerable_types  = [],[],[]
all_befores, all_afters = [],[]
for i in range(len(df_)):
    one_item = df_.iloc[i, :]
    cwe_id = one_item[0]
    for name in column_names[1:]:
        try:
            assert(len(one_item[name])>0)
            print(one_item[name])
            before, after = diff2_before_after_str(one_item[name])
            all_befores.append(before.replace('\n', ' '))
            all_afters.append(after.replace('\n', ' '))
            before, after = from_string_to_input_output(before, after)

            before = before.replace('//<S2SV>', '')
            before = before.replace('<S2SV_StartBug>', '<vul-start>').replace('<S2SV_EndBug>', '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ')
            after = after.replace("<S2SV_ModStart>", '<vul-start>').replace("<S2SV_ModEnd>", '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ')

            all_source_vulnerable_codes.append(before)
            all_target_fixed_codes.append(after)
            all_vulnerable_types.append(cwe_id)
        except:
            continue


all_source_vulnerable_codes = [ all_vulnerable_types[i] +" "+ all_source_vulnerable_codes[i]   for i in range(len(all_source_vulnerable_codes))]


def remove_redundent_newlines(l):
    l = l.replace('\n', ' ')
    l = ' '.join(l.split())
    return l
all_source_vulnerable_codes = [ remove_redundent_newlines(item) for item in all_source_vulnerable_codes]
all_target_fixed_codes = [ remove_redundent_newlines(item) for item in all_target_fixed_codes]
print()


print()
context_passage_lists = []
assert (len(all_source_vulnerable_codes) == len(all_vulnerable_types) == len(all_target_fixed_codes))
for i in range(len(all_source_vulnerable_codes)):
    source_ = all_source_vulnerable_codes[i]
    target_ = all_target_fixed_codes[i]
    type_ = all_vulnerable_types[i]

    one_item = {"id": str(10000+i), "title": type_.strip()+" Vulnerable Code Is: "+ source_, "text": type_ + " Fixed Code Lines are: "+target_}
    context_passage_lists.append(one_item)





print()
from datasets import load_dataset
# Question 1: What is the dataset? I don't have thost files. Maybe from VRepair?
full_dataset = load_dataset('csv', data_files={"train":"deduplicated_data/train.csv","test":"deduplicated_data/test.csv", "validation":"deduplicated_data/valid.csv"})
train_dataset = full_dataset["train"]
valid_dataset = full_dataset["validation"]
test_dataset = full_dataset["test"]


def dataset2list(dataset):
    sources = list(dataset["source"])
    targets = list(dataset["target"])
    return sources, targets
train_sources, train_targets = dataset2list(train_dataset)
valid_sources, valid_targets = dataset2list(valid_dataset)
test_sources, test_targets = dataset2list(test_dataset)
assert (len(train_sources) == len(train_targets))
train_sources = [s.replace('<S2SV_StartBug>', '<vul-start>').replace('<S2SV_EndBug>', '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ') for s in train_sources]
valid_sources = [s.replace('<S2SV_StartBug>', '<vul-start>').replace('<S2SV_EndBug>', '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ') for s in valid_sources]
test_sources = [s.replace('<S2SV_StartBug>', '<vul-start>').replace('<S2SV_EndBug>', '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ') for s in test_sources]
train_targets = [s.replace("<S2SV_ModStart>", '<vul-start>').replace("<S2SV_ModEnd>", '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ') for s in train_targets]
valid_targets = [s.replace("<S2SV_ModStart>", '<vul-start>').replace("<S2SV_ModEnd>", '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ') for s in valid_targets]
test_targets = [s.replace("<S2SV_ModStart>", '<vul-start>').replace("<S2SV_ModEnd>", '<vul-end>').replace('<S2SV_blank>', ' ').replace('<S2SV_null>', ' ').replace('<S2SV>', ' ') for s in test_targets]

### ------ 将 manually vulnerable function 变为AST -------
import argparse
import os,sys
sys.path.insert(0, '../')
sys.path.insert(0, '../../')
sys.path.insert(0, '../../..')
from evaluator.CodeBLEU import bleu, weighted_ngram_match, syntax_match, dataflow_match
from evaluator.CodeBLEU.parser import DFG_python, DFG_java, DFG_ruby, DFG_go, DFG_php, DFG_javascript, DFG_csharp
from evaluator.CodeBLEU.parser import (remove_comments_and_docstrings,
                                       tree_to_token_index,
                                       index_to_code_token,
                                       tree_to_variable_index)
from tree_sitter import Language, Parser
import os,json
from collections import Counter
from tqdm import tqdm
root_dir = './evaluator/CodeBLEU'
dfg_function = {
    'python': DFG_python,
    'java': DFG_java,
    'ruby': DFG_ruby,
    'go': DFG_go,
    'php': DFG_php,
    'javascript': DFG_javascript,
    'c_sharp': DFG_csharp,
}
def generate_SBT(candidate, lang):
    JAVA_LANGUAGE = Language(root_dir + '/parser/my-languages.so', lang)
    parser = Parser()
    parser.language=JAVA_LANGUAGE
    match_count = 0
    total_count = 0

    try:
        candidate = remove_comments_and_docstrings(candidate, 'c_sharp')
    except:
        pass

    candidate_tree = parser.parse(bytes(candidate, 'utf8')).root_node

    def get_SBT(node, raw_code):
        all_types = []
        sbt_expression = ''
        if len(node.children) == 0:
            # sbt_expression += " ({( "+raw_code[node.start_point[1]:node.end_point[1]]+" )}) "+ str(node.type)
            sbt_expression += " ({( " + str(node.type) + " )}) " + str(node.type)
            all_types.append(str(node.type))
        else:
            sbt_expression += " ({( " + str(node.type)
            for child_node in node.children:
                sbt_expression += get_SBT(child_node, raw_code)[0]
                all_types += get_SBT(child_node, raw_code)[1]
            sbt_expression += " )}) " + str(node.type)
        return sbt_expression, all_types


    SBT_sexps, types = get_SBT(candidate_tree, candidate)

    return SBT_sexps, types
def generate_deepth_first_trajectory(candidate, lang):
    JAVA_LANGUAGE = Language(root_dir + '/parser/my-languages.so', lang)
    parser = Parser()
    parser.language=JAVA_LANGUAGE
    match_count = 0
    total_count = 0

    try:
        candidate = remove_comments_and_docstrings(candidate, 'c_sharp')
    except:
        pass

    candidate_tree = parser.parse(bytes(candidate, 'utf8')).root_node

    def get_tree(node):
        return node.sexp()

    first_order_tree = get_tree(candidate_tree)
    return first_order_tree
def generate_subtree(candidate, start_end, lang):
    JAVA_LANGUAGE = Language(root_dir + '/parser/my-languages.so', lang)
    parser = Parser()
    parser.language=JAVA_LANGUAGE
    match_count = 0
    total_count = 0

    try:
        candidate = remove_comments_and_docstrings(candidate, 'c_sharp')
    except:
        pass

    candidate_tree = parser.parse(bytes(candidate, 'utf8')).root_node

    def get_subtree(node, raw_code):
        sbt_expression = ''
        if len(node.children) == 0:
            if node.start_point[1] >= start_end[0] and node.end_point[1] <= start_end[1]:
                if str(node.type).replace('_','').isalpha():
                    sbt_expression += " " + node.type + "_" + raw_code[node.start_point[1]:node.end_point[1]] + " "
                else:
                    sbt_expression += " " + node.type + " "

        else:
            if node.start_point[1] >= start_end[0] and node.end_point[1] <= start_end[1]:
                sbt_expression += " (" + node.type + ", "
            for child_node in node.children:
                sbt_expression += get_subtree(child_node, raw_code)
            if node.start_point[1] >= start_end[0] and node.end_point[1] <= start_end[1]:
                sbt_expression += " )"
        return sbt_expression


    SBT_sexps = get_subtree(candidate_tree, candidate)

    return SBT_sexps
def find_buggy_location(code):
    start_ = code.find('<vul-start>')
    end_ = code.rfind('<vul-end>')
    return (max(start_-100,0), min(end_+100, len(code)))


start_end_positions = [ find_buggy_location(d) for d in train_sources]
ast_data = [ ' '.join(d.split()[1:]) for d in train_sources]
all_trees, all_first_trees = [],[]
for i in tqdm(range(len(ast_data))):
    code = ast_data[i]
    # sbt_exp, type_ = generate_SBT(code, 'c_sharp')
    # Question 2: Why you use C# only? Where is C?
    first_order  = generate_deepth_first_trajectory(code, 'c_sharp')
    subtrees = generate_subtree(code , start_end_positions[i],  'c_sharp')
    all_first_trees.append(first_order)
    all_trees.append(subtrees)
train_trees = all_trees

start_end_positions = [ find_buggy_location(d) for d in valid_sources]
ast_data = [ ' '.join(d.split()[1:]) for d in valid_sources]
all_trees, all_first_trees = [],[]
for i in tqdm(range(len(ast_data))):
    code = ast_data[i]
    # sbt_exp, type_ = generate_SBT(code, 'c_sharp')
    first_order  = generate_deepth_first_trajectory(code, 'c_sharp')
    subtrees = generate_subtree(code , start_end_positions[i],  'c_sharp')
    all_first_trees.append(first_order)
    all_trees.append(subtrees)
valid_trees = all_trees

start_end_positions = [ find_buggy_location(d) for d in test_sources]
ast_data = [ ' '.join(d.split()[1:]) for d in test_sources]
all_trees, all_first_trees = [],[]
for i in tqdm(range(len(ast_data))):
    code = ast_data[i]
    # sbt_exp, type_ = generate_SBT(code, 'c_sharp')
    first_order  = generate_deepth_first_trajectory(code, 'c_sharp')
    subtrees = generate_subtree(code , start_end_positions[i],  'c_sharp')
    all_first_trees.append(first_order)
    all_trees.append(subtrees)
test_trees = all_trees


with open('cwe_tree.json') as f:
    cwe_tree_relation_data = f.readlines()
parent_child_dict = dict()
for i in range(len(cwe_tree_relation_data)):
    parent, childs = line2key_values(cwe_tree_relation_data[i].strip()[1:-1])
    parent_child_dict[parent] = childs


train_fid_input_lists = []
for i in range(len(train_sources)):
    source_ = train_sources[i]
    target_ = train_targets[i]
    type_ = source_.split()[0].strip()
    source_ = ' '.join(source_.split()[0:1024])
    source_length = len(source_.split())
    unit_chunk_length = 200
    num_units = int(source_length/unit_chunk_length)+1
    chunk_sources = [] ## 将过长的输入切分成多个部分
    for jk in range(num_units):
        chunk_sources.append(  ' '.join(source_.split() [ max(0, jk*unit_chunk_length-10) : min((jk+1)*unit_chunk_length+10, source_length) ] ) )
    one_item = {"question": type_.strip()+" Code Input Vulnerable Code Is: "+ chunk_sources[0], "answers": [type_ +  " Fixed Code Lines are: "+target_]}

    question_backup = chunk_sources[0]

    too_long_code_contents = []
    for jk in range(num_units-1):
        jk = jk + 1
        too_long_code_contents.append ( {"id": str(100*i+jk), "title": type_.strip()+" Code Input Vulnerable Code Is: "+ chunk_sources[jk], "text": type_ + " Fixed Code Lines are:"} )

    source_tree = train_trees[i]
    source_length = len(source_tree.split())
    unit_chunk_length = 100
    num_units = min (int(source_length / unit_chunk_length) + 1, 3)
    ast_chunk_sources = []  ## 将过长的输入切分成多个部分
    for jk in range(num_units):
        ast_chunk_sources.append(' '.join(source_tree.split()[ max(0, jk * unit_chunk_length - 10): min((jk + 1) * unit_chunk_length + 10,source_length)]))
    for ast_ in ast_chunk_sources:
        jk = jk + 1
        too_long_code_contents.append ( {"id": str(100*i+jk), "title": type_.strip()+" Code Input AST Vulnerable Code Is: "+ ast_, "text": type_ + " Fixed Code Lines are:"} )


    candidate_lines = []
    candidate_num = 20
    near_parent, near_child, near_peer = find_near_child_and_parent_and_peer(type_, parent_child_dict)
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] == type_:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_parent:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_child:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_peer:
            candidate_lines.append(context_passage_lists[j])

    if type_ == 'CWE-000':
        candidate_lines = context_passage_lists[0:3]
        cwe_name_and_description = 'Unknown CWE type and no descriptions'
    else:
        if type_ not in cwe_id_2_descriptions_from_treevul:
            cwe_name_and_description =  ' Unknown CWE type and no descriptions'
        else:
            cwe_name_and_description = cwe_id_2_descriptions_from_treevul[type_]
    cwe_name_and_description  = [{"id": str(40000), "title": type_ + ' ' + cwe_name_and_description.strip(), "text": type_ + " Fixed Code Lines are:"}]



    candidate_line_list_string = []
    for item_ in candidate_lines:
        string_ =  item_['title'] + ' ' + item_['text'] + ' ' + question_backup
        candidate_line_list_string.append(string_)
    new_candidate_lines = []
    for line_ in candidate_line_list_string:
        new_candidate_lines.append({"id": 20000, "title": line_, "text": "CWE-119 Fixed Code Lines are:"})

    question_backup = [{"id": str(0), "title": type_.strip() + " Code Input Vulnerable Code Is: " + question_backup,
                        "text": type_ + " Fixed Code Lines are:"}]


    context_passages = too_long_code_contents + question_backup + cwe_name_and_description + new_candidate_lines
    while len(context_passages)< candidate_num:
        context_passages += too_long_code_contents + question_backup + cwe_name_and_description + new_candidate_lines
    context_passages = context_passages[0:candidate_num]

    one_item['ctxs'] = context_passages

    train_fid_input_lists.append(one_item)



with open('cwe_tree.json') as f:
    cwe_tree_relation_data = f.readlines()
parent_child_dict = dict()
for i in range(len(cwe_tree_relation_data)):
    parent, childs = line2key_values(cwe_tree_relation_data[i].strip()[1:-1])
    parent_child_dict[parent] = childs

print(" \n  %%%%%%%%% Here %%%%%%%% \n ")

valid_fid_input_lists = []
for i in range(len(valid_sources)):
    source_ = valid_sources[i]
    target_ = valid_targets[i]
    type_ = source_.split()[0].strip()
    source_ = ' '.join(source_.split()[0:1024])
    source_length = len(source_.split())
    unit_chunk_length = 200
    num_units = int(source_length/unit_chunk_length)+1
    chunk_sources = [] ## 将过长的输入切分成多个部分
    for jk in range(num_units):
        chunk_sources.append(  ' '.join(source_.split() [ max(0, jk*unit_chunk_length-10) : min((jk+1)*unit_chunk_length+10, source_length) ] ) )
    one_item = {"question": type_.strip()+" Code Input Vulnerable Code Is: "+ chunk_sources[0], "answers": [type_ +  " Fixed Code Lines are: "+target_]}

    question_backup = chunk_sources[0]


    too_long_code_contents = []
    for jk in range(num_units-1):
        jk = jk + 1
        too_long_code_contents.append ( {"id": str(100*i+jk), "title": type_.strip()+" Code Input Vulnerable Code Is: "+ chunk_sources[jk], "text": type_ + " Fixed Code Lines are:"} )

    source_tree = valid_trees[i]
    source_length = len(source_tree.split())
    unit_chunk_length = 100
    num_units = min (int(source_length / unit_chunk_length) + 1, 3)
    ast_chunk_sources = []  ## 将过长的输入切分成多个部分
    for jk in range(num_units):
        ast_chunk_sources.append(' '.join(source_tree.split()[max(0, jk * unit_chunk_length - 10): min((jk + 1) * unit_chunk_length + 10, source_length)]))
    for ast_ in ast_chunk_sources:
        jk = jk + 1
        too_long_code_contents.append( {"id": str(100 * i + jk), "title": type_.strip() + " Code Input AST Vulnerable Code Is: " + ast_, "text": type_ + " Fixed Code Lines are:"})

    candidate_lines = []
    candidate_num = 20
    near_parent, near_child, near_peer = find_near_child_and_parent_and_peer(type_, parent_child_dict)
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] == type_:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_parent:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_child:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_peer:
            candidate_lines.append(context_passage_lists[j])

    if type_ == 'CWE-000':
        candidate_lines = context_passage_lists[0:3]
        cwe_name_and_description = 'Unknown CWE type and no descriptions'
    else:
        if type_ not in cwe_id_2_descriptions_from_treevul:
            cwe_name_and_description =' Unknown CWE type and no descriptions'
        else:
            cwe_name_and_description = cwe_id_2_descriptions_from_treevul[type_]
    cwe_name_and_description  = [{"id": str(40000), "title": type_ + ' ' + cwe_name_and_description.strip(), "text": type_ + " Fixed Code Lines are:"}]

    candidate_line_list_string = []
    for item_ in candidate_lines:
        string_ =  item_['title'] + ' ' + item_['text'] + ' ' + question_backup
        candidate_line_list_string.append(string_)
    new_candidate_lines = []
    for line_ in candidate_line_list_string:
        new_candidate_lines.append({"id": 20000, "title": line_, "text": "CWE-119 Fixed Code Lines are:"})


    question_backup = [{"id": str(0), "title": type_.strip() + " Code Input Vulnerable Code Is: " + question_backup,
                        "text": type_ + " Fixed Code Lines are:"}]

    context_passages = too_long_code_contents + question_backup + cwe_name_and_description + new_candidate_lines
    while len(context_passages) < candidate_num:
        context_passages += too_long_code_contents + question_backup + cwe_name_and_description + new_candidate_lines
    context_passages = context_passages[0:candidate_num]

    one_item['ctxs'] = context_passages

    valid_fid_input_lists.append(one_item)


with open('cwe_tree.json') as f:
    cwe_tree_relation_data = f.readlines()
parent_child_dict = dict()
for i in range(len(cwe_tree_relation_data)):
    parent, childs = line2key_values(cwe_tree_relation_data[i].strip()[1:-1])
    parent_child_dict[parent] = childs

print(" \n  %%%%%%%%% Here %%%%%%%% \n ")

test_fid_input_lists = []
for i in range(len(test_sources)):
    source_ = test_sources[i]
    target_ = test_targets[i]
    type_ = source_.split()[0].strip()
    source_ = ' '.join(source_.split()[0:1024])
    source_length = len(source_.split())
    unit_chunk_length = 200
    num_units = int(source_length/unit_chunk_length)+1
    chunk_sources = [] ## 将过长的输入切分成多个部分
    for jk in range(num_units):
        chunk_sources.append(  ' '.join(source_.split() [ max(0, jk*unit_chunk_length-10) : min((jk+1)*unit_chunk_length+10, source_length) ] ) )
    one_item = {"question": type_.strip()+" Code Input Vulnerable Code Is: "+ chunk_sources[0], "answers": [type_ +  " Fixed Code Lines are: "+target_]}

    question_backup = chunk_sources[0]


    too_long_code_contents = []
    for jk in range(num_units-1):
        jk = jk + 1
        too_long_code_contents.append ( {"id": str(100*i+jk), "title": type_.strip()+" Code Input Vulnerable Code Is: "+ chunk_sources[jk], "text": type_ + " Fixed Code Lines are:"} )

    source_tree = test_trees[i]
    source_length = len(source_tree.split())
    unit_chunk_length = 100
    num_units = min (int(source_length / unit_chunk_length) + 1, 3)
    ast_chunk_sources = []  ## 将过长的输入切分成多个部分
    for jk in range(num_units):
        ast_chunk_sources.append(' '.join(source_tree.split()[max(0, jk * unit_chunk_length - 10): min((jk + 1) * unit_chunk_length + 10, source_length)]))
    for ast_ in ast_chunk_sources:
        jk = jk + 1
        too_long_code_contents.append( {"id": str(100 * i + jk), "title": type_.strip() + " Code Input AST Vulnerable Code Is: " + ast_, "text": type_ + " Fixed Code Lines are:"})

    candidate_lines = []
    candidate_num = 20
    near_parent, near_child, near_peer = find_near_child_and_parent_and_peer(type_, parent_child_dict)
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] == type_:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_parent:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_child:
            candidate_lines.append(context_passage_lists[j])
    for j in range(len(all_vulnerable_types)):
        if all_vulnerable_types[j] in near_peer:
            candidate_lines.append(context_passage_lists[j])

    if type_ == 'CWE-000':
        candidate_lines = context_passage_lists[0:3]
        cwe_name_and_description = 'Unknown CWE type and no descriptions'
    else:
        if type_ not in cwe_id_2_descriptions_from_treevul:
            cwe_name_and_description = ' Unknown CWE type and no descriptions'
        else:
            cwe_name_and_description = cwe_id_2_descriptions_from_treevul[type_]
    cwe_name_and_description  = [{"id": str(40000), "title": type_ + ' ' + cwe_name_and_description.strip(), "text": type_ + " Fixed Code Lines are:"}]

    candidate_line_list_string = []
    for item_ in candidate_lines:
        string_ =  item_['title'] + ' ' + item_['text'] + ' ' + question_backup
        candidate_line_list_string.append(string_)
    new_candidate_lines = []
    for line_ in candidate_line_list_string:
        new_candidate_lines.append({"id": 20000, "title": line_, "text": "CWE-119 Fixed Code Lines are:"})

    question_backup = [{"id": str(0), "title": type_.strip() + " Code Input Vulnerable Code Is: " + question_backup,
                        "text": type_ + " Fixed Code Lines are:"}]

    context_passages = too_long_code_contents + question_backup + cwe_name_and_description + new_candidate_lines
    while len(context_passages) < candidate_num:
        context_passages += too_long_code_contents + question_backup + cwe_name_and_description + new_candidate_lines
    context_passages = context_passages[0:candidate_num]

    one_item['ctxs'] = context_passages

    test_fid_input_lists.append(one_item)

print()
os.mkdir('./vulmaster_data')
print(len(train_fid_input_lists), len(valid_fid_input_lists) , len(test_fid_input_lists))
import  json
with open('./vulmaster_data/train.json', 'w') as f:
    json.dump(train_fid_input_lists, f, indent=4)

with open('./vulmaster_data/dev.json', 'w') as f:
    json.dump(valid_fid_input_lists, f, indent=4)

with open('./vulmaster_data/test.json', 'w') as f:
    json.dump(test_fid_input_lists, f, indent=4)

print()
