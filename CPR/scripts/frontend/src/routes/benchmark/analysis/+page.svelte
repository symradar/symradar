<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import type { Metadata } from '$lib/metadata';
  import { resultStore, metaDataStore } from '$lib/store';
  import type { NodeType, EdgeType, GraphType } from '$lib/graph_api';
  import AnalysisTable from './AnalysisTable.svelte';
  import type { ResultType } from '$lib/store';
  let meta_data: Metadata;
  let data: ResultType;
  metaDataStore.subscribe(value => {
    meta_data = value;
  });
  resultStore.subscribe(value => {
    data = value;
    console.log("analysis page", data);
    console.log("analysis page table", data.table);
  });

  const handle_error = (error: any) => {
    console.log(error);
  }
</script>

<h1><a href="/">Web UI of uni-klee</a></h1>
<a href="/benchmark?id={meta_data.id}">Goto benchmark</a>
<a href="/benchmark/table">Goto table</a>
<a href="/benchmark/graph">Goto graph</a>
<a href="/benchmark/analysis">Goto analysis</a>

<div>Current benchmark (id {meta_data.id}): {meta_data.subject}/{meta_data.bug_id}</div>

{#if data && data.table.columns.length > 0}
  <AnalysisTable table={data.table}/>
{/if}