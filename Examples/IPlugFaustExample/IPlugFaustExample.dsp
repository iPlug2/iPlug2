declare name "FaustExample";
import("stdfaust.lib");

g = vslider("Gain", 1, 0., 1, 0.1);

process = os.osc(100), os.osc(201) * g;
