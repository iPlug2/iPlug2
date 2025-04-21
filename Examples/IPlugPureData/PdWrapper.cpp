#include "PdWrapper.h"
#include "IPlugPureData.h"
#include <cstring>
#include <cstdlib>

// Static map to associate pd instances with IPlugPureData instances
#include <map>
static std::map<t_pdinstance*, IPlugPureData*> sPdInstanceMap;

PdWrapper::PdWrapper(IPlugPureData* plugin)
  : mPlugin(plugin)
  , mPdInstance(nullptr)
  , mPatchOpen(false)
  , mFromGuiHook(nullptr)
  , mFromPdHook(nullptr)
  , mLastTempo(-1.0)
  , mTicksSinceTempo(1000)
{
  mPatchFilename[0] = '\0';
  mPatchDir[0] = '\0';
}

PdWrapper::~PdWrapper()
{
  Cleanup();
}

void PdWrapper::Init()
{
  // Initialize libpd
  libpd_init();
  
  // Create a new pd instance
  mPdInstance = libpd_new_instance();
  
  // Add to the map
  sPdInstanceMap[mPdInstance] = mPlugin;
  
  // Set callbacks
  libpd_set_noteonhook(NoteOnCallback);
  libpd_set_messagehook(MessageCallback);
  
  // Set the current instance
  libpd_set_instance(mPdInstance);
  
  // Initialize audio with default channels and sample rate
  libpd_init_audio(2, 2, 48000);
  
  // Start DSP
  libpd_start_message(1);
  libpd_add_float(1.0f);
  libpd_finish_message("pd", "dsp");
  
  // Bind to pd and host objects
  mFromGuiHook = libpd_bind("pd");
  mFromPdHook = libpd_bind("host");
}

void PdWrapper::Cleanup()
{
  if (mPdInstance) {
    // Remove from the map
    sPdInstanceMap.erase(mPdInstance);
    
    // Free the instance
    libpd_set_instance(mPdInstance);
    libpd_free_instance(mPdInstance);
    mPdInstance = nullptr;
  }
}

void PdWrapper::OpenPatch(const char* filename, const char* dir)
{
  if (!mPdInstance) return;
  
  libpd_set_instance(mPdInstance);
  libpd_openfile(filename, dir);
  
  strncpy(mPatchFilename, filename, MAXPDSTRING - 1);
  mPatchFilename[MAXPDSTRING - 1] = '\0';
  
  strncpy(mPatchDir, dir, MAXPDSTRING - 1);
  mPatchDir[MAXPDSTRING - 1] = '\0';
  
  mPatchOpen = true;
}

void PdWrapper::ClosePatch()
{
  if (!mPdInstance || !mPatchOpen) return;
  
  libpd_set_instance(mPdInstance);
  libpd_closefile(mPatchFilename);
  
  mPatchOpen = false;
  mPatchFilename[0] = '\0';
  mPatchDir[0] = '\0';
}

void PdWrapper::SetParamValue(int paramIndex, float value)
{
  if (!mPdInstance) return;
  
  if (value < 0.0f) value = 0.0f;
  else if (value > 1.0f) value = 1.0f;
  
  SendParamToPd(paramIndex, value);
}

void PdWrapper::DefineParam(int argc, t_atom* argv)
{
  // This would be implemented to handle parameter definitions from pd
  // For now, we'll leave it as a stub
}

void PdWrapper::ProcessAudio(float** inputs, float** outputs, int numChannels, int blockSize)
{
  if (!mPdInstance) return;
  
  // Set the current pd instance
  libpd_set_instance(mPdInstance);
  
  // Resize buffers if needed
  size_t bufferSize = numChannels * blockSize;
  if (mInBuffer.size() < bufferSize) {
    mInBuffer.resize(bufferSize);
  }
  if (mOutBuffer.size() < bufferSize) {
    mOutBuffer.resize(bufferSize);
  }
  
  // Copy input data to the input buffer
  for (int c = 0; c < numChannels; c++) {
    for (int s = 0; s < blockSize; s++) {
      mInBuffer[c * blockSize + s] = inputs[c][s];
    }
  }
  
  // Process audio with pd
  libpd_process_raw(mInBuffer.data(), mOutBuffer.data());
  
  // Copy output data to the output buffers
  for (int c = 0; c < numChannels; c++) {
    for (int s = 0; s < blockSize; s++) {
      outputs[c][s] = mOutBuffer[c * blockSize + s];
    }
  }
}

