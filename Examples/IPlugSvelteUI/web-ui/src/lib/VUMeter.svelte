<script lang="ts">
  export let label: string;
  
  let level = 0;
  
  export function setLevel(value: number) {
    level = value;
  }
</script>

<div class="vu-meter">
  <div class="label">{label}</div>
  <div class="meter-body">
    <div class="meter-fill" style="height: {level * 100}%"></div>
  </div>
</div>

<style>
  .vu-meter {
    width: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
  }

  .label {
    font-size: 14px;
    color: #fff;
  }

  .meter-body {
    width: 30px;
    height: 200px;
    background: #333;
    border-radius: 4px;
    position: relative;
    overflow: hidden;
  }

  .meter-fill {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    background: #4CAF50;
    transition: height 100ms linear;
  }
</style> 
