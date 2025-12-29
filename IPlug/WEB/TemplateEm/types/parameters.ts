/**
 * iPlug2 Parameter Type Definitions
 *
 * These types match the IPlug2 IParam class definitions in C++.
 */

/** Parameter type enumeration (matches IParam::EParamType) */
export enum ParamType {
  None = 0,
  Bool = 1,
  Int = 2,
  Enum = 3,
  Double = 4
}

/** Display type for parameter scaling (matches IParam::EDisplayType) */
export enum DisplayType {
  Linear = 0,
  Log = 1,
  Exp = 2
}

/** Parameter flags (matches IParam::EFlags) */
export enum ParamFlags {
  None = 0,
  CannotAutomate = 1 << 0,
  Stepped = 1 << 1,
  Negative = 1 << 2,
  SignDisplay = 1 << 3,
  Meta = 1 << 4
}

/** Parameter unit type */
export type ParamUnit =
  | 'custom'
  | 'linear'
  | 'seconds'
  | 'milliseconds'
  | 'samples'
  | 'hertz'
  | 'decibels'
  | 'pan'
  | 'phase'
  | 'octaves'
  | 'cents'
  | 'semitones'
  | 'bpm'
  | 'beats'
  | 'percent'
  | 'ratio';

/**
 * Parameter information descriptor
 * Returned by plugin initialization to describe available parameters
 */
export interface ParamInfo {
  /** Parameter index */
  id: number;
  /** Parameter name */
  name: string;
  /** Parameter type */
  type: 'bool' | 'int' | 'enum' | 'float';
  /** Minimum value */
  min: number;
  /** Maximum value */
  max: number;
  /** Default value */
  default: number;
  /** Display scaling type */
  displayType: DisplayType;
  /** Automation rate */
  rate: 'control';
  /** Optional display label/unit string */
  label?: string;
  /** Optional parameter group */
  group?: string;
  /** Step size for stepped parameters */
  step?: number;
  /** Display texts for enum parameters */
  displayTexts?: string[];
}

/**
 * Audio bus configuration
 */
export interface AudioBusInfo {
  /** Bus identifier */
  id: number;
  /** Number of channels */
  channels: number;
  /** Bus label (e.g., "Main", "Sidechain") */
  label?: string;
}

/**
 * Audio configuration descriptor
 */
export interface AudioConfig {
  /** Input bus configurations */
  inputs: AudioBusInfo[];
  /** Output bus configurations */
  outputs: AudioBusInfo[];
}

/**
 * Plugin descriptor JSON
 * Describes the plugin's capabilities and parameters
 */
export interface PluginDescriptor {
  /** Plugin name */
  name: string;
  /** Plugin unique identifier */
  bundleId?: string;
  /** Version string */
  version?: string;
  /** Manufacturer name */
  manufacturer?: string;
  /** Audio bus configuration */
  audio: AudioConfig;
  /** Parameter definitions */
  parameters: ParamInfo[];
  /** Whether plugin supports MIDI input */
  midiIn?: boolean;
  /** Whether plugin produces MIDI output */
  midiOut?: boolean;
  /** Whether plugin is an instrument (synth) */
  isSynth?: boolean;
  /** Plugin latency in samples */
  latency?: number;
}

/**
 * Get display value for a parameter
 * @param param Parameter info
 * @param value Normalized value (0-1)
 * @returns Display string
 */
export function getParamDisplayValue(param: ParamInfo, normalizedValue: number): string {
  const range = param.max - param.min;
  let value: number;

  switch (param.displayType) {
    case DisplayType.Log:
      value = param.min * Math.pow(param.max / param.min, normalizedValue);
      break;
    case DisplayType.Exp:
      value = param.min + range * normalizedValue * normalizedValue;
      break;
    case DisplayType.Linear:
    default:
      value = param.min + range * normalizedValue;
  }

  if (param.type === 'int' || param.type === 'enum') {
    value = Math.round(value);
  }

  if (param.type === 'enum' && param.displayTexts) {
    const idx = Math.round(value - param.min);
    return param.displayTexts[idx] || value.toString();
  }

  if (param.type === 'bool') {
    return value > 0.5 ? 'On' : 'Off';
  }

  // Format number with appropriate precision
  const formatted = value.toFixed(param.type === 'float' ? 2 : 0);
  return param.label ? `${formatted} ${param.label}` : formatted;
}

/**
 * Convert display value to normalized (0-1)
 * @param param Parameter info
 * @param value Display value
 * @returns Normalized value
 */
export function normalizeParamValue(param: ParamInfo, value: number): number {
  const range = param.max - param.min;

  switch (param.displayType) {
    case DisplayType.Log:
      return Math.log(value / param.min) / Math.log(param.max / param.min);
    case DisplayType.Exp:
      return Math.sqrt((value - param.min) / range);
    case DisplayType.Linear:
    default:
      return (value - param.min) / range;
  }
}

/**
 * Convert normalized (0-1) to display value
 * @param param Parameter info
 * @param normalized Normalized value
 * @returns Display value
 */
export function denormalizeParamValue(param: ParamInfo, normalized: number): number {
  const range = param.max - param.min;

  switch (param.displayType) {
    case DisplayType.Log:
      return param.min * Math.pow(param.max / param.min, normalized);
    case DisplayType.Exp:
      return param.min + range * normalized * normalized;
    case DisplayType.Linear:
    default:
      return param.min + range * normalized;
  }
}
