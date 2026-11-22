/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Bridges the IGraphics control hierarchy to platform accessibility APIs, via AccessKit (https://accesskit.dev)
 * Only compiled if IGRAPHICS_ACCESSKIT is defined
 */

#ifdef IGRAPHICS_ACCESSKIT

#include <mutex>
#include <vector>
#include <unordered_map>

#include "accesskit.h"

#include "IGraphicsStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IGraphics;
class IControl;

/** Maintains an AccessKit accessibility tree that mirrors the controls in an IGraphics context.
 * Platform classes inherit from this in order to own the AccessKit platform adapter.
 * The tree is rebuilt from IControl::GetAccessibleNodes() when controls are dirty, and only the nodes that changed are sent to the platform adapter.
 * Nothing is built unless assistive technology (e.g. a screen reader) is actually in use */
class IGraphicsAccessibility
{
public:
  IGraphicsAccessibility(IGraphics& graphics);
  virtual ~IGraphicsAccessibility();

  IGraphicsAccessibility(const IGraphicsAccessibility&) = delete;
  IGraphicsAccessibility& operator=(const IGraphicsAccessibility&) = delete;

  /** Perform any actions that were requested by assistive technology. Called once per frame on the UI thread, before controls are checked for dirtiness */
  void ProcessActions();

  /** Update the accessibility tree if necessary. Called once per frame on the UI thread
   * @param controlsDirty \c true if one or more controls were dirty this frame */
  void OnFrame(bool controlsDirty);

  /** AccessKit activation handler, called on the UI thread when assistive technology first asks for the tree. pUserData should be the IGraphicsAccessibility */
  static accesskit_tree_update* ActivationHandler(void* pUserData);

  /** AccessKit action handler, which may be called on any thread. pUserData should be the IGraphicsAccessibility */
  static void ActionHandler(accesskit_action_request* pRequest, void* pUserData);

protected:
  /** Call update_if_active on the platform adapter, and raise any resulting events */
  virtual void UpdateIfActive(accesskit_tree_update_factory factory, void* pUserData) = 0;

  /** @return The scale factor between IGraphics coordinates and the coordinates that the platform adapter expects, which by default is physical pixels */
  virtual double GetRootScale() const;

  IGraphics& mGraphics;

private:
  struct Node
  {
    accesskit_node_id id = 0;
    IAccessibleNode info;
    std::vector<accesskit_node_id> children;
    double scale = 0.; // Only used for the root node
  };

  struct Action
  {
    accesskit_action action;
    accesskit_node_id target;
    bool hasValue = false;
    double value = 0.;
  };

  static constexpr accesskit_node_id kRootId = 1;
  static constexpr int kNodeIdxBits = 16;

  static accesskit_tree_update* UpdateFactory(void* pUserData);

  void BuildSnapshot(std::vector<Node>& nodes, accesskit_node_id& focus);
  accesskit_tree_update* MakeUpdate(bool full);
  accesskit_node* MakeNode(const Node& node) const;
  uint32_t GetControlId(IControl* pControl);
  IControl* GetControlForNode(accesskit_node_id id, int& nodeIdx) const;

  std::vector<Node> mNodes; // The nodes that were last sent to the adapter
  std::unordered_map<IControl*, uint32_t> mControlIds;
  uint32_t mNextControlId = 1;
  bool mActive = false;

  std::mutex mActionMutex;
  std::vector<Action> mActions;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif // IGRAPHICS_ACCESSKIT
