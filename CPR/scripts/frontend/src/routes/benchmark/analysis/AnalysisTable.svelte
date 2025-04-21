<script lang='ts'>
  import type { ResultType, AnalysisTableType, DirDataType, InputSelectType, StateType } from '$lib/store';
  import { dirDataStore } from '$lib/store';
  import { onMount } from 'svelte';
  import { fastapi } from '$lib/fastapi';
  export let table: AnalysisTableType;
  interface InputSummaryType {
    crashId: number,
    isCrash: boolean,
    state: number,
    symbolic_constraints: {
      constraint: string,
      position: string,
    }[],
    symbolic_objects: {
      address: number,
      name: string,
      size: number,
      value: string,
    }[],
  }
  interface MultiSelectResultType {
    selected_input_a: number,
    selected_input_b: number,
    diff: {
      diff: number,
      state_a: StateType,
      state_b: StateType,
      input_a: InputSummaryType,
      input_b: InputSummaryType,
      compare: {
        symbolic_objects: {
          diff: boolean,
          a: string[],
          b: string[],
        }
        symbolic_constraints: {
          diff: boolean,
          a: string[],
          b: string[],
        }
      }
    },
    remaining_inputs: number[],
    remaining_patches: number[],
  }
  let dirData: DirDataType;
  dirDataStore.subscribe(value => {
    dirData = value;
  });
  let input_select_list: {input: number, feasibility: boolean, used: boolean}[] = [];
  let localTable: AnalysisTableType = {columns: [], rows: []};
  let trace_data: {trace: string[], input: InputSummaryType};
  const original_columns: number[] = [...table.columns];
  const original_rows: {base: number, row: boolean[]}[] = [...table.rows];
  let remaining_inputs: Map<number, boolean[]> = new Map();
  let remaining_patches: Set<number> = new Set();
  let showOriginalTable: boolean = false;
  let showTrace = false;
  let multiSelectMode = false;
  let inputDiff: MultiSelectResultType;

  const get_table_header = (): number[] => {
    let header: number[] = [];
    for (const column of original_columns) {
      if (remaining_patches.has(column)) {
        header.push(column);
      }
    }
    return header;
  };

  const get_table_row = (input: number): boolean[] => {
    if (!remaining_inputs.has(input)) {
      return [];
    }
    const row = remaining_inputs.get(input);
    if (row && remaining_patches.size > 0) {
      const hasPatches = original_rows.some((r) => remaining_patches.has(r.base));
      if (hasPatches) {
        // Extract the corresponding row based on the input
        let result: boolean[] = [];
        row.map((value, index) => {
          if (remaining_patches.has(original_columns[index])) {
            result.push(value);
          }
        });
        return result;
      }
    }
    return [];
  }

  const rebuild_table = () => {
    // Rebuild table based on remaining_inputs, remaining_patches
    console.log("rebuild_table");
    console.log(remaining_inputs);
    console.log(remaining_patches);
    let new_columns = get_table_header();
    let new_rows: {base: number, row: boolean[]}[] = [];
    for (const [base, values] of remaining_inputs) {
      new_rows.push({base: base, row: get_table_row(base)});
    }
    localTable = {columns: new_columns, rows: new_rows.sort((a, b) => a.base - b.base)};
  };

  const get_input_trace = () => {
    const params: DirDataType = { dir: dirData.dir, inputs: input_select_list };
    console.log("get_input_trace: " + JSON.stringify(params));
    fastapi("POST", "/meta-data/data-log-parser/explain", params, (data: {trace: string[], input: InputSummaryType}) => {
      console.log("get_input_trace: " + JSON.stringify(data));
      if (data == null) {
        console.log("get_input_trace: data is null");
        return;
      }
      trace_data = data;
      showTrace = true;
    }, (error: any) => {
      console.log(error);
    });
  };

  const update_remainings = (rmi: number[], rmp: number[]) => {
    const remaining_inputs_filter = new Set(rmi);
    for (const key of remaining_inputs.keys()) {
      if (!remaining_inputs_filter.has(key)) {
        remaining_inputs.delete(key);
      }
    }
    remaining_patches = new Set(rmp);
    rebuild_table();
  };

  const select_input = () => {
    input_select_list.forEach((value, index) => {
      value.used = true;
    });
    const params: DirDataType = { dir: dirData.dir, inputs: input_select_list };
    console.log("select_input: " + JSON.stringify(params));
    fastapi("POST", "/meta-data/data-log-parser/select", params, (data: {selected_input: number, remaining_patches: number[], remaining_inputs: number[]}) => {
      console.log("select_input: " + JSON.stringify(data));
      const sel_in: InputSelectType = { input: data.selected_input, feasibility: true, used: false };
      input_select_list = [...input_select_list, sel_in];
      console.log("select result: ", input_select_list);
      // Update remaining_inputs, remaining_patches
      update_remainings(data.remaining_inputs, data.remaining_patches);
      console.log("remaining_inputs: " + JSON.stringify(remaining_inputs));
      console.log("remaining_patches: " + JSON.stringify(remaining_patches));
      multiSelectMode = false;
    }, (error: any) => {
      console.log(error);
    });
  };

  const multi_select_input = () => {
    input_select_list.forEach((value, index) => {
      value.used = true;
    });
    const params: DirDataType = { dir: dirData.dir, inputs: input_select_list };
    console.log("select_input_compare: " + JSON.stringify(params));
    fastapi("POST", "/meta-data/data-log-parser/multi/select", params, (data: MultiSelectResultType) => {
      console.log("select_input_compare: " + JSON.stringify(data));
      if (data.selected_input_a == null || data.selected_input_a < 0) {
        console.log("select_input_compare: data is null");
        input_select_list = input_select_list;
        multiSelectMode = false;
        update_remainings(data.remaining_inputs, data.remaining_patches);
        return;
      }
      if (data.selected_input_b == null || data.selected_input_b < 0) {
        console.log("select_input_compare: data is null");
        const sel_in_a: InputSelectType = { input: data.selected_input_a, feasibility: true, used: false };
        input_select_list = [...input_select_list, sel_in_a];
        multiSelectMode = false;
        update_remainings(data.remaining_inputs, data.remaining_patches);
        return;
      }
      multiSelectMode = true;
      const sel_in_a: InputSelectType = { input: data.selected_input_a, feasibility: true, used: false };
      const sel_in_b: InputSelectType = { input: data.selected_input_b, feasibility: true, used: false };
      input_select_list = [...input_select_list, sel_in_a, sel_in_b];
      console.log("select result: ", input_select_list);
      // Update remaining_inputs, remaining_patches
      update_remainings(data.remaining_inputs, data.remaining_patches);
      inputDiff = data;
    }, (error: any) => {
      console.log(error);
    });
  }

  onMount(() => {
    console.log("AnalysisTable onMount");
    // Initialize remaining_inputs, remaining_patches
    table.rows.forEach((value, index) => {
      remaining_inputs.set(value.base, value.row);
    });
    table.columns.forEach((value, index) => {
      remaining_patches.add(value);
    });
    rebuild_table();
  });
