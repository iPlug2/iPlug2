/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <memory>

#ifndef NO_IGRAPHICS
  #include "IPlugCLI_ScreenshotRenderer.h"
  using CLIPluginType = iplug::IPlugCLI_ScreenshotRenderer;
#else
  #include "IPlugCLI.h"
  using CLIPluginType = iplug::IPlugCLI;
#endif

namespace iplug {
  CLIPluginType* MakePlug(const InstanceInfo& info);
}

#include "IPlugPaths.h"
#include "config.h"
#include "wavread.h"
#include "wavwrite.h"

using namespace iplug;

// Simple argument parser
class ArgParser
{
public:
  ArgParser(int argc, char** argv) : mArgc(argc), mArgv(argv), mIndex(1) {}

  bool hasNext() const { return mIndex < mArgc; }

  const char* next()
  {
    if (mIndex < mArgc)
      return mArgv[mIndex++];
    return nullptr;
  }

  const char* peek() const
  {
    if (mIndex < mArgc)
      return mArgv[mIndex];
    return nullptr;
  }

  bool match(const char* flag)
  {
    if (mIndex < mArgc && strcmp(mArgv[mIndex], flag) == 0)
    {
      mIndex++;
      return true;
    }
    return false;
  }

  int getInt(int defaultVal = 0)
  {
    const char* arg = next();
    if (arg)
      return atoi(arg);
    return defaultVal;
  }

  double getDouble(double defaultVal = 0.0)
  {
    const char* arg = next();
    if (arg)
      return atof(arg);
    return defaultVal;
  }

private:
  int mArgc;
  char** mArgv;
  int mIndex;
};

void printUsage(const char* progName)
{
  fprintf(stderr, "Usage: %s [options]\n", progName);
  fprintf(stderr, "\nInformation:\n");
  fprintf(stderr, "  --help              Show this help message\n");
  fprintf(stderr, "  --info              Print plugin info (JSON)\n");
  fprintf(stderr, "  --params            Print all parameters (JSON)\n");
  fprintf(stderr, "  --param <idx>       Print single parameter (JSON)\n");
  fprintf(stderr, "\nConfiguration:\n");
  fprintf(stderr, "  --sr <rate>         Set sample rate (default: 44100, or from input file)\n");
  fprintf(stderr, "  --bs <size>         Set block size (default: 512)\n");
  fprintf(stderr, "  --set <idx> <val>   Set parameter by index to value\n");
  fprintf(stderr, "  --set-name <name> <val>  Set parameter by name to value\n");
  fprintf(stderr, "  --set-norm <idx> <val>   Set parameter by index to normalized value (0-1)\n");
  fprintf(stderr, "  --load-params <file.json>  Load parameters from JSON file\n");
  fprintf(stderr, "  --save-params <file.json>  Save parameters to JSON file\n");
  fprintf(stderr, "  --load-state <file.bin>    Load full plugin state from binary file\n");
  fprintf(stderr, "  --save-state <file.bin>    Save full plugin state to binary file\n");
  fprintf(stderr, "\nFile I/O:\n");
  fprintf(stderr, "  --input <file.wav>  Input WAV file\n");
  fprintf(stderr, "  --output <file.wav> Output WAV file\n");
  fprintf(stderr, "  --process-file      Process entire input file through plugin\n");
  fprintf(stderr, "\nProcessing:\n");
  fprintf(stderr, "  --process <frames>  Process N frames of silence\n");
  fprintf(stderr, "  --impulse <length>  Generate impulse response (default length: 4096)\n");
  fprintf(stderr, "  --render <ms>       Render N milliseconds of audio\n");
  fprintf(stderr, "\nTest Signals:\n");
  fprintf(stderr, "  --sine <freq> <ms>              Generate sine wave at freq Hz for duration\n");
  fprintf(stderr, "  --noise <ms>                    Generate white noise for duration\n");
  fprintf(stderr, "  --step <ms>                     Generate unit step response for duration\n");
  fprintf(stderr, "  --chirp <start> <end> <ms>      Generate log sweep from start to end Hz\n");
  fprintf(stderr, "\nMIDI (queue events, then use --render):\n");
  fprintf(stderr, "  --midi <note> <vel> <start_ms> <dur_ms>  Queue MIDI note\n");
  fprintf(stderr, "  --midi-cc <cc> <val> [time_ms]    Queue CC message (val: 0-127, time: default 0)\n");
  fprintf(stderr, "  --midi-bend <val> [time_ms]       Queue pitch bend (val: -1.0 to 1.0, time: default 0)\n");
  fprintf(stderr, "  --midi-pc <program> [time_ms]     Queue program change (time: default 0)\n");
  fprintf(stderr, "\nText Output:\n");
  fprintf(stderr, "  --output-txt <file> Output as text (one sample per line)\n");
#ifndef NO_IGRAPHICS
  fprintf(stderr, "\nUI Screenshot (requires IGraphics build):\n");
  fprintf(stderr, "  --screenshot <file.png>  Render UI to PNG image file\n");
  fprintf(stderr, "  --scale <factor>         Screenshot scale factor (default: 1.0)\n");
  fprintf(stderr, "  --resources <path>       Path to resources directory (fonts, images)\n");
#endif
}

