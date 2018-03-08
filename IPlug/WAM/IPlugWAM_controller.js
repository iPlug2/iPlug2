// ----------------------------------------------------------------------------
// controller
//
var IPlugWAMController = function ()
{
  var desc = { audio: { outputs: [{ id:0, channels:2 }] } };
  this.init = function (actx, bufsize)
  {     
    var processor = new IPlugWAMProcessor();
    return this.setup(actx, bufsize, desc, processor);
  }

  // display message
  this.onmidi = function (me, time)
  {
    var elem = document.body.querySelector("#midiInfo");
    if (elem)
    {
      var info = "";
      for (var i=0; i<3; i++)
        info += "0x" + me[i].toString(16).toUpperCase() + " ";
      elem.innerHTML = info;
    }
  }
}
IPlugWAMController.prototype = new WAM.Controller("sync");

