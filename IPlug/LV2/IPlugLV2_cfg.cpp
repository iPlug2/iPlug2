/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
/*
 * Generate ttl files for LV2
 */
#include "config.h"
#include "IPlugLV2.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#define PROP_PREFIX "propex"
#define FMT_MAX (4096)

#if defined(OS_WINDOWS)
#define UI_TYPE "ui:WindowsUI"
#define DLL_EXT "dll"
#elif defined(OS_MAC) || defined(OS_IOS)
#define UI_TYPE "ui:CocoaUI"
#define DLL_EXT "dylib"
#elif defined(OS_LINUX)
#define UI_TYPE "ui:X11UI"
#define DLL_EXT "so"
#else
#error LV2 not supported on this platform!
#endif

BEGIN_IPLUG_NAMESPACE

static bool IsValidSymbol(const WDL_String &str)
{
  const char *s = str.Get();
  if (isalpha(*s++))
  {
    for (; *s ; ++s)
    {
      if ((*s != '_') && !isalnum(*s))
        return false;
    }
    return true;
  }
  return false;
}

static bool MapLV2Unit(IParam *p, WDL_String& unitStr)
{
  const char* unitName = nullptr;

  switch(p->Unit())
  {
  case IParam::kUnitAbsCents:
  case IParam::kUnitCents:
    unitName = "units:cent";
    break;
  case IParam::kUnitBeats:
    unitName = "units:beat";
    break;
  case IParam::kUnitBPM:
    unitName = "units:bpm";
    break;
  case IParam::kUnitDB:
    unitName = "units:db";
    break;
  case IParam::kUnitDegrees:
    unitName = "units:degree";
    break;
  case IParam::kUnitFrequency:
    unitName = "units:hz";
    break;
  case IParam::kUnitLinearGain:
    unitStr = "iplug2:unit_amp";
    break;
  case IParam::kUnitMeters:
    unitName = "units:m";
    break;
  case IParam::kUnitMIDICtrlNum:
    unitName = "iplug2:unit_midi_ctrl";
    break;
  case IParam::kUnitMIDINote:
    unitName = "units:midiNote";
    break;
  case IParam::kUnitMilliseconds:
    unitName = "units:ms";
    break;
  case IParam::kUnitOctaves:
    unitName = "units:oct";
    break;
  case IParam::kUnitPan:
    unitName = "iplug2:unit_pan";
    break;
  case IParam::kUnitPercentage:
    unitName = "units:coef";
    break;
  case IParam::kUnitPhase:
    unitName = "units:degrees";
    break;
  case IParam::kUnitRate:
    unitName = "units:coef";
    break;
  case IParam::kUnitRatio:
    unitName = "units:coef";
    break;
  case IParam::kUnitSamples:
    unitName = "units:frame";
    break;
  case IParam::kUnitSeconds:
    unitName = "units:s";
    break;
  case IParam::kUnitSemitones:
    unitName = "units:semitone12TET";
    break;
  case IParam::kUnitCustom:
    unitStr.SetFormatted(1024,
      "[ a units:Unit; rdfs:label \"%s\"; units:render \"%%.%df\" ]",
      p->GetCustomUnit(), p->GetDisplayPrecision());
    break;
  default:
    return false;
  }
  // Basically any case other than custom.
  if (unitName != nullptr)
  {
    unitStr.Set(unitName);
  }
  return true;
}

int IPlugLV2DSP::GetFirstControlPort() const
{
  int nInputs = MaxNChannels(ERoute::kInput);
  int nOutputs = MaxNChannels(ERoute::kOutput);
  return nInputs + nOutputs + 2;
}

