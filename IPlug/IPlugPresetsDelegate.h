#pragma once

#include <cstring>
#include <cstdint>

#include "ptrlist.h"
#include "mutex.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugParameter.h"
#include "IPlugDelegate.h"
/**
 * @file
 * @copydoc IPlugPresets
 *
*/

/** Everything to do with saving presets */
class IPresetDelegate : public IDelegate
{
public:
  IPresetDelegate(int nParams, int nPresets);
  virtual ~IPresetDelegate();
  
  //IDelegate
  void ModifyCurrentPreset(const char* name = 0) override;
  int NPresets() override { return mPresets.GetSize(); }
  bool RestorePreset(int idx) override;
  bool RestorePreset(const char* name) override;
  const char* GetPresetName(int idx) override;
  
  // You can't use these three methods with chunks-based plugins, because there is no way to set the custom data
  void MakeDefaultPreset(const char* name = 0, int nPresets = 1);
  // MakePreset(name, param1, param2, ..., paramN)
  void MakePreset(const char* name, ...);
  // MakePresetFromNamedParams(name, nParamsNamed, paramEnum1, paramVal1, paramEnum2, paramVal2, ..., paramEnumN, paramVal2)
  // nParamsNamed may be less than the total number of params.
  void MakePresetFromNamedParams(const char* name, int nParamsNamed, ...);

  // Use these methods with chunks-based plugins
  void MakePresetFromChunk(const char* name, IByteChunk& chunk);
  void MakePresetFromBlob(const char* name, const char* blob, int sizeOfChunk);

  void PruneUninitializedPresets();

  // VST2 API only
  virtual void OnPresetsModified() {}
  void EnsureDefaultPreset();
  bool SerializePresets(IByteChunk& chunk);
  int UnserializePresets(IByteChunk& chunk, int startPos); // Returns the new chunk position (endPos).
  // /VST2 API only
  
  // Dump the current state as source code for a call to MakePresetFromNamedParams / MakePresetFromBlob
  void DumpPresetSrcCode(const char* file, const char* paramEnumNames[]);
  void DumpPresetBlob(const char* file);
  void DumpAllPresetsBlob(const char* filename);
  void DumpBankBlob(const char* file);

  //VST2 Presets
  bool SaveProgramAsFXP(const char* file);
  bool SaveBankAsFXB(const char* file);
  bool LoadProgramFromFXP(const char* file);
  bool LoadBankFromFXB(const char* file);
  bool SaveBankAsFXPs(const char* path) { return false; }

  //VST3 format
  bool SaveProgramAsVSTPreset(const char* file) { return false; }
  bool LoadProgramFromVSTPreset(const char* file) { return false; }
  bool SaveBankAsVSTPresets(const char* path) { return false; }

  //AU format
  bool SaveProgramAsAUPreset(const char* name, const char* file) { return false; }
  bool LoadProgramFromAUPreset(const char* file) { return false; }
  bool SaveBankAsAUPresets(const char* path) { return false; }

  //ProTools format
  bool SaveProgramAsProToolsPreset(const char* presetName, const char* file, unsigned long pluginID) { return false; }
  bool LoadProgramFromProToolsPreset(const char* file) { return false; }
  bool SaveBankAsProToolsPresets(const char* bath, unsigned long pluginID) { return false; }

protected:
  WDL_PtrList<IPreset> mPresets;
};
