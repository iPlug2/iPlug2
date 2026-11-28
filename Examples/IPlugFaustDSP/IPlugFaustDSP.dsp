// An example Faust DSP for iPlug2. In Debug builds it is compiled at runtime by the libfaust interpreter:
// edit and save this file while the plug-in is running, and the DSP, parameters and UI will be updated.
// The group/widget metadata in the labels determines the IGraphics UI built by IGraphicsFaustUI.

declare name "IPlugFaustDSP";
declare description "A stereo filter/drive effect, demonstrating iPlug2's Faust integration";

import("stdfaust.lib");

main(x) = hgroup("IPlugFaustDSP", x);
fx(x) = main(tgroup("[0]FX", x));
filterGroup(x) = fx(hgroup("[0]Filter", x));
driveGroup(x) = fx(hgroup("[1]Drive", x));
outputGroup(x) = main(vgroup("[1]Output", x));
meterGroup(x) = outputGroup(hgroup("[1]Meters", x));

filterType = filterGroup(nentry("[0]Type [style:menu{'Lowpass':0;'Highpass':1;'Bandpass':2}][tooltip:Filter response]", 0, 0, 2, 1));
cutoff = filterGroup(hslider("[1]Cutoff [style:knob][unit:Hz][scale:log]", 5000, 20, 20000, 1)) : si.smoo;
q = filterGroup(hslider("[2]Q [style:knob]", 0.7, 0.3, 10, 0.01)) : si.smoo;

drive = driveGroup(hslider("[0]Drive [style:knob]", 0.2, 0, 1, 0.01)) : si.smoo;
driveOn = driveGroup(checkbox("[1]Enable"));

gain = outputGroup(hslider("[0]Gain [unit:dB]", 0, -60, 12, 0.1)) : ba.db2linear : si.smoo;

filter(x) = fi.resonlp(cutoff, q, 1, x), fi.resonhp(cutoff, q, 1, x), fi.resonbp(cutoff, q, 1, x) : ba.selectn(3, filterType);
saturate = _ <: select2(driveOn, _, ef.cubicnl(drive, 0));

level = an.amp_follower(0.15) : ba.linear2db;
meterL = _ <: attach(_, level : meterGroup(vbargraph("[0]L [unit:dB]", -60, 6)));
meterR = _ <: attach(_, level : meterGroup(vbargraph("[1]R [unit:dB]", -60, 6)));
clip(x, y) = attach(x, max(abs(x), abs(y)) > 1 : meterGroup(vbargraph("[2]Clip [style:led]", 0, 1))), y;

channel = filter : saturate : *(gain);

process = channel, channel : meterL, meterR : clip;