int IPlugLV2DSP::write_manifest(const char* dest_dir)
{
  WDL_String path (dest_dir);
  path.Append("/manifest.ttl");

  std::remove(path.Get());
  FILE *f = fopen(path.Get(), "w");
  if(f == nullptr)
  {
    return 1;
  }

  fprintf(f, "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
             "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
#ifdef PLUG_HAS_UI
             "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n"
#endif
             "\n");

  // Write the UI for this config
#ifdef PLUG_HAS_UI
  fprintf(f,  "<" PLUG_UI_URI ">\n"
              "  a " UI_TYPE " ;\n"
              "  lv2:binary <" PLUG_NAME "." DLL_EXT "> ;\n"
              "  rdfs:seeAlso <" PLUG_NAME ".ttl> .\n\n");
#endif

  // Pretend there's a different plugin for each IO config
  int nIOConfigs = NIOConfigs();
  for (int i = 0; i < nIOConfigs; i++)
  {
    fprintf(f,  "<" PLUG_URI "#io_%d>\n"
                "  a lv2:Plugin ;\n"
                "  lv2:binary <" PLUG_NAME ".so> ;\n"
                "  rdfs:seeAlso <" PLUG_NAME ".ttl> .\n",
                i);
  }
  
  fclose(f);
  return 0;
}

int IPlugLV2DSP::write_also(const char* dest_dir)
{
  // Path to output file
  WDL_String path (dest_dir);
  path.AppendFormatted(PATH_MAX, "/%s.ttl", PLUG_NAME);

  std::remove(path.Get());
  FILE *f = fopen(path.Get(), "w");
  if(f == nullptr)
  {
    return 2;
  }

  // Temp string for message formatting.
  WDL_String msg;

  fprintf(f,  "@prefix atom:  <http://lv2plug.in/ns/ext/atom#> .\n"
              "@prefix doap:  <http://usefulinc.com/ns/doap#> .\n"
              "@prefix foaf:  <http://xmlns.com/foaf/0.1/> .\n"
              "@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .\n"
              "@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
              "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n"
              "@prefix patch: <http://lv2plug.in/ns/ext/patch#> .\n"
              "@prefix units: <http://lv2plug.in/ns/extensions/units#> .\n"
#ifdef PLUG_HAS_UI
              "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n"
#endif
              "@prefix urid:  <http://lv2plug.in/ns/ext/urid#> .\n"
              "\n"
              "@prefix iplug2: <https://iplug2.github.io/lv2#> .\n"
              "@prefix " PROP_PREFIX ": <" PLUG_URI "#> .\n"
              "\n");

  // Write iplug2 units
  auto write_unit = [&](const char* name, const char* label, const char *symbol, const char *fmt_str) {
    fprintf(f, 
      "iplug2:%s\n"
      "  a units:Unit ;\n"
      "  rdfs:label   \"%s\" ;\n"
      "  units:symbol \"%s\" ;\n"
      "  units:render \"%s\" .\n",
      name, label, symbol, fmt_str);
  };

  write_unit("unit_amp", "Linear Amplitude", "amp", "%f amp");
  write_unit("unit_midi_ctrl", "MIDI Ctrl", "", "%d");
  write_unit("unit_pan", "Pan", "%", "%f");
              
  write_cfg_parameters(f);

  // Write UI config
#ifdef PLUG_HAS_UI
  fprintf(f,  "\n<" PLUG_UI_URI ">\n"
              "  a " UI_TYPE " ;\n"
              "  lv2:optionalFeature ui:parent, ui:resize ;\n"
              "  lv2:requiredFeature ui:idleInterface ;\n"
              "  lv2:extensionData   ui:idleInterface .\n\n");
#endif

  int nIOConfigs = NIOConfigs();
  for (int i = 0; i < nIOConfigs; i++)
  {
    write_also_io(f, GetIOConfig(i), i);
    fprintf(f, "\n");
  }

  fclose(f);
  return 0;
}

