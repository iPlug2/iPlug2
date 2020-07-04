// declare options "[midi:on][nvoices:12]";
// declare name "FaustExample";

// import("stdfaust.lib");

// freq = hslider("freq ",200,50,1000,0.01);
// gain = hslider("gain",0.5,0,1,0.01);
// master = hslider("master [midi: ctrl 7]",0.5,0,1,0.01);
// gate = button("gate");
// envelope = en.adsr(0.01,0.01,0.8,0.1,gate)*gain;
// process = os.sawtooth(freq)*envelope*master <: (_,_);

declare name "FaustExample";
import("stdfaust.lib");

g = vslider("Gain", 0, 0., 1, 0.1);

process = os.osc(440) * g, os.osc(441) * g;