void printInfo(CLIPluginType* pPlugin)
{
  const char* typeStr = "effect";
  if (pPlugin->IsInstrument())
    typeStr = "instrument";
  else if (pPlugin->DoesMIDIIn() && pPlugin->DoesMIDIOut())
    typeStr = "midi_effect";

  printf("{\n");
  printf("  \"name\": \"%s\",\n", pPlugin->GetPluginName());
  printf("  \"version\": \"%d.%d.%d\",\n",
         (pPlugin->GetPluginVersion(false) >> 16) & 0xFF,
         (pPlugin->GetPluginVersion(false) >> 8) & 0xFF,
         pPlugin->GetPluginVersion(false) & 0xFF);
  printf("  \"manufacturer\": \"%s\",\n", pPlugin->GetMfrName());
  printf("  \"type\": \"%s\",\n", typeStr);
  printf("  \"channels\": {\n");
  printf("    \"inputs\": %d,\n", pPlugin->NInChannels());
  printf("    \"outputs\": %d\n", pPlugin->NOutChannels());
  printf("  },\n");
  printf("  \"latency\": %d,\n", pPlugin->GetLatency());
  printf("  \"midi_in\": %s,\n", pPlugin->DoesMIDIIn() ? "true" : "false");
  printf("  \"midi_out\": %s\n", pPlugin->DoesMIDIOut() ? "true" : "false");
  printf("}\n");
}

void printParams(CLIPluginType* pPlugin)
{
  printf("{\n");
  printf("  \"parameters\": [\n");

  int nParams = pPlugin->NParams();
  for (int i = 0; i < nParams; i++)
  {
    const IParam* pParam = pPlugin->GetParam(i);
    printf("    {\n");
    printf("      \"index\": %d,\n", i);
    printf("      \"name\": \"%s\",\n", pParam->GetName());
    printf("      \"value\": %g,\n", pParam->Value());
    printf("      \"min\": %g,\n", pParam->GetMin());
    printf("      \"max\": %g,\n", pParam->GetMax());
    printf("      \"default\": %g,\n", pParam->GetDefault());
    printf("      \"label\": \"%s\"\n", pParam->GetLabel());
    printf("    }%s\n", (i < nParams - 1) ? "," : "");
  }

  printf("  ]\n");
  printf("}\n");
}

void printParam(CLIPluginType* pPlugin, int idx)
{
  if (idx < 0 || idx >= pPlugin->NParams())
  {
    fprintf(stderr, "Error: Parameter index %d out of range (0-%d)\n", idx, pPlugin->NParams() - 1);
    return;
  }

  const IParam* pParam = pPlugin->GetParam(idx);
  printf("{\n");
  printf("  \"index\": %d,\n", idx);
  printf("  \"name\": \"%s\",\n", pParam->GetName());
  printf("  \"value\": %g,\n", pParam->Value());
  printf("  \"min\": %g,\n", pParam->GetMin());
  printf("  \"max\": %g,\n", pParam->GetMax());
  printf("  \"default\": %g,\n", pParam->GetDefault());
  printf("  \"label\": \"%s\"\n", pParam->GetLabel());
  printf("}\n");
}