int IPlugLV2DSP::write_also_io(FILE* f, const IOConfig* io, int io_index)
{
  // Temp string for message formatting
  WDL_String msg;

  // URI of the plugin
  WDL_String uri (PLUG_URI);
  uri.AppendFormatted(20, "#io_%d", io_index);

  // Helper function for writing port data
  auto write_port = [&](bool prelude, const char* msg) {
    if (prelude)
    {
      fprintf(f, ", [\n");
    }
    write_indent(f, 4, msg);
    write_indent(f, 2, "]");
  };

  fprintf(f, "<%s>\n", uri.Get());
  write_indent(f, 2,
    "a lv2:Plugin ;\n" // TODO: type, f.e. lv2:AmplifierPlugin ;
    "lv2:project <" PLUG_URL_STR "> ;\n"
    "doap:name \"" PLUG_NAME "\" ;\n" // TODO: where can we get human readable name ?
    // TODO: licese, f.e. doap:license <https://github.com/iPlug2/iPlug2/blob/master/LICENSE.txt> ;
    "lv2:optionalFeature lv2:hardRTCapable ;\n"
    "lv2:requiredFeature urid:map ;\n");
#ifdef PLUG_HAS_UI
  fprintf(f, "  ui:ui <" PLUG_UI_URI "> ;\n");
#endif

  // Write patch parameters
  int nParams = NParams();
  for (int n = 0; n < nParams; n++)
  {
    fprintf(f, "  patch:writeable %s:Par%d ;\n", PROP_PREFIX, n);
  }
  
  // Current port index
  int portIndex = 0;

  // Begin list of ports
  fprintf(f, "\n"
             "  lv2:port [\n");

  // Write input control port
  write_port(false,
    "a atom:AtomPort, lv2:InputPort; \n"
    "atom:bufferType atom:Sequence;\n"
    "atom:supports atom:Object, patch:Message;\n"
#if PLUG_DOES_MIDI_IN
    "atom:supports midi:MidiEvent ;\n"
#endif
    "lv2:index 0;\n"
    "lv2:symbol \"control_in\";\n"
    "lv2:name \"Control Input\";\n"
    "lv2:designation lv2:control;\n");
  portIndex++;

  // Write output control port
  write_port(true,
    "a atom:AtomPort, lv2:OutputPort; \n"
    "atom:bufferType atom:Sequence;\n"
    "atom:supports atom:Object, patch:Message;\n"
#if PLUG_DOES_MIDI_OUT
    "atom:supports midi:MidiEvent ;\n"
#endif
    "lv2:index 1;\n"
    "lv2:symbol \"control_out\";\n"
    "lv2:name \"Control Output\";\n"
    "lv2:designation lv2:control;\n");
  portIndex++;

  // Write audio input ports
  int nInputs = io->GetTotalNChannels(ERoute::kInput);
  int nOutputs = io->GetTotalNChannels(ERoute::kOutput);
  for (int n = 0; n < nInputs; ++n)
  {
    WDL_String name(GetChannelLabel(ERoute::kInput, n));
    if(name.GetLength() == 0)
    {
      name.SetFormatted(32, "In%d", n + 1);
    }

    WDL_String symbol(name);
    if(!IsValidSymbol(symbol)) // spaces are not allowed
    {
      symbol.SetFormatted(32, "in%d", n + 1);
    }

    msg.SetFormatted(FMT_MAX,
      "a lv2:AudioPort, lv2:InputPort ;\n"
      "lv2:index %d ;\n"
      "lv2:symbol \"%s\" ;\n"
      "lv2:name \"%s\" ;\n",
      portIndex, symbol.Get(), name.Get());
    write_port(true, msg.Get());
    portIndex++;
  }

  // Write audio output ports
  for (int n = 0; n < nOutputs; ++n)
  {
    WDL_String name(GetChannelLabel(ERoute::kOutput, n));
    if(!name.GetLength())
      name.SetFormatted(32, "Out%d", n + 1);
    WDL_String symbol(name);
    if(!IsValidSymbol(symbol)) // spaces are not allowed
      symbol.SetFormatted(32, "out%d", n + 1);
    
    msg.SetFormatted(FMT_MAX,
      "a lv2:AudioPort, lv2:OutputPort ;\n"
      "lv2:index %d ;\n"
      "lv2:symbol \"%s\" ;\n"
      "lv2:name \"%s\" ;\n",
      portIndex, symbol.Get(), name.Get());
    write_port(true, msg.Get());
    portIndex++;
  }

#if LV2_CONTROL_PORTS
  // Control ports are the old LADSPA way for parameters.
  // They're backwards-compatible but not very nice to work with.
  // Only enable if requested.
  portIndex = GetFirstControlPort();

  // int nParams = NParams(); // Already declared above
  for (int n = 0; n < nParams; ++n)
  {
    IParam *p = GetParam(n);

    WDL_String symbol(p->GetName());
    if(!IsValidSymbol(symbol)) // spaces are not allowed
    {
      symbol.SetFormatted(32, "Par%d", n + 1);
    }

    msg.SetFormatted(FMT_MAX,
      "a lv2:InputPort, lv2:ControlPort ;\n"
      "lv2:index %d ;\n"
      "lv2:symbol \"%s\" ;\n"
      "lv2:name \"%s\" ;\n"
      "lv2:default %.4f ;\n"
      "lv2:minimum %.4f ;\n"
      "lv2:maximum %.4f ;\n",
      portIndex, p->GetName(), symbol.Get(),
      p->GetDefault(), p->GetMin(), p->GetMax());
      // TODO: units
    write_port(true, msg.Get());
    portIndex++;
  }
#endif // LV2_CONTROL_PORTS

  // End writing ports list
  fprintf(f, " .\n");
}

