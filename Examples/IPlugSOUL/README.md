# IPlugSOUL
An IPlug example with DSP generated using the [SOUL](https://soul.dev/) programming language

update the soul-generated C++ with the following command:

```soul generate --cpp IPlugSOUL_DSP.soul --output=IPlugSOUL_DSP.h```


To iterate on the SOUL DSP with the SOUL JIT compiler, use ```soul play --nopatch IPlugSOUL_DSP.soul```

To do the same, inside a DAW, use the official [SOUL plugin](https://github.com/soul-lang/SOUL/tree/master/tools/plugin) 

NOTE:

- Install the latest SOUL binaries: https://github.com/soul-lang/SOUL/releases
- The SOUL compiler can also generate WASM directly, so SOUL -> C++ -> iPlug -> WAM/WASM may not be the best option if targetting the web
