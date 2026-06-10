declare global {
  // Declare IPlugSendMsg as a global function
  function IPlugSendMsg(message: any): void;

  interface Window {
    // From UI to Plugin
    SPVFUI: (paramIdx: number, value: number) => void;
    BPCFUI: (paramIdx: number) => void;
    EPCFUI: (paramIdx: number) => void;
    
    // From Plugin to UI
    SPVFD: (paramIdx: number, value: number) => void;
    SCVFD: (ctrlTag: number, value: number) => void;
    SCMFD: (ctrlTag: number, msgTag: number, dataSize: number, msg: string) => void;
    SAMFD: (msgTag: number, dataSize: number, msg: string) => void;
    
    OnParamChange: (paramIdx: number, value: number) => void;
  }
}

export {}; 
