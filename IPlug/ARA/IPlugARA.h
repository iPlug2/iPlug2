/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief ARA (Audio Random Access) support for iPlug2 VST3 plug-ins
 *
 * ARA is an extension of a companion plug-in format (here VST3), not a standalone format.
 * Define ARA_API at project level in addition to VST3_API to build an ARA-enabled VST3 plug-in.
 *
 * A minimal ARA plug-in subclasses IPlugARADocumentController (typically also providing a custom
 * ARA::PlugIn::PlaybackRenderer via doCreatePlaybackRenderer()) and sets ARA_DOC_CONTROLLER_CLASS
 * in config.h to the name of that subclass. See Examples/IPlugARAExample.
 */

#include "ARA_Library/PlugIn/ARAPlug.h"

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

/** Get the ARAFactory for this plug-in binary.
 * The definition is generated in IPlug_include_in_plug_src.h from config.h macros
 * (ARA_FACTORY_ID, ARA_DOC_ARCHIVE_ID, ARA_DOC_CONTROLLER_CLASS etc.) */
const ARA::ARAFactory* GetIPlugARAFactory();

/** Base class for an iPlug2 ARA document controller.
 * Provides default no-op implementations for the hooks that ARA::PlugIn::DocumentController
 * leaves pure virtual, so a subclass only needs to override what it actually uses.
 * Plug-ins that persist analysis results must override the archiving hooks.
 * @ingroup APIClasses */
class IPlugARADocumentController : public ARA::PlugIn::DocumentController
{
public:
  using ARA::PlugIn::DocumentController::DocumentController;

protected:
  // Archiving - override to persist/restore your ARA model state (analysis results etc.)
  bool doRestoreObjectsFromArchive(ARA::PlugIn::HostArchiveReader* pArchiveReader, const ARA::PlugIn::RestoreObjectsFilter* pFilter) noexcept override { return true; }
  bool doStoreObjectsToArchive(ARA::PlugIn::HostArchiveWriter* pArchiveWriter, const ARA::PlugIn::StoreObjectsFilter* pFilter) noexcept override { return true; }

  // Content updates from the host
  void doUpdateMusicalContextContent(ARA::PlugIn::MusicalContext* pMusicalContext, const ARA::ARAContentTimeRange* pRange, ARA::ContentUpdateScopes scopeFlags) noexcept override {}
  void doUpdateAudioSourceContent(ARA::PlugIn::AudioSource* pAudioSource, const ARA::ARAContentTimeRange* pRange, ARA::ContentUpdateScopes scopeFlags) noexcept override {}

  // Host sample access state changes
  void willEnableAudioSourceSamplesAccess(ARA::PlugIn::AudioSource* pAudioSource, bool enable) noexcept override {}
  void didEnableAudioSourceSamplesAccess(ARA::PlugIn::AudioSource* pAudioSource, bool enable) noexcept override {}
};

END_IPLUG_NAMESPACE
