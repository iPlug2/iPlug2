<script lang="ts">
  export let label: string;
  
  let level = 0;
  
  export function setLevel(value: number) {
    // Convert linear value to logarithmic scale
    // Assuming input value is 0-1, where 1 = 0dB
    if (value === 0) {
      level = 0;
    } else {
      // Convert to dB scale (-60dB to 0dB)
      const db = 20 * Math.log10(value);
      // Normalize to 0-1 range for display
      level = (db + 60) / 60;
      // Clamp between 0 and 1
      level = Math.max(0, Math.min(1, level));
    }
  }

  function getColor(height: number): string {
    if (height > 0.9) return '#ff4444';  // Red for peaks
    if (height > 0.75) return '#ffaa00'; // Yellow for high levels
    return '#4CAF50';                    // Green for normal levels
  }
</script>

<div class="vu-meter">
  <div class="label">{label}</div>
  <div class="meter-body">
    <div 
      class="meter-fill" 
      style="height: {level * 100}%; background: {getColor(level)}"
    ></div>
    <div class="scale-marks">
      <div class="mark" style="bottom: 90%"><span>0dB</span></div>
      <div class="mark" style="bottom: 75%"><span>-6dB</span></div>
      <div class="mark" style="bottom: 50%"><span>-18dB</span></div>
      <div class="mark" style="bottom: 25%"><span>-36dB</span></div>
    </div>
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
    transition: height 100ms linear;
  }

  .scale-marks {
    position: absolute;
    width: 100%;
    height: 100%;
    pointer-events: none;
  }

  .mark {
    position: absolute;
    width: 100%;
    height: 1px;
    background: rgba(255, 255, 255, 0.3);
  }

  .mark span {
    position: absolute;
    right: 35px;
    font-size: 10px;
    color: rgba(255, 255, 255, 0.7);
    transform: translateY(-50%);
    white-space: nowrap;
  }
</style> 
