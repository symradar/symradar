#!/usr/bin/env python3
from typing import Union, List, Dict, Tuple, Optional, Set
from fastapi import FastAPI, Query, BackgroundTasks, Request
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from starlette.responses import RedirectResponse, FileResponse
import multiprocessing as mp

import os
import sys
import json

# import importlib
# PARENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# import module from uni-klee.py
import uni_klee
import symvass

app = FastAPI()

app.mount("/_app", StaticFiles(directory="frontend/build/_app"), name="_app")

class Item(BaseModel):
    name: str
    price: float
    is_offer: Optional[bool] = None

# Serve static files from frontend/build
@app.get("/")
def read_root():
    return FileResponse("frontend/build/index.html")
@app.get("/favicon.ico")
def read_favicon():
    return FileResponse("frontend/build/favicon.ico")
@app.get("/benchmark")
def read_benchmark():
    return FileResponse("frontend/build/benchmark.html")
@app.get("/benchmark/{path}")
def read_benchmark_path(path: str):
    return FileResponse(f"frontend/build/benchmark/{path}.html")

class InputSelect(BaseModel):
    input: int
    feasibility: bool
    used: bool

class DirData(BaseModel):
    dir: str
    inputs: List[InputSelect]

# APIs
@app.get("/meta-data/list")
def meta_data_list():
    return uni_klee.global_config.get_meta_data_list()

@app.get("/meta-data/info/{id}")
def meta_data_info(id: str):
    print(f"meta_data_info: {id}")
    return uni_klee.global_config.get_meta_data_info_by_id(id)

@app.get("/meta-data/out-dir")
def meta_data_out_dir(id: int = Query(0), prefix = Query("uni-m-out")):
    print(f"meta_data_out_dir: {id} & {prefix}")
    config = uni_klee.global_config.get_config_for_analyzer(id, prefix)
    print(f"meta_data_out_dir: {config.conf_files.out_base_dir}, {config.conf_files.out_dir_prefix}")
    dirs = config.conf_files.find_all_nums(config.conf_files.out_base_dir, config.conf_files.out_dir_prefix)
    result = list()
    for dir, id in dirs:
        result.append({"id": dir, "full": os.path.join(config.conf_files.out_base_dir, dir)})
    return result

def check_and_get_result(dp: uni_klee.DataLogParser) -> dict:
    cache_file = dp.get_cache_file("result.json")
    data_log_file = dp.get_cache_file("data.log")
    if os.path.exists(cache_file):
        if os.path.exists(data_log_file):
            cache_time = os.path.getmtime(cache_file)
            data_log_time = os.path.getmtime(data_log_file)
            if cache_time > data_log_time:
                with open(cache_file, "r") as f:
                    ar = json.load(f)
                return ar
    dp.read_data_log()
    ar = dp.generate_table_v2(dp.cluster())
    return ar

@app.get("/meta-data/data-log-parser/parse")
def meta_data_data_log_parser(dir: str = Query("")):
    print(f"meta_data_data_log_parser: {dir}")
    if not os.path.exists(dir):
        return {"table": ""}
    dp = symvass.SymvassAnalyzer(dir)
    dp.read_data_log("data.log")
    result_table = dp.generate_table(dp.cluster())
    with open(result_table, "r") as f:
        result = f.read()
    fork_map_nodes, fork_map_edges = dp.generate_fork_graph_v2()
    input_map_nodes, input_map_edges = dp.generate_input_graph()
    ar = check_and_get_result(dp)
    return {"table": result, 
            "fork_graph": {"nodes": fork_map_nodes, "edges": fork_map_edges}, 
            "input_graph": {"nodes": input_map_nodes, "edges": input_map_edges},
            "result": ar,
        }

@app.get("/meta-data/data-log-parser/result")
def meta_data_data_log_parser_result(dir: str = Query("")):
    print(f"meta_data_data_log_parser_result: {dir}")
    if not os.path.exists(dir):
        return {"result": ""}
    dp = uni_klee.DataLogParser(dir)
    ar = check_and_get_result(dp)
    return {"result": ar}

@app.post("/meta-data/data-log-parser/explain")
def meta_data_data_log_parser_explain(request_data: DirData):
    if not os.path.exists(request_data.dir) or len(request_data.inputs) == 0:
        return {"result": ""}
    dp = uni_klee.DataLogParser(request_data.dir)
    ar = check_and_get_result(dp)
    return dp.get_trace(ar, request_data.inputs[-1].input)

