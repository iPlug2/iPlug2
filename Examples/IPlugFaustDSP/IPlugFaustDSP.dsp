declare name "FaustExample";
import("stdfaust.lib");

g = vslider("Gain", 0, 0., 1, 0.1);

process = os.osc(440) * g, os.osc(441) * g;
