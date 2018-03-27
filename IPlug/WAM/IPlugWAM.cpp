#include "IPlugWAM.h"

IPlugWAM::IPlugWAM(IPlugInstanceInfo instanceInfo, IPlugConfig c)
  : IPLUG_BASE_CLASS(c, kAPIWAM)
  , IPlugProcessor<float>(c, kAPIWAM)
{
}

const char* IPlugWAM::init(uint32_t bufsize, uint32_t sr, void* pDesc)
{
  DBGMSG("init\n");

  _SetSampleRate(sr);
  _SetBlockSize(bufsize);

  DBGMSG("%i %i\n", sr, bufsize);


  WDL_String json;
  json.Set("{\n");
  json.AppendFormatted(8192, "\"audio\": { \"inputs\": [{ \"id\":0, \"channels\":%i }], \"outputs\": [{ \"id\":0, \"channels\":%i }] },\n", MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
  json.AppendFormatted(8192, "\"parameters\": [\n");

  for (int idx = 0; idx < NParams(); idx++)
  {
    IParam* pParam = GetParam(idx);
    pParam->GetJSON(json, idx);

    if(idx < NParams()-1)
      json.AppendFormatted(8192, ",\n");
    else
      json.AppendFormatted(8192, "\n");

    onParam(idx, pParam->Value());
  }

  json.Append("]\n}");

  DBGMSG("%s\n", json.Get());

  //TODO: correct place?
//   OnReset();

  return json.Get();
}

void IPlugWAM::onProcess(WAM::AudioBus* pAudio, void* pData)
{
  DBGMSG("Audio\n");

//   _AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), pAudio->inputs, GetBlockSize());
//   _AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), pAudio->outputs, GetBlockSize());
//   _ProcessBuffers((float) 0.0f, GetBlockSize());
}

//void IPlugWAM::onMidi(byte status, byte data1, byte data2)
//{
//  DBGMSG("onMidi\n");
//}
//
//void IPlugWAM::onParam(uint32_t idparam, double value)
//{
//  DBGMSG("onParam\n");
//
//  GetParam(idparam)->Set(value);
//  OnParamChange(idparam);
//}