int IPlugLV2DSP::write_cfg_parameters(FILE* f)
{
  WDL_String msg;
  WDL_String range;
  WDL_String unitLine;

  int nParams = NParams();
  for (int n = 0; n < nParams; n++)
  {
    IParam *p = GetParam(n);

    switch (p->Type())
    {
    case IParam::kTypeBool:
      range.Set("atom:Bool");
      break;
    case IParam::kTypeEnum:
    case IParam::kTypeInt:
      range.Set("atom:Int");
      break;
    case IParam::kTypeDouble:
    default:
      range.Set("atom:Double");
      break;
    }

    MapLV2Unit(p, unitLine);

    fprintf(f, "%s:Par%d\n", PROP_PREFIX, n);
    msg.SetFormatted(FMT_MAX,
      "a lv2:Parameter ;\n"
      "rdfs:label \"%s\" ;\n"
      "rdfs:range %s;\n"
      "lv2:default %.4f ;\n"
      "lv2:minimum %.4f ;\n"
      "lv2:maximum %.4f ;\n"
      "units:unit %s .\n",
      p->GetName(), range.Get(),
      p->GetDefault(), p->GetMin(), p->GetMax(), unitLine.Get());
  }
}

int IPlugLV2DSP::write_indent(FILE* f, int indent, const char* msg)
{
  // First create the indent string
  char indent_str[20];
  for (int i = 0; i < indent; i++) {
    indent_str[i] = ' ';
  }
  indent_str[indent] = 0;

  // Now loop through and print indent before each newline
  const char *sp = msg;
  while (*sp)
  {
    size_t line_len = strcspn(sp, "\n\0");
    fprintf(f, "%s%.*s", indent_str, (int)line_len, sp);
    // Move to next line
    sp += line_len + 1;
  }
}


END_IPLUG_NAMESPACE


extern "C" {

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index);


int main(int argc, const char *argv[])
{
  const LV2_Descriptor *dsc = lv2_descriptor(0);
  LV2_Feature *features[] = { nullptr };
  
  if ( dsc && dsc->instantiate )
  {
    LV2_Handle handle = dsc->instantiate(dsc, 44.1, ".", features);
    auto plug = static_cast<iplug::IPlugLV2DSP *>(handle);
    if (plug)
    {
      return plug->write_manifest() || plug->write_also();
    }
  }
  return 1;
}

// Alternate "main" function may be invoked with rundyn
LV2_SYMBOL_EXPORT int write_ttl(int argc, char **argv)
{
  if (argc < 2 || strcmp(argv[1], "-h") == 0)
  {
    fprintf(stdout, "rundyn " PLUG_NAME "." DLL_EXT ",write_ttl <output_dir>\n");
    return 0;
  }

  const char* output_dir = argv[1];

  const LV2_Descriptor *dsc = lv2_descriptor(0);
  LV2_Feature *features[] = { nullptr };
  
  if ( dsc && dsc->instantiate )
  {
    LV2_Handle handle = dsc->instantiate(dsc, 44.1, ".", features);
    auto plug = static_cast<iplug::IPlugLV2DSP *>(handle);
    if (plug)
    {
      return plug->write_manifest(output_dir) || plug->write_also(output_dir);
    }
  }
  return 1;
}

}
