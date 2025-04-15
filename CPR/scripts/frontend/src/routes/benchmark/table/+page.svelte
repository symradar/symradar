<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import type { Metadata } from '$lib/metadata';
  import {onMount} from 'svelte';
  import ResultTable from './ResultTable.svelte';
  import { mdTableStore, metaDataStore } from '$lib/store';
  let meta_data: Metadata;
  metaDataStore.subscribe(value => {
    meta_data = value;
  });
  let result_table: {table: string} = {table: 'not available'};
  mdTableStore.subscribe(value => {
    result_table = value;
  });

</script>


<h1><a href="/">Web UI of uni-klee</a></h1>
<a href="/benchmark?id={meta_data.id}">Goto benchmark</a>
<a href="/benchmark/table">Goto table</a>
<a href="/benchmark/graph">Goto graph</a>
<a href="/benchmark/analysis">Goto analysis</a>
<ResultTable data={result_table} />

