/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// N.B. this file is included by IGraphics.cpp, it should not be added to projects

#ifdef IGRAPHICS_ACCESSKIT

#include "IGraphicsAccessibility.h"
#include "IGraphics.h"
#include "IControl.h"
#include "IPlugPluginBase.h"

#include <cstdlib>

using namespace iplug;
using namespace igraphics;

namespace
{
accesskit_role ToAccessKitRole(EAccessibleRole role)
{
  switch (role)
  {
    case EAccessibleRole::Group:        return ACCESSKIT_ROLE_GROUP;
    case EAccessibleRole::Label:        return ACCESSKIT_ROLE_LABEL;
    case EAccessibleRole::Button:       return ACCESSKIT_ROLE_BUTTON;
    case EAccessibleRole::Toggle:       return ACCESSKIT_ROLE_CHECK_BOX;
    case EAccessibleRole::Slider:       return ACCESSKIT_ROLE_SLIDER;
    case EAccessibleRole::ComboBox:     return ACCESSKIT_ROLE_COMBO_BOX;
    case EAccessibleRole::RadioGroup:   return ACCESSKIT_ROLE_RADIO_GROUP;
    case EAccessibleRole::RadioButton:  return ACCESSKIT_ROLE_RADIO_BUTTON;
    case EAccessibleRole::TabList:      return ACCESSKIT_ROLE_TAB_LIST;
    case EAccessibleRole::Tab:          return ACCESSKIT_ROLE_TAB;
    case EAccessibleRole::Meter:        return ACCESSKIT_ROLE_METER;
    case EAccessibleRole::Image:        return ACCESSKIT_ROLE_IMAGE;
    case EAccessibleRole::TextInput:    return ACCESSKIT_ROLE_TEXT_INPUT;
    default:                            return ACCESSKIT_ROLE_GENERIC_CONTAINER;
  }
}
} // namespace

IGraphicsAccessibility::IGraphicsAccessibility(IGraphics& graphics)
: mGraphics(graphics)
{
}

IGraphicsAccessibility::~IGraphicsAccessibility()
{
}

double IGraphicsAccessibility::GetRootScale() const
{
  return mGraphics.GetTotalScale();
}

uint32_t IGraphicsAccessibility::GetControlId(IControl* pControl)
{
  auto result = mControlIds.insert({pControl, mNextControlId});

  if (result.second)
    mNextControlId++;

  return result.first->second;
}

IControl* IGraphicsAccessibility::GetControlForNode(accesskit_node_id id, int& nodeIdx) const
{
  const uint32_t controlId = static_cast<uint32_t>(id >> kNodeIdxBits);
  nodeIdx = static_cast<int>(id & ((1 << kNodeIdxBits) - 1));

  for (auto& entry : mControlIds)
  {
    // The control may have been removed since the tree was last updated
    if (entry.second == controlId && mGraphics.GetControlIdx(entry.first) >= 0)
      return entry.first;
  }

  return nullptr;
}

void IGraphicsAccessibility::BuildSnapshot(std::vector<Node>& nodes, accesskit_node_id& focus)
{
  const int nControls = mGraphics.NControls();

  nodes.clear();
  nodes.reserve(nControls + 1);
  focus = kRootId;

  int focusNodeIdx = 0;
  IControl* pFocus = mGraphics.GetKeyboardFocus(&focusNodeIdx);

  nodes.emplace_back();
  nodes[0].id = kRootId;
  nodes[0].info.bounds = mGraphics.GetBounds();
  nodes[0].scale = GetRootScale();

  if (auto* pPlug = dynamic_cast<IPluginBase*>(mGraphics.GetDelegate()))
    nodes[0].info.label.Set(pPlug->GetPluginName());

  std::unordered_map<IControl*, size_t> controlNodes; // Indices of the first node of each control that is exposed
  std::unordered_map<IControl*, uint32_t> controlIds;
  IAccessibleNodeList controlNodeList;

  for (int i = 0; i < nControls; i++)
  {
    IControl* pControl = mGraphics.GetControl(i);

    if (pControl->IsHidden())
      continue;

    controlNodeList.clear();
    pControl->GetAccessibleNodes(controlNodeList);

    if (controlNodeList.empty() || controlNodeList[0].role == EAccessibleRole::None)
      continue;

    const uint32_t controlId = GetControlId(pControl);
    controlIds[pControl] = controlId;

    // The parent is the closest container that is itself exposed
    size_t parentNode = 0;

    for (IControl* pParent = pControl->GetParent(); pParent; pParent = pParent->GetParent())
    {
      auto itr = controlNodes.find(pParent);

      if (itr != controlNodes.end())
      {
        parentNode = itr->second;
        break;
      }
    }

    const size_t firstNode = nodes.size();
    const accesskit_node_id firstId = static_cast<accesskit_node_id>(controlId) << kNodeIdxBits;
    const int nNodes = std::min(static_cast<int>(controlNodeList.size()), 1 << kNodeIdxBits);

    controlNodes[pControl] = firstNode;
    nodes[parentNode].children.push_back(firstId);

    for (int n = 0; n < nNodes; n++)
    {
      nodes.emplace_back();
      Node& node = nodes.back();
      node.id = firstId | static_cast<accesskit_node_id>(n);
      node.info = controlNodeList[n];

      if (n > 0)
        nodes[firstNode].children.push_back(node.id);

      if (pControl == pFocus && n == focusNodeIdx)
        focus = node.id;
    }
  }

  // Forget controls that are no longer exposed, so that the map doesn't grow
  mControlIds.swap(controlIds);
}

