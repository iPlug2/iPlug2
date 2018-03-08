#include "IPlugWAM.h"

const char* IPlugWAM::init(uint32_t bufsize, uint32_t sr, void* desc)
{
  wam_logs("init\n");
  
  SetSampleRate(sr);
  SetBlockSize(bufsize);
  
  WDL_String json;
  json.Set("{\n");
  json.AppendFormatted(8192, "\"audio\": { \"inputs\": [{ \"id\":0, \"channels\":0 }], \"outputs\": [{ \"id\":0, \"channels\":2 }] },\n");
  json.AppendFormatted(8192, "\"parameters\": [\n");
  
  for (int idx = 0; idx < NParams(); idx++)
  {
    IParam* pParam = GetParam(idx);
    json.AppendFormatted(8192, "{");
    json.AppendFormatted(8192, "\"id\":%i, ", idx);
    json.AppendFormatted(8192, "\"name\":\"%s\", ", pParam->GetNameForHost());
    switch (pParam->Type())
    {
      case IParam::kTypeNone:
        break;
      case IParam::kTypeBool:
        json.AppendFormatted(8192, "\"type\":\"%s\", ", "bool");
        break;
      case IParam::kTypeInt:
        json.AppendFormatted(8192, "\"type\":\"%s\", ", "int");
        break;
      case IParam::kTypeEnum:
        json.AppendFormatted(8192, "\"type\":\"%s\", ", "enum");
        break;
      case IParam::kTypeDouble:
        json.AppendFormatted(8192, "\"type\":\"%s\", ", "float");
        break;
      default:
        break;
    }
    json.AppendFormatted(8192, "\"min\":%f, ", pParam->GetMin());
    json.AppendFormatted(8192, "\"max\":%f, ", pParam->GetMax());
    json.AppendFormatted(8192, "\"default\":%f, ", pParam->GetDefault());
    json.AppendFormatted(8192, "\"rate\":\"audio\"");
    json.AppendFormatted(8192, "}");
    
    if(idx < NParams()-1)
      json.AppendFormatted(8192, ",\n");
    else
      json.AppendFormatted(8192, "\n");
    
    onParam(idx, pParam->Value());
  }
  
  json.Append("]\n}");
  
  wam_logs(json.Get());
  
  //TODO: correct place?
//  Reset();
  
  return json.Get();
}

void IPlugWAM::onProcess(WAM::AudioBus* audio, void* data)
{
  AttachInputBuffers(0, NInChannels(), audio->inputs, GetBlockSize());
  AttachOutputBuffers(0, NOutChannels(), audio->outputs);
  ProcessBuffers((float) 0.0f, GetBlockSize());
}

void IPlugWAM::onMidi(byte status, byte data1, byte data2)
{
  wam_logs("onMidi\n");
}

void IPlugWAM::onParam(uint32_t idparam, double value)
{
  wam_logs("onParam\n");
  
  GetParam(idparam)->Set(value);
  OnParamChange(idparam);
}


