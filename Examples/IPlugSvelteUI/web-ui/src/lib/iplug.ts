// FROM UI TO PLUGIN

export function SPVFUI(paramIdx: number, value: number) {
  if (paramIdx < 0) {
    console.log("SPVFUI paramIdx must be >= 0")
    return;
  }
  
  const message = {
    msg: "SPVFUI",
    paramIdx: parseInt(String(paramIdx)),
    value: value
  };

  IPlugSendMsg(message);
}

export function BPCFUI(paramIdx: number) {
  if (paramIdx < 0) {
    console.log("BPCFUI paramIdx must be >= 0")
    return;
  }
  
  const message = {
    msg: "BPCFUI",
    paramIdx: parseInt(String(paramIdx)),
  };

  IPlugSendMsg(message);
}

export function EPCFUI(paramIdx: number) {
  if (paramIdx < 0) {
    console.log("EPCFUI paramIdx must be >= 0")
    return;
  }
  
  const message = {
    msg: "EPCFUI",
    paramIdx: parseInt(String(paramIdx)),
  };

  IPlugSendMsg(message);
}

// FROM PLUGIN TO UI
export function SPVFD(paramIdx: number, val: number) {
  OnParamChange(paramIdx, val);
}

export function OnParamChange(paramIdx: number, value: number) {
  if (SPVFD) {
    SPVFD(paramIdx, value);
  }
}

// Make these functions globally available
globalThis.SPVFUI = SPVFUI;
globalThis.BPCFUI = BPCFUI;
globalThis.EPCFUI = EPCFUI;
globalThis.SPVFD = SPVFD;
globalThis.OnParamChange = OnParamChange; 