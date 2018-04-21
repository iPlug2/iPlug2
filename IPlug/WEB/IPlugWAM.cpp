#include "IPlugWAM.h"

IPlugWAM::IPlugWAM(IPlugInstanceInfo instanceInfo, IPlugConfig c)
  : IPlugBase(c, kAPIWAM)
  , IPlugProcessor<float>(c, kAPIWAM)
{
  int nInputs = MaxNChannels(ERoute::kInput), nOutputs = MaxNChannels(ERoute::kOutput);

  _SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  _SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);
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
  }

  json.Append("]\n}");

  //TODO: correct place? - do we need a WAM reset message?
  OnReset();

  return json.Get();
}

void IPlugWAM::onProcess(WAM::AudioBus* pAudio, void* pData)
{
//  DBGMSG("onProcess\n");
  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false); //TODO: go elsewhere - enable inputs
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true); //TODO: go elsewhere
  _AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), pAudio->inputs, GetBlockSize());
  _AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), pAudio->outputs, GetBlockSize());
  _ProcessBuffers((float) 0.0f, GetBlockSize());
}

void IPlugWAM::onMidi(byte status, byte data1, byte data2)
{
//   DBGMSG("onMidi\n");
  IMidiMsg msg = {0 /* TODO:what about offset?*/, status, data1, data2};
  ProcessMidiMsg(msg);
}

void IPlugWAM::onParam(uint32_t idparam, double value)
{
//  DBGMSG("IPlugWAM:: onParam %i %f\n", idparam, value);
  SetParameterValue(idparam, value);
}

void IPlugWAM::onSysex(byte* msg, uint32_t size)
{
  ISysEx sysex = {0 /* TODO:what about offset?*/, msg, (int) size };
  ProcessSysEx(sysex);
}

