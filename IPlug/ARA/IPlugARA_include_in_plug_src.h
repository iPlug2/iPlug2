/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief ARA VST3 glue - included by IPlug_include_in_plug_src.h when ARA_API is defined
 * alongside VST3_API. Do not include this file directly.
 * Generates the ARAFactory from config.h macros and provides the ARA::IMainFactory class
 * that is registered with the VST3 module factory under kARAMainFactoryClass.
*/

#include "ARA_API/ARAVST3.h"
#include "IPlugARA.h"

// The FUID definitions for the ARA VST3 interfaces - these must exist exactly once per binary
DEF_CLASS_IID(ARA::IMainFactory)
DEF_CLASS_IID(ARA::IPlugInEntryPoint)
DEF_CLASS_IID(ARA::IPlugInEntryPoint2)

#ifndef ARA_DOC_CONTROLLER_CLASS
  #define ARA_DOC_CONTROLLER_CLASS iplug::IPlugARADocumentController
#endif

/** The playback transformations the plug-in applies to playback regions, as a combination of
 * ARA::ARAPlaybackTransformationFlags. Hosts only offer the corresponding region editing
 * (e.g. stretching a region's borders independently of its content) if it is advertised here. */
#ifndef ARA_PLAYBACK_TRANSFORMATION_FLAGS
  #define ARA_PLAYBACK_TRANSFORMATION_FLAGS ARA::kARAPlaybackTransformationNoChanges
#endif

/** Comma separated list of ARA::ARAContentType values that the plug-in can analyze and then
 * export to the host, e.g. ARA::kARAContentTypeNotes. Hosts use this both to decide which
 * content to request and to show the plug-in's capabilities in their UI. */
#ifdef ARA_ANALYZEABLE_CONTENT_TYPES
  #define ARA_HAS_ANALYZEABLE_CONTENT_TYPES 1
#else
  #define ARA_HAS_ANALYZEABLE_CONTENT_TYPES 0
#endif

/** Whether the plug-in can store its analysis into ARA audio file chunks, so that it travels
 * with the audio file between projects and hosts */
#ifndef ARA_SUPPORTS_AUDIO_FILE_CHUNKS
  #define ARA_SUPPORTS_AUDIO_FILE_CHUNKS 0
#endif

BEGIN_IPLUG_NAMESPACE

/** ARAFactory configuration, populated from config.h macros */
class IPlugARAFactoryConfig : public ARA::PlugIn::FactoryConfig
{
public:
  const char* getFactoryID() const noexcept override { return ARA_FACTORY_ID; }
  const char* getPlugInName() const noexcept override { return PLUG_NAME; }
  const char* getManufacturerName() const noexcept override { return PLUG_MFR; }
  const char* getInformationURL() const noexcept override { return PLUG_URL_STR; }
  const char* getVersion() const noexcept override { return PLUG_VERSION_STR; }
  const char* getDocumentArchiveID() const noexcept override { return ARA_DOC_ARCHIVE_ID; }

  ARA::ARAPlaybackTransformationFlags getSupportedPlaybackTransformationFlags() const noexcept override
  {
    return ARA_PLAYBACK_TRANSFORMATION_FLAGS;
  }

#if ARA_HAS_ANALYZEABLE_CONTENT_TYPES
  ARA::ARASize getAnalyzeableContentTypesCount() const noexcept override { return sizeof(sAnalyzeableContentTypes) / sizeof(sAnalyzeableContentTypes[0]); }
  const ARA::ARAContentType* getAnalyzeableContentTypes() const noexcept override { return sAnalyzeableContentTypes; }
#endif

#if ARA_SUPPORTS_AUDIO_FILE_CHUNKS
  bool supportsStoringAudioFileChunks() const noexcept override { return true; }
#endif

#if ARA_HAS_ANALYZEABLE_CONTENT_TYPES
private:
  static constexpr ARA::ARAContentType sAnalyzeableContentTypes[] = { ARA_ANALYZEABLE_CONTENT_TYPES };
#endif
};

#if ARA_HAS_ANALYZEABLE_CONTENT_TYPES
constexpr ARA::ARAContentType IPlugARAFactoryConfig::sAnalyzeableContentTypes[];
#endif

const ARA::ARAFactory* GetIPlugARAFactory()
{
  return ARA::PlugIn::PlugInEntry::getPlugInEntry<IPlugARAFactoryConfig, ARA_DOC_CONTROLLER_CLASS>()->getFactory();
}

/** VST3 class exposing the ARAFactory to the host, registered under kARAMainFactoryClass.
 * This lets ARA hosts obtain the factory without instantiating the audio effect component. */
class IPlugARAMainFactory : public ARA::IMainFactory
{
public:
  IPlugARAMainFactory() { FUNKNOWN_CTOR }
  virtual ~IPlugARAMainFactory() { FUNKNOWN_DTOR }

  static Steinberg::FUnknown* createInstance(void*) { return (ARA::IMainFactory*) new IPlugARAMainFactory(); }

  DECLARE_FUNKNOWN_METHODS

  // ARA::IMainFactory
  const ARA::ARAFactory* PLUGIN_API getFactory() SMTG_OVERRIDE { return GetIPlugARAFactory(); }
};

IMPLEMENT_FUNKNOWN_METHODS(IPlugARAMainFactory, ARA::IMainFactory, ARA::IMainFactory::iid)

END_IPLUG_NAMESPACE
