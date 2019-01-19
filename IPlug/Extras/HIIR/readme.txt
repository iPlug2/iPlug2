==============================================================================

        hiir
        Version 1.11

        An oversampling and Hilbert transform library in C++

        By Laurent de Soras, 2005-2013

==============================================================================



Contents:

1. Legal
2. What is hiir ?
3. Using hiir
4. Compilation
5. Oversampling to higher ratios
6. History
7. Contact



1. Legal
--------

Check the file license.txt to get full information about the license.



2. What is hiir ?
-----------------

hiir is a DSP (digital signal processing) library in C++, with two purposes:

    - Changing the sampling rate of a signal by a factor two, in both
directions (upsampling and downsampling).
    - Obtaining two signals with a pi/2 phase difference (Hilbert transform)

These distinct operations are actually sharing the same filter design method
and processing kernel, that is why they are included in the same package. The
filter (a two-path polyphase IIR) is very efficient and can be used to achieve
high-quality oversampling or phase shift at a low CPU cost. It is made of a
symetric half-band elliptic low-pass filter, hence having an extremely flat
frequency response in the passband.

Various implementations are supplied with the library, using special CPU
instructions to optimise the calculation speed. Currently SSE and 3DNow!
instruction sets are supported, as well as the classic and portable FPU
implementation.

Source code may be downloaded from this webpage:
http://ldesoras.free.fr/prod.html



3. Using hiir
-------------

To avoid any collision, names have been encapsulated in the namespace "hiir".
So you have to prefix every name of this library with hiir:: or put a line
"using namespace hiir;" into your code.

The filter design class is PolyphaseIir2Designer. It generates coefficients
for the filters from a specification: stopband attenuation, transition
bandwidth and/or number of coefficients.

The main processing classes are Downsampler2x*, Upsampler2x* and PhaseHalfPi*.
The suffix indicates the implementation. Choose "Fpu" if you are not sure
about the right one to use. All implementations of a class category have the
same function syntax, so you can use them with C++ templates.

The implementations should have a consistent behaviour, based on the FPU one.
Some of them have specific requirement, like object alignment in memory or
delay in processing. See the header file (.h) of the class for details about
the constraints and inconsistencies, and the code file (.cpp/.hpp) for details
about function calls.

As you can see, almost all classes are templates based on number of
coefficients. This means it is not possible to change this number at run-time.
This is the most important constraint of this library. However the reward is
the high speed of the execution. Anyway, you could build a wrapper to support
variable number of coefficients, althought it means that you will have
probably to compile a large number of variations on the same code.

The library processes only 32-bit floating point data.

hiir is intended to be portable, but has little architecture-dependant pieces
of code. So far, it has been built and tested on:

     - MS Windows / MS Visual C++ 6.0 (FPU/SSE/3DNow)
     - MS Windows / MS Visual C++ 2005 (FPU/SSE/3DNow)
     - MS Windows / MS Visual C++ 2010 (FPU/SSE/3DNow)
     - MS Windows / GCC 4.5.3 (FPU/SSE only)
     - MS Windows / Clang 3.1 (FPU/SSE only)
     - MacOS 10.5 / GCC 4 (FPU/SSE only)

If you happen to have another system and tweak it to make it run successfully,
pleeeeease send me your modification so I can include it to the main
distribution. Run main.cpp in Debug mode before, then in Release mode, in
order to be sure that everything is fine. I would also be glad to include
implementations for other processors/compilers.

References for filter use and design:

- Scott Wardle, "A Hilbert-Transformer Frequency Shifter for Audio"
http://www.iua.upf.es/dafx98/papers/

- Valenzuela and Constantinides, "Digital Signal Processing Schemes for
Efficient Interpolation and Decimation", IEEE Proceedings, Dec 1983

- Artur Krukowski, Izzet Kale, "The design of arbitrary-band multi-path
polyphase IIR filters", ISCAS 2, page 741-744. IEEE, 2001



4. Compilation and testing
--------------------------

Drop the following files into your project or makefile :

hiir/Array.*
hiir/def.h
hiir/Downsampler2x*.*
hiir/fnc.*
hiir/PhaseHalfPi*.*
hiir/PolyphaseIir2Designer.*
hiir/Stage*.*
hiir/Upsampler2x*.*

