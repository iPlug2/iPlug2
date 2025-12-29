/**
 * @iplug2/em-audioworklet - TypeScript Types for iPlug2 Emscripten AudioWorklet
 *
 * This package provides TypeScript type definitions for the iPlug2 EMAudioWorklet
 * web audio plugin system.
 *
 * @example
 * ```typescript
 * import type { IPlugController, ParamInfo } from '@iplug2/em-audioworklet';
 *
 * const controller: IPlugController = new IPlugController('MyPlugin');
 * controller.on('paramChange', ({ paramIdx, value }) => {
 *   console.log(`Parameter ${paramIdx} changed to ${value}`);
 * });
 * await controller.init();
 * ```
 */

// Message types
export type {
  UIMessageType,
  DelegateMessageType,
  ParamValueFromUI,
  BeginParamChangeFromUI,
  EndParamChangeFromUI,
  MidiMsgFromUI,
  SysexMsgFromUI,
  ArbitraryMsgFromUI,
  ParamValueFromDelegate,
  ControlValueFromDelegate,
  ControlMsgFromDelegate,
  MidiMsgFromDelegate,
  SysexMsgFromDelegate,
  ArbitraryMsgFromDelegate
} from './messages';

export { isParamValue, isControlValue, isMidiMsg } from './messages';

// Parameter types
export type {
  ParamUnit,
  ParamInfo,
  AudioBusInfo,
  AudioConfig,
  PluginDescriptor
} from './parameters';

export {
  ParamType,
  DisplayType,
  ParamFlags,
  getParamDisplayValue,
  normalizeParamValue,
  denormalizeParamValue
} from './parameters';

// Module types
export type {
  EmscriptenModule,
  IPlugEmModule,
  IPlugModuleFactory
} from './module';

// Controller types
export type {
  ControllerInitOptions,
  ControllerEventMap,
  ControllerEventListener,
  ControllerCallbacks,
  AudioContextState,
  IPlugController,
  IPlugControllerConstructor
} from './controller';
