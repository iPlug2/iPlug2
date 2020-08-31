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

int IPlugLV2DSP::write_manifest()
{
  unlink("minifest.ttl");
  FILE *f = fopen("manifest.ttl", "w");
  if(f)
  {
    fprintf(f, "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
               "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
#ifdef LV2_WITH_UI
               "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n"
#endif
               "\n");
    fprintf(f, "<" PLUG_URI ">\n"
               "  a lv2:Plugin ;\n"
               "  lv2:binary <" PLUG_NAME ".so> ;\n"
               "  rdfs:seeAlso <" PLUG_NAME ".ttl> .\n");
#ifdef LV2_WITH_UI
    fprintf(f, "\n<" PLUG_UI_URI ">\n"
               "  a ui:X11UI ;\n"
               "  lv2:binary <" PLUG_NAME "_ui.so> ;\n"
               "  rdfs:seeAlso <" PLUG_NAME ".ttl> .\n");
#endif
    fclose(f);
    return 0;
  }
  return 1;
}

int IPlugLV2DSP::write_also()
{
  char fname[PATH_MAX];
  snprintf(fname, PATH_MAX, "%s.ttl", PLUG_NAME);
  unlink(fname);
  FILE *f = fopen(fname, "w");
  if(f)
  {
    fprintf(f, "@prefix doap:  <http://usefulinc.com/ns/doap#> .\n"
               "@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .\n"
               "@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
               "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n"
               "@prefix units: <http://lv2plug.in/ns/extensions/units#> .\n"
#ifdef LV2_WITH_UI
               "@prefix ui:   <http://lv2plug.in/ns/extensions/ui#> .\n"
               "@prefix urid:  <http://lv2plug.in/ns/ext/urid#> .\n"
#endif
               "\n");
               
    fprintf(f, "<" PLUG_URI ">\n"
               "  a lv2:Plugin ;\n" // TODO: type, f.e. lv2:AmplifierPlugin ;
               "  lv2:project <" PLUG_URL_STR "> ;\n"
               "   doap:name \"" PLUG_NAME "\" ;\n" // TODO: where can we get human readable name ?
               // TODO: licese, f.e. doap:license <https://github.com/iPlug2/iPlug2/blob/master/LICENSE.txt> ;
               "  lv2:optionalFeature lv2:hardRTCapable ;\n"
#ifdef LV2_WITH_UI
               "  ui:ui <" PLUG_UI_URI "> ;\n"
#endif
               "  lv2:port [\n");
    
    int nInputs = MaxNChannels(ERoute::kInput);
    int nOutputs = MaxNChannels(ERoute::kOutput);
    
    for (int n = 0; n < nInputs; ++n)
    {
      WDL_String name(GetChannelLabel(ERoute::kInput, n));
      if(!name.GetLength())
        name.SetFormatted(32, "In%d", n + 1);
      WDL_String symbol(name);
      if(!IsValidSymbol(symbol)) // spaces are not allowed
        symbol.SetFormatted(32, "in%d", n + 1);
      
      if(n)
        fprintf(f, " , [\n");
      fprintf(f, "    a lv2:AudioPort ,\n"
                 "      lv2:InputPort ;\n"
                 "    lv2:index %d ;\n"
                 "    lv2:symbol \"%s\" ;\n"
                 "    lv2:name \"%s\" ;\n"
                 "  ]", n, symbol.Get(), name.Get());
    }
    for (int n = 0; n < nOutputs; ++n)
    {
      WDL_String name(GetChannelLabel(ERoute::kOutput, n));
      if(!name.GetLength())
        name.SetFormatted(32, "Out%d", n + 1);
      WDL_String symbol(name);
      if(!IsValidSymbol(symbol)) // spaces are not allowed
        symbol.SetFormatted(32, "out%d", n + 1);
      
      if(nInputs + n)
        fprintf(f, " , [\n");
      fprintf(f, "    a lv2:AudioPort ,\n"
                 "      lv2:OutputPort ;\n"
                 "    lv2:index %d ;\n"
                 "    lv2:symbol \"%s\" ;\n"
                 "    lv2:name \"%s\" ;\n"
                 "  ]", nInputs + n, symbol.Get(), name.Get());
    }
    // TODO: replace with LV2 parameters (may be as an option...)
    //       with UI notification
    // TODO: add MIDI In/Out
    int nParams = NParams();
    for (int n = 0; n < nParams; ++n)
    {
      IParam *p = GetParam(n);

      WDL_String symbol(p->GetNameForHost());
      if(!IsValidSymbol(symbol)) // spaces are not allowed
        symbol.SetFormatted(32, "Par%d", n + 1);

      if(nInputs + nOutputs + n)
        fprintf(f, " , [\n");
      fprintf(f, "    a lv2:InputPort ,\n"
                 "      lv2:ControlPort ;\n"
                 "    lv2:index %d ;\n"
                 "    lv2:symbol \"%s\" ;\n"
                 "    lv2:name \"%s\" ;\n"
                 "    lv2:default %.2f ;\n"
                 "    lv2:minimum %.2f ;\n"
                 "    lv2:maximum %.2f ;\n"
                 "  ]", nInputs + nOutputs + n, p->GetNameForHost(), symbol.Get(),
                        p->GetDefault(), p->GetMin(), p->GetMax()
             ); // TODO: units
    }
    fprintf(f, " .\n");

#ifdef LV2_WITH_UI
    fprintf(f, "\n<" PLUG_UI_URI ">\n"
               "  a ui:X11UI ;\n"
               "  lv2:optionalFeature ui:parent , ui:resize ;\n"
               "  lv2:requiredFeature ui:idleInterface ;\n"
               "  lv2:extensionData   ui:idleInterface .\n"
           );
#endif    
               /*
        lv2:port [
                a lv2:InputPort ,
                        lv2:ControlPort ;
                lv2:index 0 ;
                lv2:symbol "gain" ;
                lv2:name "Gain" ;
                lv2:default 0.0 ;
                lv2:minimum -90.0 ;
                lv2:maximum 24.0 ;
                units:unit units:db ;
        ] , [
                a lv2:AudioPort ,
                        lv2:InputPort ;
                lv2:index 1 ;
                lv2:symbol "in" ;
                lv2:name "In"
       ] , [
                a lv2:AudioPort ,
                        lv2:OutputPort ;
                lv2:index 2 ;
                lv2:symbol "out" ;
                lv2:name "Out"
        ] .
               */
               
               
    fclose(f);
    return 0;
  }
  return 1;
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

}
