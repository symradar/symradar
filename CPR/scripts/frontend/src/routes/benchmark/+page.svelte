<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import type { Metadata } from '$lib/metadata';
  import { graphStore, mdTableStore, metaDataStore } from '$lib/store';
  import type { NodeType, EdgeType, GraphType } from '$lib/graph_api';
  import Benchmark from './Benchmark.svelte';
  import OutDirs from './OutDirs.svelte';
  const urlSearchParams = new URLSearchParams(window.location.search);
  let data = {id: parseInt(urlSearchParams.get('id') || '0')};
  let meta_data: Metadata;


  const get_meta_data = (id: number) => {
    console.log("get_meta_data" + id);
    fastapi("GET", "/meta-data/info/" + id, {}, (data: {meta: Metadata, conf: object, meta_program: object}) => {
      console.log("get_meta_data: " + JSON.stringify(data));
      meta_data = data.meta;
      metaDataStore.set(meta_data);
    }, handle_error);
  }

  const handle_error = (error: any) => {
    console.log(error);
  }

  get_meta_data(data.id);

</script>

<h1><a href="/">Web UI of uni-klee</a></h1>
<h2>Id: {data.id}</h2>
<h3>Bug id is {meta_data ? meta_data.bug_id : ''}</h3>
<div class="benchmark">
  <Benchmark data={meta_data} />
</div>

<div class="out-dirs">
  <OutDirs meta_data={meta_data} />
</div>