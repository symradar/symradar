<script lang='ts'>
  import { fastapi } from '$lib/fastapi';
  import { onMount } from 'svelte';
  let status: number[] = [];
  const fetch_status = () => {
    fastapi("GET", "/benchmark/run/status", {}, (data: number[]) => {
      status = data;
    }, (error: any) => {
      console.log(error);
    });
  }
  const to_links = (ids: number[]) => {
    return ids.map(id => `<a href="/benchmark?id=${id}">${id}</a>`).join(', ');
  }
  onMount(fetch_status);
</script>

{#if status.length > 0}
  <div>Running status: {@html to_links(status)}</div>
{/if}