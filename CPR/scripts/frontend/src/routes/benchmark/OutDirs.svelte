<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import type { Metadata } from '$lib/metadata';
  import { graphStore, mdTableStore, resultStore, dirDataStore } from '$lib/store';
  import type { ResultType, DirDataType } from '$lib/store';
  import type { NodeType, EdgeType, GraphType } from '$lib/graph_api';
  interface dir_type { id: string, full: string };
  interface log_parser_result {
    table: string, 
    fork_graph: GraphType,
    input_graph: GraphType,
    result: ResultType,
  };
  interface benchmark_current_result {
    message: string,
    running: boolean,
    dir: string,
  };

  export let meta_data: Metadata;
  let out_dirs: dir_type[] = [];
  let user_prefix = "uni-m-out";
  let show_result_table = false;
  let show_current_dir = false;
  let current_dir: string = "";

  const get_out_dirs = (prefix: string) => {
    console.log("get_out_dirs" + meta_data.id);
    fastapi("GET", "/meta-data/out-dir/", {id: meta_data.id, prefix: prefix}, (dirs: dir_type[]) => {
      console.log("get_out_dirs: " + JSON.stringify(dirs));
      out_dirs = dirs;
    }, handle_error);
  }

  const handle_error = (error: any) => {
    console.log(error);
  }

  const check_current_dir = (full_path: string) => {
    fastapi("GET", "/benchmark/run/current", {bug_id: meta_data.bug_id, dir: full_path},
      (data: benchmark_current_result) => {
        console.log("check_current_dir: " + data.message);
        show_current_dir = data.running;
        if (show_current_dir && full_path == data.dir) {
          show_result_table = false;
        }
        current_dir = data.dir;
      }, handle_error);
  }

  const handle_click_out_dir = (full_path: string) => {
    check_current_dir(full_path);
    if (current_dir == full_path) {
      console.log("current dir is running");
      return;
    }
    dirDataStore.set({dir: full_path, inputs: []});
    const data_log_parser_url = "/meta-data/data-log-parser/parse";
    const params = { dir: full_path };
    fastapi("GET", data_log_parser_url, params, handle_log_parser_response, handle_error)
  }

  const handle_log_parser_response = (result: log_parser_result) => {
    mdTableStore.set({table: result.table});
    graphStore.set(result.input_graph);
    resultStore.set(result.result);
    console.log("outdir: ", result.result);
    console.log("outdir2: ", result.result.table);
    show_result_table = true;
  }
</script>

<style>
  /* Add your styling here */
  .button-list {
    list-style: none;
    padding: 0;
    margin: 0;
  }

  .button-list-item {
    margin-bottom: 8px;
  }

  .button-list-item button {
    display: block;
    width: 100%;
    padding: 8px;
    text-align: left;
    background-color: #f0f0f0; /* Background color */
    border: 1px solid #ddd;   /* Border color */
    border-radius: 4px;       /* Rounded corners */
    cursor: pointer;
  }

  .button-list-item button:hover {
    background-color: #ddd;   /* Change background color on hover */
  }
</style>

{#if show_result_table}
  <div class="result-table">
    <a href="/benchmark/table">Goto result table</a>
  </div>
  <div class="graph">
    <a href="/benchmark/graph">Goto graph</a>
  </div>
  <div class="analysis">
    <a href="/benchmark/analysis">Goto analysis</a>
  </div>
{/if}

{#if show_current_dir}
  <div class="current-dir">
    <p>Currently running dir: {current_dir}</p>
  </div>
{/if}

<input type="text" bind:value={user_prefix} />
<button on:click={() => get_out_dirs(user_prefix)}>Get Out Dirs</button>
<ul class="button-list">
  {#each out_dirs as out_dir}
    <li class="button-list-item">
      <button on:click={() => handle_click_out_dir(out_dir.full)}>{out_dir.id}</button>
    </li>
  {/each}
</ul>