Other files (in the hiir/test directory) are for testing purpose only, do not
include them if you just need to use the library; they are not needed to use
hiir in your own programs.

hiir may be compiled in two versions: release and debug. Debug version
has checks that could slow down the code. Define NDEBUG to set the Release
mode. For example, the command line to compile the test bench on GCC or
Clang would look like:

Debug mode:
g++ -msse -I. -o ./hiir_debug.exe hiir/*.cpp hiir/test/*.cpp
clang++ -D_X86_ -msse -I. -o ./hiir_debug.exe hiir/*.cpp hiir/test/*.cpp

Release mode:
g++ -msse -I. -o ./hiir_release.exe -DNDEBUG -O3 hiir/*.cpp hiir/test/*.cpp
clang++ -D_X86_ -msse -I. -o ./hiir_release.exe -DNDEBUG -O3 hiir/*.cpp hiir/test/*.cpp

The "-msse" option enables the compilation of the SSE intrinsics.

Notes for MS VC++ 6.0 users:

- You'll need the Processor Pack in order to be able to compile 3DNow! and
SSE code.

- The intensive use of recursive templates may slow down a bit the compilation,
especially if you use many different filter sizes (number of coefficients).
On MS Visual C++, you will probably have to use the /Zm option to increase
the memory reserved to the compiler. /Zm500 should be enough to compile the
test bench.

- Also, MS Visual C++ issues a lot of warning related to the use of the EBX
register or lack of FEMMS instruction at the end of a function. This is normal
and you can safely disable these warning while using hiir classes.

The included test bench checks roughly the accuracy of the filters. It also
tests the speed of every available function. Therefore, implementing new
instruction set should be facilitated.

If you want to compile and run the test bench, please first edit the
test/conf.h file, in order to select the instruction sets available for your
CPU (there is currently no automatic detection). If you are not sure, disable
all of them.

In the same file, you have also testing options. You can save on the disk all
the samples generated during tests in order to check them in a sample editor.
However the files may take a lot of space on the disk, so it is recommended to
disable this option if it is not required. The "long tests" options are
intended to provide extensive checks on various filter sizes (it takes longer
to compile, but is safer if you want to change anything in the lib).



5. Oversampling to higher ratios
--------------------------------

It is possible to oversample a signal at a higher ratio than 2. You just have
to cascade up/downsamplers to achieve a power-of-2 ratio. Depending on your
requirements, you can reduce the filter order as the sampling rate is getting
bigger by reducing the transition bandwidth (TBW).

For example, let's suppose one wants 16x downsampling, with 96 dB of stopband
attenuation and a 0.49*Fs passband. You'll need the following specifications
for each stage:

 2x -> 1x: TBW = 0.01
 4x -> 2x: TBW = 0.01/2 + 1/4 = 0.255
 8x -> 4x: TBW = 0.01/4 + 1/8 + 1/4 = 0.3775
16x -> 8x: TBW = 0.01/8 + 1/16 + 1/8 + 1/4 = 0.43865

The reason is that you do not need to preserve spectrum parts that will be
wiped out by subsequent stages. Only the spectrum part present after the
final stage has to be perserved.

More generally:

TBW[stage] = (TBW[stage-1] + 0.5) / 2
or
TBW[stage] = TBW[0] * (0.5^stage) + 0.5 * (1 - 0.5^stage)

So transition bandwidth requirement is significatively low until the last
stage (0). Thus, the optimal performance would be reached by using hiir
downsampler for the last stage because the requirement on the transition
bandwidth is important, and by using a classic FIR filtering for other
stages. Of course, it's possible to use hiir at every stage, but a well-
optimised polyphase FIR routine is probably more efficient than a 1- or 2-
coefficent IIR downsampler. Indeed, these IIR SIMD implementations have
little or no benefit for low-order filters, whereas small FIR filters can
benefit from SIMD. Check the speed test results to make your mind.



6. History
----------

v1.11 (2012.06.26)
    - Changed the license to the WTFPL
    - Fixed some compilation warnings

v1.10 (2008.05.28)
    - Changed directory structure
    - Test code is now in its own namespace (hiir::test)
    - Uses intrinsics for SSE code, making the code compilable on GCC.

v1.00 (2005.03.29)
    - Initial release



7. Contact
----------

Please address any comment, bug report or flame to:

Laurent de Soras
http://ldesoras.free.fr

