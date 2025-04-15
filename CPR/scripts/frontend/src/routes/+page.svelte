<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import type { Metadata } from '$lib/metadata';
  import MetaDataTable from './MetaDataTable.svelte';
  import MetaTestStatus from './MetaTestStatus.svelte';
  let message: string = "invalid string";
  let meta_data: Metadata[] = [];
  const handle_error = (error: any) => {
    console.log(error);
  }
  const get_message = (data: {message: string}) => {
    message = data.message;
  }
  const get_meta_data = (data: Metadata[]) => {
    meta_data = data;
  }

  fastapi("GET", "/hello", {}, get_message, handle_error);
  fastapi("GET", "/meta-data/list", {}, get_meta_data, handle_error);

</script>

<h1><a href="/">Web UI of uni-klee</a></h1>
<MetaTestStatus />
<MetaDataTable data={meta_data} />