void outputText(const std::vector<double>& data, const char* filename)
{
  FILE* f = fopen(filename, "w");
  if (!f)
  {
    fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
    return;
  }

  for (size_t i = 0; i < data.size(); i++)
  {
    fprintf(f, "%.15g\n", data[i]);
  }

  fclose(f);
  fprintf(stderr, "Wrote %zu samples to %s\n", data.size(), filename);
}

int findParamByName(CLIPluginType* pPlugin, const char* name)
{
  int nParams = pPlugin->NParams();
  for (int i = 0; i < nParams; i++)
  {
    if (strcmp(pPlugin->GetParam(i)->GetName(), name) == 0)
      return i;
  }
  return -1;
}

void saveParams(CLIPluginType* pPlugin, const char* filename)
{
  FILE* f = fopen(filename, "w");
  if (!f)
  {
    fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
    return;
  }

  fprintf(f, "{\n");
  fprintf(f, "  \"parameters\": [\n");

  int nParams = pPlugin->NParams();
  for (int i = 0; i < nParams; i++)
  {
    const IParam* pParam = pPlugin->GetParam(i);
    fprintf(f, "    {\"index\": %d, \"name\": \"%s\", \"value\": %g}%s\n",
            i, pParam->GetName(), pParam->Value(),
            (i < nParams - 1) ? "," : "");
  }

  fprintf(f, "  ]\n");
  fprintf(f, "}\n");

  fclose(f);
  fprintf(stderr, "Saved %d parameters to %s\n", nParams, filename);
}

void saveState(CLIPluginType* pPlugin, const char* filename)
{
  IByteChunk chunk;
  if (pPlugin->SerializeState(chunk))
  {
    FILE* f = fopen(filename, "wb");
    if (!f)
    {
      fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
      return;
    }
    fwrite(chunk.GetData(), 1, chunk.Size(), f);
    fclose(f);
    fprintf(stderr, "Saved plugin state (%d bytes) to %s\n", chunk.Size(), filename);
  }
  else
  {
    fprintf(stderr, "Error: Failed to serialize plugin state\n");
  }
}

bool loadState(CLIPluginType* pPlugin, const char* filename)
{
  FILE* f = fopen(filename, "rb");
  if (!f)
  {
    fprintf(stderr, "Error: Could not open file %s for reading\n", filename);
    return false;
  }

  // Get file size
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  // Read into chunk
  IByteChunk chunk;
  chunk.Resize(static_cast<int>(size));
  fread(chunk.GetData(), 1, size, f);
  fclose(f);

  // Unserialize
  int pos = pPlugin->UnserializeState(chunk, 0);
  if (pos > 0)
  {
    fprintf(stderr, "Loaded plugin state (%ld bytes) from %s\n", size, filename);
    return true;
  }
  else
  {
    fprintf(stderr, "Error: Failed to unserialize plugin state\n");
    return false;
  }
}

bool loadParams(CLIPluginType* pPlugin, const char* filename)
{
  FILE* f = fopen(filename, "r");
  if (!f)
  {
    fprintf(stderr, "Error: Could not open file %s for reading\n", filename);
    return false;
  }

  // Read entire file
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  std::string content(size, '\0');
  fread(&content[0], 1, size, f);
  fclose(f);

  // Simple JSON parser for our format
  // Look for patterns like "index": N, "value": V or "name": "X", "value": V
  int paramsLoaded = 0;
  size_t pos = 0;

  while ((pos = content.find("\"index\"", pos)) != std::string::npos)
  {
    // Find index value
    size_t colonPos = content.find(':', pos);
    if (colonPos == std::string::npos) break;

    int idx = atoi(content.c_str() + colonPos + 1);

    // Find corresponding value
    size_t valuePos = content.find("\"value\"", colonPos);
    if (valuePos == std::string::npos) break;

    size_t valueColonPos = content.find(':', valuePos);
    if (valueColonPos == std::string::npos) break;

    double val = atof(content.c_str() + valueColonPos + 1);

    if (idx >= 0 && idx < pPlugin->NParams())
    {
      pPlugin->GetParam(idx)->Set(val);
      pPlugin->OnParamChange(idx);
      paramsLoaded++;
    }

    pos = valueColonPos + 1;
  }

  fprintf(stderr, "Loaded %d parameters from %s\n", paramsLoaded, filename);
  return paramsLoaded > 0;
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    printUsage(argv[0]);
    return 1;
  }

  // Create plugin instance
  InstanceInfo info = { argc, argv };
  std::unique_ptr<CLIPluginType> plugin(MakePlug(info));

  if (!plugin)
  {
    fprintf(stderr, "Error: Failed to create plugin instance\n");
    return 1;
  }

  // Default settings
  double sampleRate = 44100.0;
  int blockSize = 512;
  bool sampleRateSet = false;
  std::string inputWavFile;
  std::string outputWavFile;
  std::string outputTxtFile;
  std::vector<double> outputData;
  std::vector<std::vector<double>> outputChannels;
  int outputSampleRate = 44100;