void PdWrapper::ProcessMidiNote(int channel, int pitch, int velocity)
{
  if (!mPdInstance) return;
  
  SendMidiToPd(channel, pitch, velocity);
}

void PdWrapper::ProcessMessage(const char* recv, const char* msg, int argc, t_atom* argv)
{
  if (!mPdInstance) return;
  
  if (strcmp(recv, "pd") == 0) {
    if (strcmp(msg, "open") == 0 && argc >= 2) {
      // Handle open message from pd
      const char* filename = libpd_get_symbol(argv);
      const char* dir = libpd_get_symbol(argv + 1);
      OpenPatch(filename, dir);
    }
  } else if (strcmp(recv, "host") == 0) {
    if (strcmp(msg, "param") == 0 && argc >= 2) {
      // Handle parameter message from pd
      int paramIndex = (int)libpd_get_float(argv);
      float value = libpd_get_float(argv + 1);
      SetParamValue(paramIndex, value);
    } else if (strcmp(msg, "midinote") == 0 && argc >= 2) {
      // Handle MIDI note message from pd
      int pitch = (int)libpd_get_float(argv);
      int velocity = (int)libpd_get_float(argv + 1);
      // We would need to implement MIDI output here
    } else if (strcmp(msg, "in-bus-list") == 0) {
      // Handle input bus list from pd
      // This would be implemented to handle bus configuration
    } else if (strcmp(msg, "out-bus-list") == 0) {
      // Handle output bus list from pd
      // This would be implemented to handle bus configuration
    } else if (strcmp(msg, "define-param") == 0) {
      // Handle parameter definition from pd
      DefineParam(argc, argv);
    } else if (strcmp(msg, "read-uidesc") == 0 && argc > 0) {
      // Handle UI description from pd
      const char* filename = libpd_get_symbol(argv);
      // This would be implemented to read UI descriptions
    } else if (strcmp(msg, "verifyquit") == 0) {
      // Handle quit message from pd
      ClosePatch();
    }
  }
}

void PdWrapper::SendParamToPd(int paramIndex, float value)
{
  if (!mPdInstance) return;
  
  libpd_set_instance(mPdInstance);
  libpd_start_message(2);
  libpd_add_float(paramIndex);
  libpd_add_float(value);
  libpd_finish_list("param");
}

void PdWrapper::SendMidiToPd(int channel, int pitch, int velocity)
{
  if (!mPdInstance) return;
  
  libpd_set_instance(mPdInstance);
  libpd_start_message(2);
  libpd_add_float(pitch);
  libpd_add_float(velocity);
  libpd_finish_list("midinote");
}

// Static callback functions
void PdWrapper::MessageCallback(const char* recv, const char* msg, int argc, t_atom* argv)
{
  // Find the IPlugPureData instance associated with this pd instance
  t_pdinstance* pd = libpd_this_instance();
  if (!pd) return;
  
  auto it = sPdInstanceMap.find(pd);
  if (it != sPdInstanceMap.end()) {
    IPlugPureData* plugin = it->second;
    if (plugin) {
      // Get the PdWrapper instance from the plugin
      // This would require adding a method to IPlugPureData to get the PdWrapper
      // For now, we'll leave this as a stub
    }
  }
}

void PdWrapper::NoteOnCallback(int channel, int pitch, int velocity)
{
  // Find the IPlugPureData instance associated with this pd instance
  t_pdinstance* pd = libpd_this_instance();
  if (!pd) return;
  
  auto it = sPdInstanceMap.find(pd);
  if (it != sPdInstanceMap.end()) {
    IPlugPureData* plugin = it->second;
    if (plugin) {
      // Get the PdWrapper instance from the plugin
      // This would require adding a method to IPlugPureData to get the PdWrapper
      // For now, we'll leave this as a stub
    }
  }
} 
