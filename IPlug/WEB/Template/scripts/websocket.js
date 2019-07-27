/* Sets up a websocket client connection, for iPlug2 remote editors */

function setupWebSocket(onCompleted) {
  ws = new WebSocket('ws://' + window.location.host + '/ws');
  ws.binaryType = 'arraybuffer';

  ws.onopen = function() {
    console.log("websocket connected");
  };

  ws.onclose = function() {
  };

  ws.onmessage = function (e) {
    var msg = e.data;

    if(e.data.byteLength) {
        var buf = new Uint8Array(msg).buffer;
        var dv = new DataView(buf);
        var pos = 0;
        var strlen = dv.getInt32(pos, true); pos += 4;
        var prefix = new TextDecoder("utf-8").decode(new Uint8Array(buf, pos, strlen)); pos += strlen;

        //Send Parameter Value From Delegate
        if(prefix == "SPVFD") {
          var paramIdx = dv.getInt32(pos, true); pos += 4;
          var value = dv.getFloat64(pos, true); pos += 8;
          Module.SPVFD(paramIdx, value);
        }
        //Send Control Message From Delegate
        else if(prefix == "SCVDD") {
          var controlTag = dv.getInt32(pos, true); pos += 4;
          var value = dv.getFloat64(pos, true); pos += 8;
          Module.SCVFD(controlTag, value);
        }
        //Send Control Message From Delegate
        else if(prefix == "SCMFD") {
          var controlTag = dv.getInt32(pos, true); pos += 4;
          var msgTag = dv.getInt32(pos, true); pos += 4;
          var dataSize = dv.getInt32(pos, true); pos += 4;
          var data = new Uint8Array(buf, pos, dataSize);

          const esbuf = Module._malloc(data.length);
          Module.HEAPU8.set(data, esbuf);
          Module.SCMFD(controlTag, msgTag, data.length, esbuf);
          Module._free(esbuf);
        }
        //Send Arbitrary Message From Delegate
        else if(prefix == "SAMFD") {
          var msgTag = dv.getInt32(pos, true); pos += 4;
          var dataSize = dv.getInt32(pos, true); pos += 4;
          var data = new Uint8Array(buf, pos, dataSize);

          const esbuf = Module._malloc(data.length);
          Module.HEAPU8.set(data, esbuf);
          Module.SAMFD(controlTag, msgTag, data.length, esbuf);
          Module._free(esbuf);
        }
        //Send MIDI Message From Delegate
        else if(prefix == "SMMFD") {
          var status = dv.getUint8(pos, true); pos ++;
          var data1 = dv.getUint8(pos, true); pos ++;
          var data2 = dv.getUint8(pos, true); pos ++;
          Module.SMMFD(status, data1, data2);
        }
        //Send Sysex Message From Delegate
        else if(prefix == "SSMFD") {
          var msgTag = dv.getInt32(pos, true); pos += 4;
          var dataSize = dv.getInt32(pos, true); pos += 4;
          var data = new Uint8Array(buf, pos, dataSize);

          const esbuf = Module._malloc(data.length);
          Module.HEAPU8.set(data, esbuf);
          Module.SSMFD(controlTag, msgTag, data.length, esbuf);
          Module._free(esbuf);
        }
    }
  }

  ws.onerror = function (error) {
      alert('WebSocket error');
      //ws.close();
  }

  onCompleted();
}