window.__iPlug2 = {
  //Called at beginning of user web content to ensure that the iPlug2 core has been initialized
  isInitialized: (timeout = 3000) => {
    return new Promise((resolve, reject) => {
      if (iPlug2) return resolve()

      var start_ms = Date.now()

      var await_iPlug2_timer = setInterval(() => {

        var end_ms = Date.now()
        var elapsed_ms = end_ms - start_ms
        if (elapsed_ms > timeout) {
          clearInterval(await_iPlug2_timer)
          var err = "[iPlug2 Core] iPlug2 core was never initialized in time"
          console.error(err)
          return reject(err)
        }


        if (!iPlug2) return
        if (!iPlug2.initialized) return

        clearInterval(await_iPlug2_timer)
        console.log("[iPlug2 Core] Initialized")
        resolve()
      }, 1)
    })
  }
}


class iPlug2_IPC {
  constructor(opt) {
    this.msg_id = 0
    this.awaitList = {}
    this.listeners = {}

    this.sender_id = opt.sender_id
  }

  sendToBE(event_id, data = {}, type = "request", overridePacketInfo = {}) {
    if (!event_id) throw "[iPlug2 IPC.sendToBE] Event ID cannot be empty"

    this.msg_id++
    var msg_id = this.msg_id

    //send to backend
    var req = {
      ...{
        msg_id: msg_id.toString(),
        type: type,
        sender: this.sender_id,
        target: "iPlug2_backend",
        event_id: event_id,
        data: data
      },

      ...overridePacketInfo
    }


    var message = {
      msg_type: "iPlug2_IPC",
      payload: req
    };

    IPlugSendMsg(message);

    return msg_id
  }

  request(event_id, data = {}, timeout = 5000) {
    return new Promise((resolve, reject) => {
      var msg_id = this.sendToBE(event_id, data)

      var timeoutTimer = setTimeout(() => {
        var err = `[iPlug2 IPC.send] Send event ${event_id} timed out`
        console.error(err)
        reject(err)
      }, timeout)

      this.awaitList[msg_id] = {
        resolve: resolve,
        reject: reject,
        timeoutTimer: timeoutTimer
      }
    })
  }

  message(data) {
    this.sendToBE("iPlug2_IPC_Message", data, "message")
  }

  on(event_id, cb, once) {
    if (!event_id) throw "[iPlug2 IPC.on] Event ID cannot be empty"

    if (this.listeners[event_id]) console.warning(`[iPlug IPC.on] An event ID named "${event_id}" already exists`)

    var opts = {
      callback: cb
    }

    if (once) opts.once = true

    this.listeners[event_id] = opts
  }

  once(event_id, cb) {
    this.on(event_id, cb, true)
  }




  handleIncoming(res) {
    if (res.type === "response") this.handleResponse(res)
    else if (res.type === "request") this.handleRequest(res)
    else if (res.type === "message") this.handleMessage(res)
    else throw `[iPlug2 IPC.handleIncoming] Invalid incoming IPC message type`
  }

  handleResponse(res) {
    try {
      res.data = JSON.parse(res.data)
      res.data_type = typeof res.data
    } catch (err) {
      res.data_type = "unknown"
    }

    var awaiter = this.awaitList[res.msg_id]

    delete this.awaitList[res.msg_id]

    if (!awaiter) throw `[iPlug2 IPC.handleResponse] Invalid IPC response for event ID "${res.event_id}" with message ID "${res.msg_id}"`

    clearTimeout(awaiter.timeoutTimer)

    if (res.data.error) {
      awaiter.reject(res.data)
      return
    }

    awaiter.resolve(res.data)
  }

  //A request expects a response
  async handleRequest(res) {
    var listener = this.listeners[res.event_id]

    
    var responseData = {
      error: "unknown_listener",
      message: "A listener with the event ID \"" + res.event_id + "\" does not exist."
    }


    if (listener) {
      if (listener.callback.constructor.name == "AsyncFunction") var responseData = await listener.callback(res.data)
      else var responseData = listener.callback(res.data)
    }

    this.sendToBE(
      "iPlug2_IPC_Message",
      responseData,
      "response",
      {
        msg_id: res.msg_id,
        sender: res.target,
        target: res.sender
      }
    )
  }

  //A message doess not expect a response. The backend has simply sent a message that the app can handle as it wishes.
  handleMessage(res) {
    if (this.onMessage) this.onMessage(res.data)
  }
}

class iPlug2_Core {
  constructor(opt) {
    this.initialized = false

    this.id = opt.id

    this.init()
  }

  init() {
    this.ipc = new iPlug2_IPC({
      sender_id: this.id
    })

    this.initialized = true

    console.log("[iPlug2] Core successfully initialized")
  }
}

window.iPlug2 = new iPlug2_Core({ id: Date.now().toString() })
window.ip2 = iPlug2 //short name