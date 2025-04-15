<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import {onMount} from 'svelte';
  import Graph from './Graph.svelte'
  import type { NodeType, EdgeType, GraphType } from '$lib/graph_api'
  import { graphStore, metaDataStore } from '$lib/store';
  import type { Metadata } from '$lib/metadata';
  let meta_data: Metadata;
  let graph_data: GraphType;
  metaDataStore.subscribe(value => {
    meta_data = value;
  });
  graphStore.subscribe(value => {
    graph_data = value;
  });
</script>

<h1><a href="/">Web UI of uni-klee</a></h1>
<a href="/benchmark?id={meta_data.id}">Goto benchmark</a>
<a href="/benchmark/table">Goto table</a>
<a href="/benchmark/graph">Goto graph</a>
<a href="/benchmark/analysis">Goto analysis</a>

<Graph graph_data={graph_data} />

