<script lang="ts">
  import type {Metadata} from '$lib/metadata';
  export let data: Metadata[];
</script>


<table>
  <thead>
    <tr>
      <th>ID</th>
      <th>Bug ID</th>
      <th>Benchmark</th>
      <th>Subject</th>
      <th>Vars</th>
      <th>Buggy Code</th>
      <th>Correct Code</th>
      <th>Target</th>
    </tr>
  </thead>
  <tbody>
    {#each data as item (item.id)}
      <tr>
        <td>{item.id}</td>
        <td><a href="/benchmark?id={item.id}">{item.bug_id}</a></td>
        <td>{item.benchmark}</td>
        <td>{item.subject}</td>
        <td>{item.vars ? item.vars.join(', ') : ''}</td>
        <td>{item.buggy ? item.buggy.code : ''}</td>
        <td>{item.correct ? item.correct.code : ''}</td>
        <td>{item.target}</td>
      </tr>
    {/each}
  </tbody>
</table>
