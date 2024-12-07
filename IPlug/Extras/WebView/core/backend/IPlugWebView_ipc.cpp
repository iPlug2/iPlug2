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

#include "IPlugWebView_ipc.h"
#include <string>
#include <memory>

using namespace iplug;

nlohmann::json IPlugWebView_IPC::handle_IPC_Msg(const nlohmann::json& payload)
{

  std::string msg_id   = std::string(payload["msg_id"]);
  std::string type     = std::string(payload["type"]);
  std::string sender   = std::string(payload["sender"]);
  std::string target   = std::string(payload["target"]);
  std::string event_id = std::string(payload["event_id"]);
  nlohmann::json data  = payload["data"];


  if (type == "request"){
    nlohmann::json iPlug2_IPC_packet;
    iPlug2_IPC_packet["msg_id"]   = payload["msg_id"];
    iPlug2_IPC_packet["type"]     = "response";
    iPlug2_IPC_packet["sender"]   = "iPlug2_backend";
    iPlug2_IPC_packet["target"]   = sender;
    iPlug2_IPC_packet["event_id"] = payload["event_id"];

    std::string responseData = "{\"error\":\"invalid_ipc_request\"}";

    handleRequest(msg_id, type, sender, event_id, data, responseData);

    iPlug2_IPC_packet["data"] = responseData;

    return iPlug2_IPC_packet;
  }
  else if (type == "response"){
    handleResponse(msg_id, type, sender, event_id, data);
    return NULL;
  }
  else if (type == "message"){
    handleMessage(data);
    return NULL;
  }
}

std::string IPlugWebView_IPC::eventExists(const std::string& event_id)
{
  auto listener = listeners[event_id];
  auto listener_once = listeners_once[event_id];

  if (listener != NULL) return "always";
  if (listener_once != NULL) return "once";

  return "";
}

void IPlugWebView_IPC::on(const std::string& event_id, IPlugWebView_IPC_FrontendCallback callback)
{
  if (eventExists(event_id) != "") return;

  listeners[event_id] = callback;
}

void IPlugWebView_IPC::off(const std::string& event_id)
{
  listeners.erase(event_id);
  listeners_once.erase(event_id);
}

void IPlugWebView_IPC::once(const std::string& event_id, IPlugWebView_IPC_FrontendCallback callback)
{
  if (eventExists(event_id) != "") return;

  listeners_once[event_id] = callback;
}


void IPlugWebView_IPC::handleRequest(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data, std::string& responseData)
{
  auto listener = listeners[event_id];
  auto listener_once = listeners_once[event_id];

  bool exists = false;

  if (listener != NULL){
    exists = true;
    listener(data, responseData);
  } else {
    if (listener_once != NULL){
      exists = true;
      listener_once(data, responseData);
      off(event_id);
    }
  }

  if (!exists){
    responseData = IPlugWebView_IPC::Error("unknown_listener", "A listener with the event ID \"" + event_id + "\" does not exist.");
  }
};

void IPlugWebView_IPC::handleResponse(std::string msg_id, std::string type, std::string sender, std::string event_id, const nlohmann::json& data)
{
  IPlugWebView_IPC_BackendCallback awaiter = awaitList[msg_id];
  if (awaiter != NULL) awaiter(data);
};

void IPlugWebView_IPC::handleMessage(const nlohmann::json& json)
{
  if (onMessage != NULL) onMessage(json);
};



std::string IPlugWebView_IPC::sendToFE(std::string event_id, nlohmann::json data, std::string type, IPlugWebView_IPC_BackendCallback cb)
{
   std::string _type = "request";
  if (type != "") _type = type;


  if (event_id == "") throw "[iPlug2 IPC.sendToFE] Event ID cannot be empty";

  msg_id++;

  nlohmann::json req;

  req["msg_id"] = std::to_string(msg_id);
  req["type"] = _type;
  req["sender"] = sender_id;
  req["target"] = "iPlug2_frontend";
  req["event_id"] = event_id;
  req["data"] = data;
  

  if (_type == "request") {
    awaitList[req["msg_id"]] = cb;
  }

  onSendToFrontend(req.dump());

  return req["msg_id"];
};

void IPlugWebView_IPC::request(std::string event_id, nlohmann::json data, IPlugWebView_IPC_BackendCallback cb)
{
  sendToFE(event_id, data, "request", cb);
};

void IPlugWebView_IPC::message(nlohmann::json data)
{
  sendToFE("iPlug2_IPC_Message", data, "message", NULL);
};
