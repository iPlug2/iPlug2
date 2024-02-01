/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "accesskit.h"

const char WINDOW_TITLE[] = "Hello world";

const accesskit_node_id WINDOW_ID = 0;
const accesskit_node_id BUTTON_1_ID = 1;
const accesskit_node_id BUTTON_2_ID = 2;
const accesskit_node_id ANNOUNCEMENT_ID = 3;
#define INITIAL_FOCUS BUTTON_1_ID

const accesskit_rect BUTTON_1_RECT = {20.0, 20.0, 100.0, 60.0};

const accesskit_rect BUTTON_2_RECT = {20.0, 60.0, 100.0, 100.0};

const int SET_FOCUS_MSG = 0;
const int DO_DEFAULT_ACTION_MSG = 1;

accesskit_node *build_button(accesskit_node_id id, const char *name,
                             accesskit_node_class_set *classes) {
  accesskit_rect rect;
  if (id == BUTTON_1_ID) {
    rect = BUTTON_1_RECT;
  } else {
    rect = BUTTON_2_RECT;
  }

  accesskit_node_builder *builder =
      accesskit_node_builder_new(ACCESSKIT_ROLE_BUTTON);
  accesskit_node_builder_set_bounds(builder, rect);
  accesskit_node_builder_set_name(builder, name);
  accesskit_node_builder_add_action(builder, ACCESSKIT_ACTION_FOCUS);
  accesskit_node_builder_set_default_action_verb(
      builder, ACCESSKIT_DEFAULT_ACTION_VERB_CLICK);
  return accesskit_node_builder_build(builder, classes);
}

accesskit_node *build_announcement(const char *text,
                                   accesskit_node_class_set *classes) {
  accesskit_node_builder *builder =
      accesskit_node_builder_new(ACCESSKIT_ROLE_STATIC_TEXT);
  accesskit_node_builder_set_name(builder, text);
  accesskit_node_builder_set_live(builder, ACCESSKIT_LIVE_POLITE);
  return accesskit_node_builder_build(builder, classes);
}

struct accesskit_iplug_adapter {
#if defined(__APPLE__)
  accesskit_macos_subclassing_adapter *adapter;
#elif defined(UNIX)
  accesskit_unix_adapter *adapter;
#elif defined(_WIN32)
  accesskit_windows_subclassing_adapter *adapter;
#endif
};

void accesskit_iplug_adapter_init(struct accesskit_iplug_adapter *adapter,
                                void *window,
                                accesskit_tree_update_factory source,
                                void *source_userdata,
                                accesskit_action_handler *handler) {
#if defined(__APPLE__)
//  accesskit_macos_add_focus_forwarder_to_window_class("SDLWindow");
  adapter->adapter = accesskit_macos_subclassing_adapter_for_window(
      window, source, source_userdata, handler);
//#elif defined(UNIX)
//  adapter->adapter =
//      accesskit_unix_adapter_new(source, source_userdata, handler);
//#elif defined(_WIN32)
//  adapter->adapter = accesskit_windows_subclassing_adapter_new(
//      wmInfo.info.win.window, source, source_userdata, handler);
#endif
}

void accesskit_iplug_adapter_destroy(struct accesskit_iplug_adapter *adapter) {
  if (adapter->adapter != NULL) {
#if defined(__APPLE__)
    accesskit_macos_subclassing_adapter_free(adapter->adapter);
//#elif defined(UNIX)
//    accesskit_unix_adapter_free(adapter->adapter);
//#elif defined(_WIN32)
//    accesskit_windows_subclassing_adapter_free(adapter->adapter);
#endif
  }
}

void accesskit_iplug_adapter_update_if_active(
    const struct accesskit_iplug_adapter *adapter,
    accesskit_tree_update_factory update_factory,
    void *update_factory_userdata) {
#if defined(__APPLE__)
  accesskit_macos_queued_events *events =
      accesskit_macos_subclassing_adapter_update_if_active(
          adapter->adapter, update_factory, update_factory_userdata);
  if (events != NULL) {
    accesskit_macos_queued_events_raise(events);
  }
//#elif defined(UNIX)
//  accesskit_unix_adapter_update_if_active(adapter->adapter, update_factory,
//                                          update_factory_userdata);
//#elif defined(_WIN32)
//  accesskit_windows_queued_events *events =
//      accesskit_windows_subclassing_adapter_update_if_active(
//          adapter->adapter, update_factory, update_factory_userdata);
//  if (events != NULL) {
//    accesskit_windows_queued_events_raise(events);
//  }
#endif
}

void accesskit_iplug_adapter_update_window_focus_state(
    const struct accesskit_iplug_adapter *adapter, bool is_focused) {
#if defined(__APPLE__)
  accesskit_macos_queued_events *events =
      accesskit_macos_subclassing_adapter_update_view_focus_state(
          adapter->adapter, is_focused);
  if (events != NULL) {
    accesskit_macos_queued_events_raise(events);
  }
//#elif defined(UNIX)
//  accesskit_unix_adapter_update_window_focus_state(adapter->adapter,
//                                                   is_focused);
#endif
  /* On Windows, the subclassing adapter takes care of this. */
}

