#ifndef AAX_CIPlugPARAMETERS_H
#define AAX_CIPlugPARAMETERS_H

// This helper class is based on AVID's AAX_CInstrumentParameters

#include "AAX_CEffectParameters.h"

#include "AAX_IEffectDescriptor.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IPropertyMap.h"

#include "AAX_IMIDINode.h"
#include "AAX_IString.h"

struct AAX_SIPlugSetupInfo
{
  bool mNeedsGlobalMIDI;              // Does the IPlug use a global MIDI input node?
  const char* mGlobalMIDINodeName;    // Name of the global MIDI node, if used
  uint32_t mGlobalMIDIEventMask;      // Global MIDI node event mask, if used
  
  bool mNeedsInputMIDI;               // Does the IPlug use a local MIDI input node?
  const char* mInputMIDINodeName;     // Name of the MIDI input node, if used
  uint32_t mInputMIDIChannelMask;     // MIDI input node channel mask, if used
  
  bool mNeedsTransport;               // Does the IPlug use the transport interface?
  const char* mTransportMIDINodeName; // Name of the MIDI transport node, if used
  
  int32_t mNumMeters;                 // Number of meter taps used by the IPlug.  Must match the size of \ref mMeterIDs
  const AAX_CTypeID* mMeterIDs;       // Array of meter IDs
  
  AAX_EStemFormat mInputStemFormat;   // Input stem format
  AAX_EStemFormat mOutputStemFormat;  // Output stem format
  bool mCanBypass;                    // Can this plugin be bypassed?
  AAX_CTypeID mManufacturerID;        // Manufacturer ID
  AAX_CTypeID mProductID;             // Product ID
  AAX_CTypeID mPluginID;              // Plug-In (Type) ID
  AAX_CTypeID mAudioSuiteID;          // AudioSuite Plug-In (Type) ID

  int32_t mLatency;                   // Initial Latency
  
  AAX_SIPlugSetupInfo()
  {
    mNeedsGlobalMIDI = false;
    mGlobalMIDINodeName = "GlobalMIDI";
    mGlobalMIDIEventMask = 0xffff;
    mNeedsInputMIDI = false;
    mInputMIDINodeName = "InputMIDI";
    mInputMIDIChannelMask = 0xffff;
    mNeedsTransport = false;
    mTransportMIDINodeName = "Transport";
    mNumMeters = 0;
    mMeterIDs = 0;
    mInputStemFormat = AAX_eStemFormat_Mono;
    mOutputStemFormat = AAX_eStemFormat_Mono;
    mCanBypass = true;
    mManufacturerID = 'none';
    mProductID = 'none';
    mPluginID = 'none';
    mAudioSuiteID = 'none';
    mLatency = 0;
  }
};

class AAX_CIPlugParameters; 

struct AAX_SIPlugPrivateData
{
  AAX_CIPlugParameters* mIPlugParametersPtr;
  AAX_EStemFormat       mInputStemFormat;
  AAX_EStemFormat       mOutputStemFormat;
};

struct AAX_SIPlugRenderInfo
{
  float** mAudioInputs;           // Audio input buffers
  float** mAudioOutputs;          // Audio output buffers
  int32_t* mNumSamples;           // Number of samples in each buffer.  Bounded as per \ref AAE_EAudioBufferLengthNative.  The exact value can vary from buffer to buffer.

  AAX_IMIDINode* mInputNode;      // Buffered local MIDI input node. Used for incoming MIDI messages directed to the IPlug.
  AAX_IMIDINode* mGlobalNode;     // Buffered global MIDI input node. Used for global events like beat updates in metronomes.
  AAX_IMIDINode* mTransportNode;  // Transport MIDI node.  Used for querying the state of the MIDI transport.

  AAX_SIPlugPrivateData* mPrivateData; // Struct containing private data relating to the instance.  You should not need to use this data.

  float** mMeters;                // Array of meter taps.  One meter value should be entered per tap for each render call.

  // TODO: Aux Stems/sidechains
};

class AAX_CIPlugParameters : public AAX_CEffectParameters
{
public:
  AAX_CIPlugParameters () {}
  virtual ~AAX_CIPlugParameters () {}

  virtual void RenderAudio(AAX_SIPlugRenderInfo* ioRenderInfo) {}   

  virtual AAX_Result ResetFieldData (AAX_CFieldIndex iFieldIndex, void * oData, uint32_t iDataSize) const; 
  static  AAX_Result  StaticDescribe (AAX_IEffectDescriptor * ioDescriptor, const AAX_SIPlugSetupInfo & setupInfo);
  static  void  AAX_CALLBACK  StaticRenderAudio(AAX_SIPlugRenderInfo* const inInstancesBegin [], const void* inInstancesEnd); 
};

#endif
