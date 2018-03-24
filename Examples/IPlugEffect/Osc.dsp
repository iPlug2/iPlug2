declare name        "Oscillator";
declare version     "0.1";
declare author      "Oliver Larkin";
declare copyright   "Copyright Oliver Larkin 2017";
declare license     "DWTFYWPL";
declare reference   "www.olilarkin.co.uk";
declare description	"Test";


import("stdfaust.lib");

process(l,r) = l + (os.osc(1000)), r + (os.osc(440));
