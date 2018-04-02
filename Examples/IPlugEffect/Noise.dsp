declare name        "Test";
declare version     "0.1";
declare author      "Oliver Larkin";
declare copyright   "Copyright Oliver Larkin 2017";
declare license     "DWTFYWPL";
declare reference   "www.olilarkin.co.uk";
declare description	"Test";

import("stdfaust.lib");

gain = vslider("Gain", 0, 0., 1, 0.1);
process = no.noise * gain, no.noise * gain;
