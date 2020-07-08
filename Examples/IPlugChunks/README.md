# IPlugChunks
An example of overriding SerializeState / UnserializeState in order to store data other than just parameter values in the plug-in state.

PLUG_DOES_STATE_CHUNKS 1 in config.h

Using chunks allows you to store arbitary data (e.g. a hidden, non-automatable parameter, a filepath etc) in the plugin's state,
 i.e. when you save a preset to a file or when you save the project in your host

