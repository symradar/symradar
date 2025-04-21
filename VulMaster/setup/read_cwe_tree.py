import json
import pandas as pd


def line2key_values(string_):
    d = string_
    # print(d)
    # print(d.split(':')[0].strip()[1:-1], type(d.split(':')[0].strip()[1:-1]))
    # print(d.split(':')[1].strip()[1:-1], type(d.split(':')[1].strip()))
    key_ = d.split(':')[0].strip()[1:-1]
    values = d.split(':')[1].strip()[1:-1].strip().split(',')
    values = [v.strip()[1:-1] for v in values]
    new_values = []
    for v in values:
        if len(v) > 0:
            new_values.append(v)
    values = new_values

    # print('key:', key_, type(key_))
    # print('val:', values, type(values), len(values))
    return key_, values


def find_near_child_and_parent_and_peer(cur_node, parent_child_dict):
    near_child, near_parent, near_peer = [],[],[]
    if cur_node in parent_child_dict.keys():
        near_child = parent_child_dict[cur_node].copy()
    for key_ in parent_child_dict.keys():
        if (key_ != cur_node) and (cur_node in parent_child_dict[key_]):
            near_parent = key_
            near_peer = parent_child_dict[key_].copy()
            near_peer.remove(cur_node)


    return near_parent, near_child, near_peer


# with open('cwe_tree.json') as f:
#     cwe_tree_relation_data = f.readlines()
# parent_child_dict = dict()
# for i in range(len(cwe_tree_relation_data)):
#     parent, childs = line2key_values(cwe_tree_relation_data[i].strip()[1:-1])
#     parent_child_dict[parent] = childs
#
# cur_node = 'CWE-119'
# near_parent, near_child, near_peer = find_near_child_and_parent_and_peer(cur_node, parent_child_dict)
# print(near_parent, near_child, near_peer)
# print()



