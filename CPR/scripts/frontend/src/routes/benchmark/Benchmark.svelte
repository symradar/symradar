<script lang='ts'>
  import type { Metadata } from '$lib/metadata';
  import { fastapi } from '$lib/fastapi';
  import { onMount } from 'svelte';
  export let data: Metadata;
  let cmds: string[] = [];
  let selected_cmd: string = "";
  const get_cmds = () => {
    const params = {id: data.id, limit: 10}
    fastapi("GET", "/benchmark/run/cmds", params, (data: string[]) => {
      cmds = data;
    }, (error: any) => {
      console.log(error);
    });
  }
  const set_cmd = (cmd: string) => {
    selected_cmd = cmd;
  }
  const run_cmd = (cmd: string) => {
    fastapi("POST", "/benchmark/run", {cmd}, (data: string[]) => {
      console.log(data);
    }, (error: any) => {
      console.log(error);
    });
  
  }
</script>

<button on:click={() => get_cmds()}> Get cmds </button>
<input type="text" bind:value={selected_cmd} />
<button on:click={() => run_cmd(selected_cmd)}> Run </button>
<ul class="button-list">
  {#each cmds as cmd}
    <li class="button-list-item">
      <button on:click={() => set_cmd(cmd)}>{cmd}</button>
    </li>
  {/each}
</ul>