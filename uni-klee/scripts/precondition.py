#!/usr/bin/env python3
import sys
import os
import pysmt.environment
from pysmt.smtlib.parser import SmtLibParser
from pysmt.shortcuts import is_sat, is_unsat, get_model, Symbol, BV, Equals, EqualsOrIff, And, Or, TRUE, FALSE, Select, BVConcat, SBV

file = "/root/projects/uni-klee/examples/list/klee-out-0/test000044.smt2" #sys.argv[1]
file_pre = "/root/projects/uni-klee/examples/list/klee-out-0/test000040.smt2"
with open(file, 'r') as f, open(file_pre, 'r') as f_pre:
    pysmt.environment.push_env()
    parser = SmtLibParser()
    script = parser.get_script(f)
    script_pre = parser.get_script(f_pre)
    formula = script.get_last_formula()
    formula_pre = script_pre.get_last_formula()
    combined = And(formula, formula_pre)
    sat = is_sat(combined)
    print(sat)
    
    