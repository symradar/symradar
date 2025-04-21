<script lang='ts'>
  import { marked } from 'marked';
  export let data: {table: string};
</script>

<div class="result-table">
  {@html marked.parse(data.table)}
</div>
