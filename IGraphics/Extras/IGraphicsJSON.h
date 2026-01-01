/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file IGraphicsJSON.h
 * @brief JSON-based declarative UI for IGraphics with runtime layout support
 *
 * Enables defining IGraphics UIs in JSON with hot-reload during development.
 * Supports absolute, relative, and flexbox layouts that recalculate on resize.
 */

#include "IGraphics.h"

#ifndef IGRAPHICS_NO_JSON

#include "json.hpp"
#include <map>
#include <vector>
#include <string>
#include <functional>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

// Forward declarations
class IContainerBase;

using json = nlohmann::json;

/** Stored layout definition for a control, evaluated on each resize */
struct LayoutDef
{
  enum class Type { Absolute, Relative, Flex };
  Type type = Type::Absolute;
  json spec;              // Original JSON bounds specification
  int parentIdx = -1;     // Parent control index, -1 = root graphics bounds
  int flexContainerIdx = -1; // If in a flex container, the container's index
  IControl* containerPtr = nullptr; // Direct pointer for container children
};

/** JSON-based UI loader with runtime layout evaluation
 *
 * Usage:
 * @code
 * mLayoutFunc = [&](IGraphics* pGraphics) {
 *   const IRECT b = pGraphics->GetBounds();
 *   if (pGraphics->NControls() == 0) {
 *     pGraphics->SetLayoutOnResize(true);
 *     mJSONUI = std::make_unique<IGraphicsJSON>(pGraphics, this);
 *     mJSONUI->SetParamMapping({{"kGain", kGain}});
 *     mJSONUI->Load("ui.json");
 *   }
 *   mJSONUI->OnResize(b);
 * };
 * @endcode
 */
class IGraphicsJSON
{
public:
  IGraphicsJSON(IGraphics* pGraphics, IGEditorDelegate* pDelegate);
  ~IGraphicsJSON() = default;

  /** Load UI from a JSON file path
   * @param jsonPath Path to the JSON file
   * @return true on success */
  bool Load(const char* jsonPath);

  /** Load UI from a JSON string
   * @param jsonStr JSON content as string
   * @return true on success */
  bool LoadFromString(const char* jsonStr);

  /** Recalculate all control bounds based on new root bounds.
   * Call this from mLayoutFunc on resize.
   * @param newBounds The new root bounds (typically pGraphics->GetBounds()) */
  void OnResize(const IRECT& newBounds);

  /** Enable hot-reload file watching (debug builds only)
   * @param enable Whether to enable hot reload */
  void EnableHotReload(bool enable = true);

  /** Check for file changes and reload if needed. Call from OnIdle. */
  void CheckForChanges();

  /** Set mapping from parameter name strings to indices
   * @param paramMap Map of {"paramName", paramIndex} */
  void SetParamMapping(const std::map<std::string, int>& paramMap) { mParamMap = paramMap; }

  /** Set mapping from control ID strings to tag integers
   * @param tagMap Map of {"controlId", tagValue} */
  void SetTagMapping(const std::map<std::string, int>& tagMap) { mTagMap = tagMap; }

  /** Set mapping from bitmap names to pre-loaded bitmaps
   * @param bitmapMap Map of {"bitmapName", IBitmap} */
  void SetBitmapMapping(const std::map<std::string, IBitmap>& bitmapMap) { mBitmapMap = bitmapMap; }

  /** Set mapping from SVG names to pre-loaded SVGs
   * @param svgMap Map of {"svgName", ISVG} */
  void SetSVGMapping(const std::map<std::string, ISVG>& svgMap) { mSVGMap = svgMap; }

  /** Set mapping from action names to action functions
   * @param actionMap Map of {"actionName", function} */
  void SetActionMapping(const std::map<std::string, std::function<void(IControl*)>>& actionMap) { mActionMap = actionMap; }

  /** Get a control by its JSON id
   * @param id The control's id from JSON
   * @return The control, or nullptr if not found */
  IControl* GetControlById(const char* id);

  /** Check if there was an error loading/parsing JSON
   * @return true if there was an error */
  bool HasError() const { return !mErrorMessage.empty(); }

  /** Get the error message from the last load attempt
   * @return Error message, or empty string if no error */
  const char* GetErrorMessage() const { return mErrorMessage.c_str(); }

private:
  void ShowError(const char* message);
  void ClearError();
  bool ParseJSON(const json& j);
  bool ParseStyles(const json& j);
  bool ParseFonts(const json& j);
  void ParseControls(const json& controls, int parentIdx);

  IControl* CreateControl(const json& def, int parentIdx);

  // Container child creation
  void CreateContainerChildren(IContainerBase* pContainer, const json& children);

  // Layout evaluation
  IRECT EvaluateBounds(int ctrlIdx, const IRECT& parentBounds);
  float EvaluateExpr(const json& val, const IRECT& parent, char dimension);

  // Style parsing
  IVStyle ParseStyle(const json& styleDef);
  IVStyle GetStyle(const std::string& name);
  IColor ParseColor(const json& c);
  IText ParseText(const json& t);
  void ApplyStyleOverrides(IVStyle& style, const json& overrides);

  // Resource resolution
  IBitmap GetBitmap(const std::string& name);
  ISVG GetSVG(const std::string& name);
  IActionFunction GetAction(const std::string& name);

  // Resolution
  int ResolveParam(const json& param);
  int ResolveTag(const std::string& id);

  IGraphics* mGraphics;
  IGEditorDelegate* mDelegate;

  // Mappings
  std::map<std::string, IVStyle> mStyles;
  std::map<std::string, int> mTagMap;
  std::map<std::string, int> mParamMap;
  std::map<std::string, int> mIdToControlIdx;
  std::map<std::string, IBitmap> mBitmapMap;
  std::map<std::string, ISVG> mSVGMap;
  std::map<std::string, std::function<void(IControl*)>> mActionMap;

  // Layout state
  std::vector<LayoutDef> mLayouts;
  json mRootSpec;

  // Storage for page names (IVTabbedPagesControl)
  std::vector<std::string> mPageNameStorage;

  // Hot reload
  std::string mLoadedPath;
  bool mHotReloadEnabled = false;
#ifndef NDEBUG
  time_t mLastModified = 0;
#endif

  // Error state
  std::string mErrorMessage;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif // IGRAPHICS_NO_JSON
