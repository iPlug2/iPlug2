import './app.css'
import App from './App.svelte'
import './lib/iplug' // Import the plugin communication functions

declare module '*.svelte' {
  import type { ComponentType } from 'svelte';
  const component: ComponentType;
  export default component;
}

const app = new App({
  target: document.getElementById('app')!,
})

export default app 