@app.post("/meta-data/data-log-parser/select")
def meta_data_data_log_parser_select(request_data: DirData):
    print(f"meta_data_data_log_parser_select: {request_data}")
    if not os.path.exists(request_data.dir):
        return {"result": ""}
    dp = uni_klee.DataLogParser(request_data.dir)
    ar = check_and_get_result(dp)
    input_list = list()
    feas_list = list()
    for i in request_data.inputs:
        input_list.append(i.input)
        feas_list.append(i.feasibility)
    selected_input, remaining_patches, remaining_inputs = dp.select_input(ar, input_list, feas_list)
    return {"selected_input": selected_input, "remaining_patches": remaining_patches, "remaining_inputs": remaining_inputs}

@app.post("/meta-data/data-log-parser/multi/select")
def meta_data_data_log_parser_multi_select(request_data: DirData):
    print(f"meta_data_data_log_parser_select: {request_data}")
    if not os.path.exists(request_data.dir):
        return {"result": ""}
    dp = uni_klee.DataLogParser(request_data.dir)
    ar = check_and_get_result(dp)
    input_list = list()
    feas_list = list()
    for i in request_data.inputs:
        input_list.append(i.input)
        feas_list.append(i.feasibility)
    selected_input_a, selected_input_b, diff, remaining_patches, remaining_inputs = dp.select_input_v2(ar, input_list, feas_list)
    return {"selected_input_a": selected_input_a, "selected_input_b": selected_input_b, "diff": diff, "remaining_patches": remaining_patches, "remaining_inputs": remaining_inputs}

@app.post("/meta-data/data-log-parser/feasible")
def meta_data_data_log_parser_feasible(request_data: DirData):
    if not os.path.exists(request_data.dir):
        return {"result": ""}
    dp = uni_klee.DataLogParser(request_data.dir)
    ar = check_and_get_result(dp)
    return dp.filter_out_patches(ar, request_data.inputs, request_data.feasible_list)

def run_cmd(cmd: List[str]):
    final_cmd = cmd
    final_cmd[0] = "uni-klee.py"
    if "--lock=f" not in final_cmd and "--lock=w" not in final_cmd:
        final_cmd.append("--lock=w")
    process = mp.Process(target=uni_klee.main, args=(final_cmd,))
    process.start()
    process.join()

@app.get("/benchmark/run/status")
def meta_data_run_status():
    return uni_klee.global_config.get_current_processes()

@app.get("/benchmark/run/cmds")
def meta_data_run_cmds(id: int = Query(0), limit: int = Query(10)):
    return uni_klee.global_config.get_last_command(id, limit)

@app.get("/benchmark/run/current")
def meta_data_run_current(bug_id: str = Query(""), dir: str = Query("")):
    if bug_id == "" or dir == "":
        return {"message": "bug_id or dir is not provided", "running": False, "dir": ""}
    lock_file = uni_klee.global_config.get_lock_file(bug_id)
    if not os.path.exists(lock_file):
        return {"message": f"no running process {bug_id}", "running": False, "dir": ""}
    with open(lock_file, "r") as f:
        lines = f.readlines()
        if len(lines) < 2:
            return {"message": f"no running process {bug_id}", "running": False, "dir": ""}
        return {"message": f"running process {bug_id}", "running": True, "dir": lines[1].strip()}
    return {"message": f"no running process {bug_id}", "running": False, "dir": ""}

@app.post("/benchmark/run")
async def meta_data_run(cmd: Dict[str, str], background_tasks: BackgroundTasks):
    if "cmd" not in cmd:
        return {"message": "cmd is not provided"}
    background_tasks.add_task(run_cmd, cmd["cmd"].split())
    return {"message": f"run {cmd} in background"}

@app.get("/analyze")
def analyze():
    return {"message": "analyze"}

@app.get("/hello")
def hello():
    return {"message": "Hello World"}

@app.get("/items/{item_id}")
def read_item(item_id: int, q: Union[str, None] = None):
    return {"item_id": item_id, "q": q}

@app.put("/items/{item_id}")
def update_item(item_id: int, item: Item):
    return {"item_name": item.name, "item_id": item_id}