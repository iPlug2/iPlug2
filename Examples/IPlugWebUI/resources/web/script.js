// FROM DELEGATE

function SPVFD(paramIdx, val) {
  console.log("paramIdx: " + paramIdx + " value:" + val);
  OnParamChange(paramIdx, val);
}

function SCVFD(ctrlTag, val) {
  OnControlChange(ctrlTag, val);
//  console.log("SCVFD ctrlTag: " + ctrlTag + " value:" + val);
}

function SCMFD(ctrlTag, msgTag, msg) {
//  var decodedData = window.atob(msg);
  console.log("SCMFD ctrlTag: " + ctrlTag + " msgTag:" + msgTag + "msg:" + msg);
}

function SAMFD(msgTag, dataSize, msg) {
  OnMessage(msgTag, dataSize, msg);
}

function SMMFD(statusByte, dataByte1, dataByte2) {
  console.log("Got MIDI Message" + status + ":" + dataByte1 + ":" + dataByte2);
}

function SSMFD(offset, size, msg) {
  console.log("Got Sysex Message");
}

// FROM UI
// data should be a base64 encoded string
function SAMFUI(msgTag, ctrlTag = -1, data = 0) {
  var message = {
    "msg": "SAMFUI",
    "msgTag": msgTag,
    "ctrlTag": ctrlTag,
    "data": data
  };
  
  IPlugSendMsg(message);
}

function SMMFUI(statusByte, dataByte1, dataByte2) {
  var message = {
    "msg": "SMMFUI",
    "statusByte": statusByte,
    "dataByte1": dataByte1,
    "dataByte2": dataByte2
  };
  
  IPlugSendMsg(message);
}

// data should be a base64 encoded string
function SSMFUI(data = 0) {
  var message = {
    "msg": "SSMFUI",
    "data": data
  };
  
  IPlugSendMsg(message);
}

function EPCFUI(paramIdx) {
  if (paramIdx < 0) {
    console.log("EPCFUI paramIdx must be >= 0")
    return;
  }
  
  var message = {
    "msg": "EPCFUI",
    "paramIdx": parseInt(paramIdx),
  };
  
  IPlugSendMsg(message);
}

function BPCFUI(paramIdx) {
  if (paramIdx < 0) {
    console.log("BPCFUI paramIdx must be >= 0")
    return;
  }
  
  var message = {
    "msg": "BPCFUI",
    "paramIdx": parseInt(paramIdx),
  };
  
  IPlugSendMsg(message);
}

function SPVFUI(paramIdx, value) {
  if (paramIdx < 0) {
    console.log("SPVFUI paramIdx must be >= 0")
    return;
  }
  
  var message = {
    "msg": "SPVFUI",
    "paramIdx": parseInt(paramIdx),
    "value": value
  };

  IPlugSendMsg(message);
}
