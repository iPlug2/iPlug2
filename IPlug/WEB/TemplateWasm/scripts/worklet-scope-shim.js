// AudioWorklet scope shim for Emscripten
// Provides 'self', 'location' and 'Module' globals for the AudioWorklet
// environment, where 'window' and 'document' are not available.
var self = globalThis;
self.location = self.location || { href: 'https://localhost/' };
var Module = globalThis.Module = globalThis.Module || {};

