# IPlugSvelteUI
A basic volume control effect plug-in which uses a platform web view to host a Svelte UI, written using Svelte and typescript.
Svelte is a modern framework for building user interfaces. Svelte files have a `.svelte` extension and must be compiled to JavaScript before they can be used, this can be done using Vite which also allows for hot reloading during development.

### Setup Instructions

1. Install Node Version Manager (nvm)
   - On macOS:
     ```bash
     brew install nvm
     ```
   - On Windows:
     Download and install [nvm-windows](https://github.com/coreybutler/nvm-windows/releases)

2. Restart your terminal and verify nvm installation:
   ```bash
   nvm --version
   ```

3. Navigate to the web-ui directory:
    ```bash
    cd web-ui
    ```

4. Install/use the Node.js version specified in the `.nvmrc` file:
   ```bash
   nvm install $(cat .\.nvmrc)
   nvm use $(cat .\.nvmrc)
   ```

5. Install Node packages:
   ```bash
   npm install
   ```

6. Build the static assets:
   ```bash
   npm run build
   ```

The built assets will be generated in the `resources/web` folder.

### Development

During development you may find it more convenient to work with the Svelte files directly, and have the platform web view automatically update as you make changes aka. **hot reloading**. To do this, you can run `npm run dev` in the `web-ui` directory, which will start a local server. If you point the iPlug2 WebView to this address (e.g. `http://localhost:5173`), it will automatically reload as you make changes. See `IPlugSvelteUI.cpp` for how this is done. You need to switch back to the static pages when you are done developing and you want to package the plug-in.

Start the local server:
   ```bash
   npm run dev
   ```