#ifndef NO_IGRAPHICS
  std::string screenshotFile;
  float screenshotScale = 1.0f;
#endif

  ArgParser args(argc, argv);

  while (args.hasNext())
  {
    if (args.match("--help") || args.match("-h"))
    {
      printUsage(argv[0]);
      return 0;
    }
    else if (args.match("--info"))
    {
      printInfo(plugin.get());
      return 0;
    }
    else if (args.match("--params"))
    {
      printParams(plugin.get());
      return 0;
    }
    else if (args.match("--param"))
    {
      int idx = args.getInt(-1);
      printParam(plugin.get(), idx);
      return 0;
    }
    else if (args.match("--sr"))
    {
      sampleRate = args.getDouble(44100.0);
      sampleRateSet = true;
    }
    else if (args.match("--input"))
    {
      const char* file = args.next();
      if (file)
        inputWavFile = file;
    }
    else if (args.match("--output"))
    {
      const char* file = args.next();
      if (file)
        outputWavFile = file;
    }
    else if (args.match("--process-file"))
    {
      if (inputWavFile.empty())
      {
        fprintf(stderr, "Error: --process-file requires --input to be specified first\n");
        return 1;
      }

      WaveReader reader;
      if (!reader.Open(inputWavFile.c_str()))
      {
        fprintf(stderr, "Error: Could not open input file: %s\n", inputWavFile.c_str());
        return 1;
      }

      int inChannels = reader.get_nch();
      int inSampleRate = reader.get_srate();
      unsigned int totalFrames = reader.GetLength();

      // Use input file sample rate unless explicitly overridden
      if (!sampleRateSet)
        sampleRate = inSampleRate;
      outputSampleRate = static_cast<int>(sampleRate);

      fprintf(stderr, "Input: %s (%d ch, %d Hz, %u frames)\n",
              inputWavFile.c_str(), inChannels, inSampleRate, totalFrames);

      plugin->SetupProcessing(sampleRate, blockSize);

      int nInCh = plugin->NInChannels();
      int nOutCh = plugin->NOutChannels();

      // Allocate input buffers and read entire file
      std::vector<std::vector<double>> inputBuffers(nInCh);
      std::vector<double*> inputs(nInCh);
      for (int c = 0; c < nInCh; c++)
      {
        inputBuffers[c].resize(totalFrames, 0.0);
        inputs[c] = inputBuffers[c].data();
      }

      // Read input file (non-interleaved)
      reader.ReadDoublesNI(inputs.data(), 0, totalFrames, nInCh);
      reader.Close();

      // Allocate output buffers
      outputChannels.resize(nOutCh);
      std::vector<double*> outputs(nOutCh);
      for (int c = 0; c < nOutCh; c++)
      {
        outputChannels[c].resize(totalFrames, 0.0);
        outputs[c] = outputChannels[c].data();
      }

      // Process in blocks
      unsigned int framesProcessed = 0;
      while (framesProcessed < totalFrames)
      {
        int framesToProcess = std::min(blockSize, static_cast<int>(totalFrames - framesProcessed));

        std::vector<double*> inPtrs(nInCh);
        std::vector<double*> outPtrs(nOutCh);
        for (int c = 0; c < nInCh; c++)
          inPtrs[c] = inputs[c] + framesProcessed;
        for (int c = 0; c < nOutCh; c++)
          outPtrs[c] = outputs[c] + framesProcessed;

        plugin->Process(inPtrs.data(), outPtrs.data(), framesToProcess);
        framesProcessed += framesToProcess;
      }

      // Store first channel for text output
      if (nOutCh > 0)
        outputData = outputChannels[0];

      fprintf(stderr, "Processed %u frames\n", totalFrames);
    }
    else if (args.match("--bs"))
    {
      blockSize = args.getInt(512);
    }
    else if (args.match("--set"))
    {
      int idx = args.getInt(-1);
      double val = args.getDouble(0.0);
      if (idx >= 0 && idx < plugin->NParams())
      {
        plugin->GetParam(idx)->Set(val);
        plugin->OnParamChange(idx);
        fprintf(stderr, "Set param %d to %g\n", idx, val);
      }
      else
      {
        fprintf(stderr, "Error: Invalid parameter index %d\n", idx);
      }
    }
    else if (args.match("--set-name"))
    {
      const char* name = args.next();
      double val = args.getDouble(0.0);
      if (name)
      {
        int idx = findParamByName(plugin.get(), name);
        if (idx >= 0)
        {
          plugin->GetParam(idx)->Set(val);
          plugin->OnParamChange(idx);
          fprintf(stderr, "Set param \"%s\" (idx %d) to %g\n", name, idx, val);
        }
        else
        {
          fprintf(stderr, "Error: Parameter \"%s\" not found\n", name);
        }
      }
    }
    else if (args.match("--set-norm"))
    {
      int idx = args.getInt(-1);
      double normVal = args.getDouble(0.0);
      if (idx >= 0 && idx < plugin->NParams())
      {
        plugin->GetParam(idx)->SetNormalized(normVal);
        plugin->OnParamChange(idx);
        fprintf(stderr, "Set param %d to normalized %g (value: %g)\n",
                idx, normVal, plugin->GetParam(idx)->Value());
      }
      else
      {
        fprintf(stderr, "Error: Invalid parameter index %d\n", idx);
      }
    }
    else if (args.match("--load-params"))
    {
      const char* file = args.next();
      if (file)
        loadParams(plugin.get(), file);
    }
    else if (args.match("--save-params"))
    {
      const char* file = args.next();
      if (file)
        saveParams(plugin.get(), file);
    }
    else if (args.match("--load-state"))
    {
      const char* file = args.next();
      if (file)
        loadState(plugin.get(), file);
    }
    else if (args.match("--save-state"))
    {
      const char* file = args.next();
      if (file)
        saveState(plugin.get(), file);
    }
    else if (args.match("--output-txt"))
    {
      const char* file = args.next();
      if (file)
        outputTxtFile = file;
    }
    else if (args.match("--process"))
    {
      int nFrames = args.getInt(44100);
      plugin->SetupProcessing(sampleRate, blockSize);

      int nOutCh = plugin->NOutChannels();
      std::vector<std::vector<double>> outputBuffers(nOutCh);
      std::vector<double*> outputs(nOutCh);
      for (int c = 0; c < nOutCh; c++)
      {
        outputBuffers[c].resize(nFrames, 0.0);
        outputs[c] = outputBuffers[c].data();
      }

      plugin->ProcessSilence(nFrames, outputs.data());

      // Store first channel output
      if (nOutCh > 0)
        outputData = outputBuffers[0];

      fprintf(stderr, "Processed %d frames at %.0f Hz\n", nFrames, sampleRate);
    }
    else if (args.match("--impulse"))
    {
      int length = args.getInt(4096);
      plugin->SetupProcessing(sampleRate, blockSize);
      plugin->ProcessImpulse(length, outputData);
      fprintf(stderr, "Generated impulse response (%d samples)\n", length);
    }
    else if (args.match("--sine"))
    {
      double freq = args.getDouble(1000.0);
      int ms = args.getInt(100);
      plugin->SetupProcessing(sampleRate, blockSize);
      int nFrames = static_cast<int>((ms / 1000.0) * sampleRate);
      plugin->ProcessSine(freq, nFrames, outputData);
      fprintf(stderr, "Generated sine wave (%.1f Hz, %d ms, %d samples)\n", freq, ms, nFrames);
    }
    else if (args.match("--noise"))
    {
      int ms = args.getInt(100);
      plugin->SetupProcessing(sampleRate, blockSize);
      int nFrames = static_cast<int>((ms / 1000.0) * sampleRate);
      plugin->ProcessNoise(nFrames, outputData);
      fprintf(stderr, "Generated white noise (%d ms, %d samples)\n", ms, nFrames);
    }
    else if (args.match("--step"))
    {
      int ms = args.getInt(100);
      plugin->SetupProcessing(sampleRate, blockSize);
      int nFrames = static_cast<int>((ms / 1000.0) * sampleRate);
      plugin->ProcessStep(nFrames, outputData);
      fprintf(stderr, "Generated step response (%d ms, %d samples)\n", ms, nFrames);
    }
    else if (args.match("--chirp"))
    {
      double startFreq = args.getDouble(20.0);
      double endFreq = args.getDouble(20000.0);
      int ms = args.getInt(1000);
      plugin->SetupProcessing(sampleRate, blockSize);
      int nFrames = static_cast<int>((ms / 1000.0) * sampleRate);
      plugin->ProcessChirp(startFreq, endFreq, nFrames, outputData);
      fprintf(stderr, "Generated chirp (%.1f-%.1f Hz, %d ms, %d samples)\n", startFreq, endFreq, ms, nFrames);
    }
    else if (args.match("--midi"))
    {
      int note = args.getInt(60);
      int vel = args.getInt(100);
      int startMs = args.getInt(0);
      int durMs = args.getInt(500);

      // Calculate frame offsets
      int startFrame = static_cast<int>((startMs / 1000.0) * sampleRate);
      int durFrames = static_cast<int>((durMs / 1000.0) * sampleRate);

      // Queue note on
      IMidiMsg noteOn;
      noteOn.MakeNoteOnMsg(note, vel, 0, startFrame);
      plugin->QueueMidiMsg(noteOn);

      // Queue note off
      IMidiMsg noteOff;
      noteOff.MakeNoteOffMsg(note, 0, startFrame + durFrames);
      plugin->QueueMidiMsg(noteOff);

      fprintf(stderr, "Queued MIDI note %d vel %d at %d ms for %d ms\n",
              note, vel, startMs, durMs);
    }
    else if (args.match("--midi-cc"))
    {
      int cc = args.getInt(1);
      int val = args.getInt(64);
      int timeMs = args.getInt(0);

      int frameOffset = static_cast<int>((timeMs / 1000.0) * sampleRate);

      IMidiMsg msg;
      msg.MakeControlChangeMsg(static_cast<IMidiMsg::EControlChangeMsg>(cc), val / 127.0, 0, frameOffset);
      plugin->QueueMidiMsg(msg);

      fprintf(stderr, "Queued MIDI CC %d = %d at %d ms\n", cc, val, timeMs);
    }
    else if (args.match("--midi-bend"))
    {
      double val = args.getDouble(0.0);
      int timeMs = args.getInt(0);

      int frameOffset = static_cast<int>((timeMs / 1000.0) * sampleRate);

      IMidiMsg msg;
      msg.MakePitchWheelMsg(val, 0, frameOffset);
      plugin->QueueMidiMsg(msg);

      fprintf(stderr, "Queued MIDI pitch bend %.3f at %d ms\n", val, timeMs);
    }
    else if (args.match("--midi-pc"))
    {
      int program = args.getInt(0);
      int timeMs = args.getInt(0);

      int frameOffset = static_cast<int>((timeMs / 1000.0) * sampleRate);

      IMidiMsg msg;
      msg.MakeProgramChange(program, 0, frameOffset);
      plugin->QueueMidiMsg(msg);

      fprintf(stderr, "Queued MIDI program change %d at %d ms\n", program, timeMs);
    }
    else if (args.match("--render"))
    {
      int ms = args.getInt(1000);
      plugin->SetupProcessing(sampleRate, blockSize);

      int nFrames = static_cast<int>((ms / 1000.0) * sampleRate);
      int nOutCh = plugin->NOutChannels();

      std::vector<std::vector<double>> outputBuffers(nOutCh);
      std::vector<double*> outputs(nOutCh);
      for (int c = 0; c < nOutCh; c++)
      {
        outputBuffers[c].resize(nFrames, 0.0);
        outputs[c] = outputBuffers[c].data();
      }

      plugin->ProcessSilence(nFrames, outputs.data());

      if (nOutCh > 0)
        outputData = outputBuffers[0];

      fprintf(stderr, "Rendered %d ms (%d frames)\n", ms, nFrames);
    }
#ifndef NO_IGRAPHICS
    else if (args.match("--screenshot"))
    {
      const char* file = args.next();
      if (file)
        screenshotFile = file;
    }
    else if (args.match("--scale"))
    {
      screenshotScale = static_cast<float>(args.getDouble(1.0));
    }
    else if (args.match("--resources"))
    {
      const char* path = args.next();
      if (path)
      {
        SetResourceBasePath(path);
        fprintf(stderr, "Set resource path: %s\n", path);
      }
    }
#endif
    else
    {
      fprintf(stderr, "Unknown option: %s\n", args.next());
      printUsage(argv[0]);
      return 1;
    }
  }

  // Output results if requested
  if (!outputTxtFile.empty() && !outputData.empty())
  {
    outputText(outputData, outputTxtFile.c_str());
  }

  // Write WAV output if requested
  if (!outputWavFile.empty() && !outputChannels.empty())
  {
    int nOutCh = static_cast<int>(outputChannels.size());
    unsigned int nFrames = static_cast<unsigned int>(outputChannels[0].size());

    WaveWriter writer(outputWavFile.c_str(), 24, nOutCh, outputSampleRate, 0);

    // WaveWriter expects interleaved samples, so we need to interleave
    std::vector<double> interleaved(nFrames * nOutCh);
    for (unsigned int f = 0; f < nFrames; f++)
    {
      for (int c = 0; c < nOutCh; c++)
      {
        interleaved[f * nOutCh + c] = outputChannels[c][f];
      }
    }

    // Convert to 24-bit and write
    std::vector<unsigned char> buf(nFrames * nOutCh * 3);
    for (unsigned int i = 0; i < nFrames * nOutCh; i++)
    {
      // Clamp and convert to 24-bit
      double sample = interleaved[i];
      if (sample > 1.0) sample = 1.0;
      if (sample < -1.0) sample = -1.0;
      int val = static_cast<int>(sample * 8388607.0); // 2^23 - 1
      buf[i * 3 + 0] = static_cast<unsigned char>(val & 0xFF);
      buf[i * 3 + 1] = static_cast<unsigned char>((val >> 8) & 0xFF);
      buf[i * 3 + 2] = static_cast<unsigned char>((val >> 16) & 0xFF);
    }

    writer.WriteRaw(buf.data(), buf.size());

    fprintf(stderr, "Wrote %s (%d ch, %d Hz, %u frames)\n",
            outputWavFile.c_str(), nOutCh, outputSampleRate, nFrames);
  }
  else if (!outputWavFile.empty() && !outputData.empty())
  {
    // Single channel output from impulse/process commands
    unsigned int nFrames = static_cast<unsigned int>(outputData.size());

    WaveWriter writer(outputWavFile.c_str(), 24, 1, outputSampleRate, 0);

    std::vector<unsigned char> buf(nFrames * 3);
    for (unsigned int i = 0; i < nFrames; i++)
    {
      double sample = outputData[i];
      if (sample > 1.0) sample = 1.0;
      if (sample < -1.0) sample = -1.0;
      int val = static_cast<int>(sample * 8388607.0);
      buf[i * 3 + 0] = static_cast<unsigned char>(val & 0xFF);
      buf[i * 3 + 1] = static_cast<unsigned char>((val >> 8) & 0xFF);
      buf[i * 3 + 2] = static_cast<unsigned char>((val >> 16) & 0xFF);
    }

    writer.WriteRaw(buf.data(), buf.size());

    fprintf(stderr, "Wrote %s (1 ch, %d Hz, %u frames)\n",
            outputWavFile.c_str(), outputSampleRate, nFrames);
  }

#ifndef NO_IGRAPHICS
  if (!screenshotFile.empty())
  {
    if (!plugin->SaveScreenshot(screenshotFile.c_str(), screenshotScale))
    {
      return 1;
    }
  }
#endif

  return 0;
}