accesskit_node* IGraphicsAccessibility::MakeNode(const Node& node) const
{
  const IAccessibleNode& info = node.info;
  const bool isRoot = node.id == kRootId;

  accesskit_node* pNode = accesskit_node_new(isRoot ? ACCESSKIT_ROLE_WINDOW : ToAccessKitRole(info.role));

  accesskit_node_set_bounds(pNode, accesskit_rect { info.bounds.L, info.bounds.T, info.bounds.R, info.bounds.B });

  if (isRoot)
    accesskit_node_set_transform(pNode, accesskit_affine_scale(node.scale));

  if (!node.children.empty())
    accesskit_node_set_children(pNode, node.children.size(), node.children.data());

  if (info.role == EAccessibleRole::Label)
  {
    // The text of a label is its value
    accesskit_node_set_value(pNode, info.label.Get());
  }
  else
  {
    if (info.label.GetLength())
      accesskit_node_set_label(pNode, info.label.Get());

    if (info.valueText.GetLength())
      accesskit_node_set_value(pNode, info.valueText.Get());
  }

  if (info.description.GetLength())
    accesskit_node_set_description(pNode, info.description.Get());

  if (info.hasNumericValue)
  {
    accesskit_node_set_numeric_value(pNode, info.value);
    accesskit_node_set_min_numeric_value(pNode, info.min);
    accesskit_node_set_max_numeric_value(pNode, info.max);
    accesskit_node_set_numeric_value_step(pNode, info.step);
  }

  if (info.toggled >= 0)
    accesskit_node_set_toggled(pNode, info.toggled ? ACCESSKIT_TOGGLED_TRUE : ACCESSKIT_TOGGLED_FALSE);

  if (info.role == EAccessibleRole::RadioButton)
    accesskit_node_set_toggled(pNode, info.selected ? ACCESSKIT_TOGGLED_TRUE : ACCESSKIT_TOGGLED_FALSE);
  else if (info.role == EAccessibleRole::Tab)
    accesskit_node_set_selected(pNode, info.selected);

  if (info.disabled)
  {
    accesskit_node_set_disabled(pNode);
  }
  else
  {
    if (info.focusable)
      accesskit_node_add_action(pNode, ACCESSKIT_ACTION_FOCUS);

    switch (info.role)
    {
      case EAccessibleRole::Button:
      case EAccessibleRole::Toggle:
      case EAccessibleRole::ComboBox:
      case EAccessibleRole::RadioButton:
      case EAccessibleRole::Tab:
        accesskit_node_add_action(pNode, ACCESSKIT_ACTION_CLICK);
        break;
      case EAccessibleRole::Slider:
        accesskit_node_add_action(pNode, ACCESSKIT_ACTION_INCREMENT);
        accesskit_node_add_action(pNode, ACCESSKIT_ACTION_DECREMENT);
        accesskit_node_add_action(pNode, ACCESSKIT_ACTION_SET_VALUE);
        break;
      default:
        break;
    }
  }

  return pNode;
}