</script>


<h2>Analysis Table</h2>

{#if showTrace}
  <p>Trace</p>
  <div class="trace-container">
    <table>
      <thead>
        <tr>
          <th>Input</th>
          <th>State</th>
          <th>IsCrash</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>{trace_data.input.crashId}</td>
          <td>{trace_data.input.state}</td>
          <td>{trace_data.input.isCrash}</td>
        </tr>
      </tbody>
    </table>
    <div>
      <p>Symbolic objects</p>
      <ul class="symbolic_objects">
        {#each trace_data.input.symbolic_objects as object}
          <li class="symbolic_objects-elem">{object.name}: {object.value} (size: {object.size})</li>
        {/each}
      </ul>
    </div>
    <div>
      <p>Symbolic constraints</p>
      <ul class="symbolic_constraints">
        {#each trace_data.input.symbolic_constraints as constraint}
          <li class="symbolic_constraints-elem">{constraint.constraint}</li>
        {/each}
      </ul>
    </div>
    <div>
      <p>Trace</p>
      <ul class="trace-list">
        {#each trace_data.trace as trace}
          <li class="trace-elem">{trace}</li>
        {/each}
      </ul>
    </div>
  </div>
{/if}

{#if multiSelectMode}
  <div>
    <p>Input diff</p>
    <table>
      <thead>
        <tr>
          <th>Label</th>
          <th>Input</th>
          <th>State</th>
          <th>IsCrash</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>a</td>
          <td>{inputDiff.selected_input_a}</td>
          <td>{inputDiff.diff.input_a.state}</td>
          <td>{inputDiff.diff.input_a.isCrash}</td>
        </tr>
        <tr>
          <td>b</td>
          <td>{inputDiff.selected_input_b}</td>
          <td>{inputDiff.diff.input_b.state}</td>
          <td>{inputDiff.diff.input_b.isCrash}</td>
        </tr>
      </tbody>
    </table>
    <table>
      <thead>
        <tr>
          <th>Value</th>
          <th>a({inputDiff.selected_input_a})</th>
          <th>b({inputDiff.selected_input_b})</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>Symbolic objects</td>
          <td>
            {#each inputDiff.diff.compare.symbolic_objects.a as obj}
              <div>{obj}</div>
            {/each}
          </td>
          <td>
            {#each inputDiff.diff.compare.symbolic_objects.b as obj}
              <div>{obj}</div>
            {/each}
          </td>
        </tr>
        <tr>
          <td>Symbolic constraints</td>
          <td>
            {#each inputDiff.diff.compare.symbolic_constraints.a as obj}
              <div>{obj}</div>
            {/each}
          </td>
          <td>
            {#each inputDiff.diff.compare.symbolic_constraints.b as obj}
              <div>{obj}</div>
            {/each}
          </td>
        </tr>
        <tr>
          <td>Stack trace</td>
          <td>{inputDiff.diff.state_a.stackTrace}</td>
          <td>{inputDiff.diff.state_b.stackTrace}</td>
        </tr>
        <tr>
          <td>Trace diff</td>
          <td></td>
          <td></td>
        </tr>
      </tbody>
    </table>
  </div>
{/if}

<p>Selected input & feasibility</p>
<div class="sel-input-container">
  {#each input_select_list as input}
    <div class="sel-input-elem {input.used ? '' : 'used'}">
      <input type="checkbox" bind:checked={input.feasibility} />
      <input type="number" bind:value={input.input} class="input-input" />
    </div>
  {/each}
</div>

<button on:click={() => rebuild_table()}> Rebuild table </button>
<button on:click={() => get_input_trace()}> Get input trace </button>
<button on:click={() => multi_select_input()}> Send feasiblity & get new multi input </button>
<button on:click={() => select_input()}> Send feasiblity & get new input </button>
<p>Remaining patches</p>
<div class="patch-container">
  {#each remaining_patches as patch}
    <div class="patch-elem">{patch}</div>
  {/each}
</div>

<table>
  <thead>
    <tr>
      <th>Input</th>
      {#each localTable.columns as column}
        <th>p{column}</th>
      {/each}
    </tr>
  </thead>
  <tbody>
    {#each localTable.rows as row}
      <tr>
        <td>i{row.base}</td>
        {#each row.row as value}
          {#if remaining_inputs.has(row.base)}
            <td>{value ? 'O' : 'X'}</td>
          {/if}
        {/each}
      </tr>
    {/each}
  </tbody>
</table>

<button on:click={() => showOriginalTable = !showOriginalTable}> {showOriginalTable ? "Hide" : "Show"} original table </button>
{#if showOriginalTable}
  <table>
  <thead>
    <tr>
      <th>Input</th>
      {#each original_columns as column}
        <th>p{column}</th>
      {/each}
    </tr>
  </thead>
  <tbody>
    {#each original_rows as row}
      <tr>
        <td>i{row.base}</td>
        {#each row.row as value}
          <td>{value ? 'O' : 'X'}</td>
        {/each}
      </tr>
    {/each}
  </tbody>
</table>
{/if}

<style>
  table {
    border-collapse: collapse;
    overflow-x: auto;
  }

  th, td {
    border: 1px solid #ddd;
    padding: 8px;
    text-align: left;
  }

  th {
    background-color: #f2f2f2;
  }
  li {
    padding: 8px;
    border-bottom: 1px solid #ddd; /* Optional: Add a border between list items for better visibility */
  }
  .trace-container {
    max-height: 200px; /* Set your desired maximum height */
    max-width: 80%;
    overflow-y: auto;
    border: 1px solid #3cc; /* Optional: Add a border for better visibility */
  }

  .trace-list {
    list-style-type: none;
    padding: 0;
    margin: 0;
  }

  .trace-elem {
    padding: 1px;
  }

  .patch-container {
    max-height: 200px; /* Set your desired maximum height */
    max-width: 80%;
    overflow-x: auto;
    border: 1px solid #c3c; /* Optional: Add a border for better visibility */
    white-space: nowrap;
  }

  .patch-elem {
    display: inline-block;
    padding: 2px;
    border: 1px solid #33c;
  }

  .sel-input-container {
    max-height: 200px; /* Set your desired maximum height */
    max-width: 80%;
    overflow-y: auto;
    display: flex;
    border: 1px solid #3cc; /* Optional: Add a border for better visibility */
    padding: 10px;
  }

  .sel-input-elem {
    padding: 8px;
    border: 1px solid #3cc; /* Optional: Add a border between list items for better visibility */
    width: 10%;
  }
  .used {
    color: red;
    border: 1px solid red;
  }
  .input-input {
    width: 60%;
  }
</style>
