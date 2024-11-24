<script lang="ts">
  import { SPVFUI, BPCFUI, EPCFUI } from './iplug';

  export let paramId = 0;
  export let defaultValue = 0.0;
  export let value = defaultValue;
  export let label = '';
  export let minValue = 0;
  export let maxValue = 100;
  export let units = '';
  export let minAngle = -135;
  export let maxAngle = 135;
  export let circleStrokeColor = '#fff';
  export let circleStrokeWidth = 2;
  export let circleFillColor = '#000';
  export let pointerColor = '#fff';
  export let pointerWidth = 4;
  export let valueArcColor = '#333';
  export let valueArcWidth = 4;

  let currentValue = defaultValue;

  export function setValueFromPlugin(newValue) {
    // convert normalized value (0-1) to real value based on min/max
    currentValue = minValue + newValue * (maxValue - minValue);
  }

  function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
    const angleInRadians = (angleInDegrees - 90) * Math.PI / 180.0;
    return {
      x: centerX + (radius * Math.cos(angleInRadians)),
      y: centerY + (radius * Math.sin(angleInRadians))
    };
  }

  function calculateValueArcPath(angle) {
    const startAngle = minAngle;
    const endAngle = angle;
    const arcRadius = 48;
    
    const start = polarToCartesian(50, 50, arcRadius, startAngle);
    const end = polarToCartesian(50, 50, arcRadius, endAngle);
    
    const largeArcFlag = endAngle - startAngle <= 180 ? 0 : 1;
    
    return [
      'M', start.x, start.y,
      'A', arcRadius, arcRadius, 0, largeArcFlag, 1, end.x, end.y
    ].join(' ');
  }

  function updateValue(newValue: number) {
    currentValue = newValue;
    const normValue = (newValue - minValue) / (maxValue - minValue);
    value = newValue;

    console.log('Sending parameter value:', paramId, normValue);
    SPVFUI(paramId, normValue);
  }

  function startDrag(e: MouseEvent | TouchEvent) {
    if ('button' in e && e.button === 2) return; // right click

    console.log('Begin parameter change:', paramId);
    BPCFUI(paramId);

    e.preventDefault();
    const initialY = ('touches' in e) ? e.touches[0].clientY : e.clientY;
    const initialValue = currentValue;

    function onMove(e) {
      const clientY = ('touches' in e) ? e.touches[0].clientY : e.clientY;
      const deltaY = initialY - clientY;
      const valueChange = deltaY * (maxValue - minValue) / 100;
      const newValue = Math.min(Math.max(initialValue + valueChange, minValue), maxValue);
      updateValue(newValue);
    }

    function onEnd() {
      document.removeEventListener('mousemove', onMove);
      document.removeEventListener('mouseup', onEnd);
      document.removeEventListener('touchmove', onMove);
      document.removeEventListener('touchend', onEnd);
      document.body.classList.remove('hidden-cursor');
      document.body.style.cursor = '';

      console.log('End parameter change:', paramId);
      EPCFUI(paramId);
    }

    document.addEventListener('mousemove', onMove);
    document.addEventListener('mouseup', onEnd);
    document.addEventListener('touchmove', onMove, { passive: false });
    document.addEventListener('touchend', onEnd);
    document.body.classList.add('hidden-cursor');
    document.body.style.cursor = 'none';
  }

  function onWheel(e) {
    e.preventDefault();
    const delta = e.deltaY < 0 ? 1 : -1;
    const valueChange = delta * (maxValue - minValue) / 100;
    const newValue = Math.min(Math.max(currentValue + valueChange, minValue), maxValue);
    updateValue(newValue);
  }

  $: angle = minAngle + ((currentValue - minValue) / (maxValue - minValue)) * (maxAngle - minAngle);
  $: valueArcPath = calculateValueArcPath(angle);
</script>

<div class="container">
  <div class="label">{label}</div>
  <svg viewBox="0 0 100 100" width="80" height="80">
    <circle 
      cx="50" 
      cy="50" 
      r="42" 
      fill={circleFillColor}
      stroke={circleStrokeColor}
      stroke-width={circleStrokeWidth}
      on:mousedown={startDrag}
      on:touchstart={startDrag}
      on:wheel={onWheel}
      role="slider"
      tabindex="0"
      aria-valuemin={minValue}
      aria-valuemax={maxValue}
      aria-valuenow={currentValue}
      aria-label={label}
    />
    <path 
      class="value-arc" 
      fill="none" 
      stroke={valueArcColor}
      stroke-width={valueArcWidth}
      d={valueArcPath}
    />
    <line 
      class="pointer"
      x1="50"
      y1="10"
      x2="50"
      y2="50"
      stroke={pointerColor}
      stroke-width={pointerWidth}
      transform="rotate({angle}, 50, 50)"
    />
  </svg>
  <div class="value">{currentValue.toFixed(1)} {units}</div>
</div>

<style>
  .container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    width: 100%;
    height: 100%;
  }

  .label {
    margin-bottom: 8px;
    color: white;
    font-size: 14px;
    pointer-events: none;
  }

  .value {
    margin-top: 8px;
    color: white;
    font-size: 12px;
    pointer-events: none;
  }

  :global(.hidden-cursor) {
    cursor: none;
  }
</style> 
