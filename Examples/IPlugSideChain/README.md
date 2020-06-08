# IPlugSideChain
Demonstrates how to do a plug-in with two input buses for effects that require sidechain inputs such as compressors/gates.

The `PLUG_CHANNEL_IO` string in config.h should include all the possible variations of channel I/O. Hosts are rarely consistent about how they handle sidechain connections, so don't be surprised if it's not allways entirely predictable which I/O config you get.

1-1  	- mono input / mono output (no sidechain)

1.1-1  - mono input - mono sc input / mono output

1.2-1 - mono input - stereo sc input / mono output

1.2-2 - mono input - stereo sc input / stereo output

2.1-1 - stereo input - mono sc input / mono output

2.1-2 - stereo input - mono sc input / stereo output

2-2 - stereo input / stereo output

2.2-2 - stereo input - stereo sc input / stereo output 