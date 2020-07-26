// declare options "[midi:on][nvoices:12]";
// declare name "IPlugFaustDSP (polydsp synth example)";

// import("stdfaust.lib");

// freq = hslider("freq ",200,50,1000,0.01);
// gain = hslider("gain",0.5,0,1,0.01);
// master = hslider("master [midi: ctrl 7]",0.5,0,1,0.01);
// gate = button("gate");
// envelope = en.adsr(0.01,0.01,0.8,0.1,gate)*gain;
// process = os.sawtooth(freq)*envelope*master <: (_,_);

declare name "IPlugFaustDSP (mono example)";
import("stdfaust.lib");

g = vslider("[1]Gain", 0, 0., 1, 0.1);
f1 = vslider("[2]Freq1", 440, 100., 1000, 0.1);
f2 = vslider("[3]Freq2", 441, 100., 1000, 0.1);

process = os.osc(f1) * g, os.osc(f2) * g;

