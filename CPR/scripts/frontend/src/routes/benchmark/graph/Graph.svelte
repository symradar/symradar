<script lang="ts">
  import cytoscape from 'cytoscape';
  import cola from 'cytoscape-cola';
  import { onMount } from 'svelte';
  import { parse } from 'svelte/compiler';
  import type { GraphType, NodeType, EdgeType } from '$lib/graph_api';
  export let graph_data: GraphType;
  cytoscape.use(cola);
  let container: HTMLElement;
  let selectedNodeScratch: any = null;
  const handle_click: cytoscape.EventHandler = (event: cytoscape.EventObject) => {
    console.log("clicked node");
    const node: any = event.target;
    const data: {id: string, extra: object} = node.data();
    console.log(data);
    selectedNodeScratch = data.extra;
  }

  onMount(async () => {
    const graph = cytoscape({ 
      container: container, 
      elements: graph_data,
      style: [
        {
          selector: 'node',
          style: {
            'background-color': '#666',
            'label': 'data(id)'
          }
        },
        {
          selector: 'edge',
          style: {
            'width': 3,
            'line-color': '#ccc',
            'target-arrow-color': '#ccc',
            'target-arrow-shape': 'vee',
            'curve-style': 'bezier',
          }
        }
      ],
    });
    graph.on('tap', 'node', handle_click);
    // graph.nodes().once('tap', handle_click);
    const layout = graph.layout({
      name: 'cola',
      infinite: true,
      fit: false,
    });
    layout.run();
  });
</script>

<style>
  .container {
    display: flex;
    width: 100%;
    height: 80vh;
    position: static;
    border: solid 1px;
  }
  .sidebar {
    width: 25%;
    height: 100%;
    float: right;
    border: solid 1px;
    overflow-y: auto;
    overflow-x: auto;
  }
</style>

<div class="container">
  <div bind:this={container} class="container" />
  <div class="sidebar">
    {#if selectedNodeScratch}
      <pre>{JSON.stringify(selectedNodeScratch, null, 2)}</pre>
    {/if}
  </div>
</div>