accesskit_tree_update* IGraphicsAccessibility::MakeUpdate(bool full)
{
  std::vector<Node> nodes;
  accesskit_node_id focus = kRootId;
  BuildSnapshot(nodes, focus);

  accesskit_tree_update* pUpdate = accesskit_tree_update_with_capacity_and_focus(full ? nodes.size() : 0, focus);

  if (full)
  {
    accesskit_tree_info* pTreeInfo = accesskit_tree_info_new(kRootId);
    accesskit_tree_info_set_toolkit_name(pTreeInfo, "iPlug2 IGraphics");
    accesskit_tree_update_set_tree_info(pUpdate, pTreeInfo);

    for (auto& node : nodes)
      accesskit_tree_update_push_node(pUpdate, node.id, MakeNode(node));
  }
  else
  {
    std::unordered_map<accesskit_node_id, const Node*> prevNodes;
    prevNodes.reserve(mNodes.size());

    for (auto& node : mNodes)
      prevNodes[node.id] = &node;

    for (auto& node : nodes)
    {
      auto itr = prevNodes.find(node.id);

      // Nodes that were removed don't need to be listed, since they are no longer amongst the children of any node
      if (itr == prevNodes.end() || itr->second->info != node.info || itr->second->children != node.children || itr->second->scale != node.scale)
        accesskit_tree_update_push_node(pUpdate, node.id, MakeNode(node));
    }
  }

  mNodes.swap(nodes);

  return pUpdate;
}

accesskit_tree_update* IGraphicsAccessibility::ActivationHandler(void* pUserData)
{
  auto* pThis = static_cast<IGraphicsAccessibility*>(pUserData);
  pThis->mActive = true;
  return pThis->MakeUpdate(true);
}

accesskit_tree_update* IGraphicsAccessibility::UpdateFactory(void* pUserData)
{
  auto* pThis = static_cast<IGraphicsAccessibility*>(pUserData);
  const bool full = !pThis->mActive.exchange(true);
  return pThis->MakeUpdate(full);
}

void IGraphicsAccessibility::ActionHandler(accesskit_action_request* pRequest, void* pUserData)
{
  auto* pThis = static_cast<IGraphicsAccessibility*>(pUserData);

  Action action;
  action.action = pRequest->action;
  action.target = pRequest->target_node;

  if (pRequest->data.has_value)
  {
    if (pRequest->data.value.tag == ACCESSKIT_ACTION_DATA_NUMERIC_VALUE)
    {
      action.hasValue = true;
      action.value = pRequest->data.value.numeric_value;
    }
    else if (pRequest->data.value.tag == ACCESSKIT_ACTION_DATA_VALUE && pRequest->data.value.value)
    {
      action.hasValue = true;
      action.value = std::atof(pRequest->data.value.value);
    }
  }

  accesskit_action_request_free(pRequest);

  // This can be called on a thread other than the UI thread, so the action is performed on the next frame
  std::lock_guard<std::mutex> lock(pThis->mActionMutex);
  pThis->mActions.push_back(action);
}

void IGraphicsAccessibility::ProcessActions()
{
  std::vector<Action> actions;

  {
    std::lock_guard<std::mutex> lock(mActionMutex);
    actions.swap(mActions);
  }

  for (auto& action : actions)
  {
    int nodeIdx = 0;
    IControl* pControl = GetControlForNode(action.target, nodeIdx);

    if (!pControl || pControl->IsHidden() || pControl->IsDisabled())
      continue;

    switch (action.action)
    {
      case ACCESSKIT_ACTION_FOCUS:
        mGraphics.SetKeyboardFocus(pControl, nodeIdx);
        break;
      case ACCESSKIT_ACTION_CLICK:
        pControl->OnAccessibleAction(EAccessibleAction::Click, nodeIdx);
        break;
      case ACCESSKIT_ACTION_INCREMENT:
        pControl->OnAccessibleAction(EAccessibleAction::Increment, nodeIdx);
        break;
      case ACCESSKIT_ACTION_DECREMENT:
        pControl->OnAccessibleAction(EAccessibleAction::Decrement, nodeIdx);
        break;
      case ACCESSKIT_ACTION_SET_VALUE:
        if (action.hasValue)
          pControl->OnAccessibleAction(EAccessibleAction::SetValue, nodeIdx, action.value);
        break;
      default:
        break;
    }
  }
}

void IGraphicsAccessibility::RequestFullUpdate()
{
  mActive = false;
  mUpdateRequested = true;
}

void IGraphicsAccessibility::SetInactive()
{
  mActive = false;
}

void IGraphicsAccessibility::OnFrame(bool controlsDirty)
{
  const bool updateRequested = mUpdateRequested.exchange(false);

  if (controlsDirty || updateRequested)
    UpdateIfActive(UpdateFactory, this);
}

#endif // IGRAPHICS_ACCESSKIT
