declare name "FaustExample";
import("stdfaust.lib");

g = vslider("Gain", 1, 0., 1, 0.1);

process = os.osc(200), os.osc(1000) * g;
