<script lang="ts">
  import svelteLogo from './assets/svelte.svg'
  import viteLogo from './assets/vite.svg'
  import iPlugLogo from './assets/iplug.png'
  import Knob from './lib/Knob.svelte'
  import VUMeter from './lib/VUMeter.svelte'

  let gainKnob: any;
  let vuMeter: any;

  // Handle parameter value changes from the plugin
  globalThis.SPVFD = (paramIdx: number, val: number) => {
    if (paramIdx === 0) {
      gainKnob?.setValueFromPlugin(val);
    }
  };

  globalThis.SCMFD = (ctrlTag: number, msgTag: number, dataSize: number, msg: string) => {
    if (ctrlTag === 0) {
      // Decode base64 to binary string
      const msgData = atob(msg);
      
      // Convert binary string to Uint8Array
      const bytes = new Uint8Array(msgData.length);
      for (let i = 0; i < msgData.length; i++) {
        bytes[i] = msgData.charCodeAt(i);
      }
      
      // Create a single Int32Array view for all three integers
      const intView = new Int32Array(bytes.buffer, 0, 3);
      const [controlTag, nChans, chanOffset] = intView;
      const data = new Float32Array(bytes.buffer, 12);

      vuMeter?.setLevel(data[0]);
    }
  };
</script>

<main>
  <div>
    <a href="https://vite.dev" target="_blank" rel="noreferrer">
      <img src={viteLogo} class="logo" alt="Vite Logo" />
    </a>
    <a href="https://svelte.dev" target="_blank" rel="noreferrer">
      <img src={svelteLogo} class="logo svelte" alt="Svelte Logo" />
    </a>
    <a href="https://iplug2.github.io" target="_blank" rel="noreferrer">
      <img src={iPlugLogo} class="logo iplug" alt="iPlug Logo" />
    </a>
  </div>
  <h1>Vite + Svelte + iPlug2</h1>

  <div class="controls">
    <Knob 
      bind:this={gainKnob}
      paramId={0}
      label="Volume"
      minValue={-70}
      maxValue={0}
      units="dB"
      defaultValue={-70}
    />
    <VUMeter
    bind:this={vuMeter}
    label="Level"
    />
  </div>
</main>

<style>
  .logo {
    height: 6em;
    padding: 1.5em;
    will-change: filter;
    transition: filter 300ms;
  }
  .logo:hover {
    filter: drop-shadow(0 0 2em #646cffaa);
  }
  .logo.svelte:hover {
    filter: drop-shadow(0 0 2em #ff3e00aa);
  }
  :global(*) {
    user-select: none;
    -webkit-user-select: none; /* Safari */
    -moz-user-select: none; /* Firefox */
    -ms-user-select: none; /* IE10+/Edge */
  }

  h1 {
    cursor: default;
  }

  .controls {
    display: flex;
    gap: 20px;
    align-items: center;
    justify-content: center;
    flex: 1;
  }

</style>
