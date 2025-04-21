#pragma once

#include <z_libpd.h>
#include <string>
#include <vector>

// Define MAXPDSTRING if not already defined by libpd
#ifndef MAXPDSTRING
#define MAXPDSTRING 4096
#endif

// Forward declarations
class IPlugPureData;

// Structure to hold parameter information
struct PdParam {
  float value;
  bool dirty;
};

// Callback function types
typedef void (*PdMessageCallback)(const char* recv, const char* msg, int argc, t_atom* argv);
typedef void (*PdNoteOnCallback)(int channel, int pitch, int velocity);

class PdWrapper {
public:
  PdWrapper(IPlugPureData* plugin);
  ~PdWrapper();
  
  // Initialization and cleanup
  void Init();
  void Cleanup();
  
  // Patch management
  void OpenPatch(const char* filename, const char* dir);
  void ClosePatch();
  
  // Parameter handling
  void SetParamValue(int paramIndex, float value);
  void DefineParam(int argc, t_atom* argv);
  
  // Audio processing
  void ProcessAudio(float** inputs, float** outputs, int numChannels, int blockSize);
  
  // MIDI handling
  void ProcessMidiNote(int channel, int pitch, int velocity);
  
  // Message handling
  void ProcessMessage(const char* recv, const char* msg, int argc, t_atom* argv);
  
  // Getters
  t_pdinstance* GetPdInstance() const { return mPdInstance; }
  bool IsPatchOpen() const { return mPatchOpen; }
  const char* GetPatchFilename() const { return mPatchFilename; }
  const char* GetPatchDir() const { return mPatchDir; }
  
private:
  IPlugPureData* mPlugin;
  t_pdinstance* mPdInstance;
  
  // Patch information
  bool mPatchOpen;
  char mPatchFilename[MAXPDSTRING];
  char mPatchDir[MAXPDSTRING];
  
  // libpd hooks
  void* mFromGuiHook;
  void* mFromPdHook;
  
  // Tempo tracking
  double mLastTempo;
  int mTicksSinceTempo;
  
  // Audio processing buffers
  std::vector<float> mInBuffer;
  std::vector<float> mOutBuffer;
  
  // Static callback functions
  static void MessageCallback(const char* recv, const char* msg, int argc, t_atom* argv);
  static void NoteOnCallback(int channel, int pitch, int velocity);
  
  // Helper methods
  void SendParamToPd(int paramIndex, float value);
  void SendMidiToPd(int channel, int pitch, int velocity);
}; 