void accesskit_iplug_adapter_update_root_window_bounds(
    const struct accesskit_iplug_adapter *adapter, void *window) {
//#if defined(UNIX)
//  int x, y, width, height;
//  SDL_GetWindowPosition(window, &x, &y);
//  SDL_GetWindowSize(window, &width, &height);
//  int top, left, bottom, right;
//  SDL_GetWindowBordersSize(window, &top, &left, &bottom, &right);
//  accesskit_rect outer_bounds = {x - left, y - top, x + width + right,
//                                 y + height + bottom};
//  accesskit_rect inner_bounds = {x, y, x + width, y + height};
//  accesskit_unix_adapter_set_root_window_bounds(adapter->adapter, outer_bounds,
//                                                inner_bounds);
//#endif
}

struct window_state {
  accesskit_node_id focus;
  const char *announcement;
  accesskit_node_class_set *node_classes;
  WDL_Mutex *mutex;
};

void window_state_init(struct window_state *state) {
  state->focus = INITIAL_FOCUS;
  state->announcement = NULL;
  state->node_classes = accesskit_node_class_set_new();
  state->mutex = new WDL_Mutex();
}

void window_state_destroy(struct window_state *state) {
  accesskit_node_class_set_free(state->node_classes);
  delete state->mutex;
}

void window_state_lock(struct window_state *state) {
  state->mutex->Enter();
}

void window_state_unlock(struct window_state *state) {
  state->mutex->Leave();
}

accesskit_node *window_state_build_root(const struct window_state *state) {
  accesskit_node_builder *builder =
      accesskit_node_builder_new(ACCESSKIT_ROLE_WINDOW);
  accesskit_node_builder_push_child(builder, BUTTON_1_ID);
  accesskit_node_builder_push_child(builder, BUTTON_2_ID);
  if (state->announcement != NULL) {
    accesskit_node_builder_push_child(builder, ANNOUNCEMENT_ID);
  }
  accesskit_node_builder_set_name(builder, WINDOW_TITLE);
  return accesskit_node_builder_build(builder, state->node_classes);
}

accesskit_tree_update *window_state_build_initial_tree(
    const struct window_state *state) {
  accesskit_node *root = window_state_build_root(state);
  accesskit_node *button_1 =
      build_button(BUTTON_1_ID, "Button 1", state->node_classes);
  accesskit_node *button_2 =
      build_button(BUTTON_2_ID, "Button 2", state->node_classes);
  accesskit_tree_update *result = accesskit_tree_update_with_capacity_and_focus(
      (state->announcement != NULL) ? 4 : 3, state->focus);
  accesskit_tree *tree = accesskit_tree_new(WINDOW_ID);
  accesskit_tree_set_app_name(tree, "Hello World");
  accesskit_tree_update_set_tree(result, tree);
  accesskit_tree_update_push_node(result, WINDOW_ID, root);
  accesskit_tree_update_push_node(result, BUTTON_1_ID, button_1);
  accesskit_tree_update_push_node(result, BUTTON_2_ID, button_2);
  if (state->announcement != NULL) {
    accesskit_node *announcement =
        build_announcement(state->announcement, state->node_classes);
    accesskit_tree_update_push_node(result, ANNOUNCEMENT_ID, announcement);
  }
  return result;
}

accesskit_tree_update *build_tree_update_for_button_press(void *userdata) {
  auto* state = reinterpret_cast<window_state*>(userdata);;
  accesskit_node *announcement =
      build_announcement(state->announcement, state->node_classes);
  accesskit_node *root = window_state_build_root(state);
  accesskit_tree_update *update =
      accesskit_tree_update_with_capacity_and_focus(2, state->focus);
  accesskit_tree_update_push_node(update, ANNOUNCEMENT_ID, announcement);
  accesskit_tree_update_push_node(update, WINDOW_ID, root);
  return update;
}

void window_state_press_button(struct window_state *state,
                               const struct accesskit_iplug_adapter *adapter,
                               accesskit_node_id id) {
  const char *text;
  if (id == BUTTON_1_ID) {
    text = "You pressed button 1";
  } else {
    text = "You pressed button 2";
  }
  state->announcement = text;
  accesskit_iplug_adapter_update_if_active(
      adapter, build_tree_update_for_button_press, state);
}

accesskit_tree_update *build_tree_update_for_focus_update(void *userdata) {
  auto* state = reinterpret_cast<window_state*>(userdata);;
  accesskit_tree_update *update =
      accesskit_tree_update_with_focus(state->focus);
  return update;
}

void window_state_set_focus(struct window_state *state,
                            const struct accesskit_iplug_adapter *adapter,
                            accesskit_node_id focus) {
  state->focus = focus;
  accesskit_iplug_adapter_update_if_active(
      adapter, build_tree_update_for_focus_update, state);
}

struct action_handler_state {
  uint32_t event_type;
  uint32_t window_id;
};

void do_action(const accesskit_action_request *request, void *userdata) {
  auto* action_handler_state = reinterpret_cast<struct action_handler_state*>(userdata);

//  SDL_Event event;
//  SDL_zero(event);
//  event.type = state->event_type;
//  event.user.windowID = state->window_id;
//  event.user.data1 = (void *)((uintptr_t)(request->target));
//  if (request->action == ACCESSKIT_ACTION_FOCUS) {
//    event.user.code = SET_FOCUS_MSG;
//    SDL_PushEvent(&event);
//  } else if (request->action == ACCESSKIT_ACTION_DEFAULT) {
//    event.user.code = DO_DEFAULT_ACTION_MSG;
//    SDL_PushEvent(&event);
//  }
}

accesskit_tree_update *build_initial_tree(void *userdata) {
  auto* state = reinterpret_cast<window_state*>(userdata);
  window_state_lock(state);
  accesskit_tree_update *update = window_state_build_initial_tree(state);
  window_state_unlock(state);
  return update;
}
