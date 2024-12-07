 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView IPC
  Copyright (c) 2024 Aid Vllasaliu
  GitHub(s): https://github.com/aidv, https://github.com/superkraft-io/
  As a contribution by splitter.ai

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "wdlstring.h"
#include <functional>
#include <memory>

#include "json.hpp"
#include <string>

BEGIN_IPLUG_NAMESPACE

/** IWebView is a base interface for hosting a platform web view inside an IPlug plug-in's UI */
class IPlugWebView_IPC
{
public:
  std::string sender_id = "iPlug2_WebView_Backend";
  long long msg_id = 0;

  /** Returns a standard OK IPC message*/
  static inline const std::string OK = "{}";

  /** Returns an ERROR IPC object
   * @param error Error code
   * @param message A human readable message
   * @return Returns a stringified JSON object, e.g {error: "failed", message: "This request failed"}*/
  static inline const std::string Error(std::string error, std::string message = "")
  {
    nlohmann::json json;

    json["error"] = error;
    json["message"] = message;

    return json.dump();
  };

  using IPlugWebView_IPC_FrontendCallback = std::function<void(nlohmann::json, std::string& responseData)>;
  using IPlugWebView_IPC_BackendCallback = std::function<void(nlohmann::json)>;

  IPlugWebView_IPC_BackendCallback onSendToFrontend;
  IPlugWebView_IPC_BackendCallback onMessage;

  
  nlohmann::json handle_IPC_Msg(const nlohmann::json& json);

  /** Returns an event type if it exists
   * @param event_id Name of the event
   * @return "always" for a standard event, "once" for a one-time event, "" (empty string) if the event does not exist*/
  std::string IPlugWebView_IPC::eventExists(const std::string& event_id);

  /** Adds an event that is fired when the frontend requests a response with the specified event ID
   * @param event_id Name of the event
   * @param callback Callback with the event data
   * @return A string representing the event message ID*/
  void on(const std::string& event_id, IPlugWebView_IPC_FrontendCallback callback);

  
  
  /** Removes an event
   * @param event_id Name of the event*/
  void off(const std::string& event_id);

  
  /** Adds a one-time event that is fired when the frontend requests a response with the specified event ID. A one-time event is automatically removed once it has been fired.
   * @param event_id Name of the event
   * @param callback Callback with the event data*/
  void once(const std::string& event_id, IPlugWebView_IPC_FrontendCallback callback);

  
  /** Makes a request to the frontend and awaits a response (currently indefinitely)
   * @param event_id Name of the event
   * @param data Data to send
   * @param cb Callback of the response*/
  void request(std::string event_id, nlohmann::json data, IPlugWebView_IPC_BackendCallback cb);

  /** Sends a response-less message to the frontend. This function does NOT expect or wait for a response.
   * @param data Data to send*/
  void message(nlohmann::json data);

private:
  std::unordered_map<std::string, IPlugWebView_IPC_BackendCallback> awaitList;
  std::unordered_map<std::string, IPlugWebView_IPC_FrontendCallback> listeners;
  std::unordered_map<std::string, IPlugWebView_IPC_FrontendCallback> listeners_once;

  void handleRequest(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data, std::string& responseData);
  void handleResponse(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data);
  void handleMessage(const nlohmann::json& json);

  std::string sendToFE(std::string event_id, nlohmann::json data, std::string type, IPlugWebView_IPC_BackendCallback cb);
};

END_IPLUG_NAMESPACE
