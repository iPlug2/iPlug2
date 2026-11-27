This is the location of the iPlug2 tests.

- **[Automated](Automated/README.md)** : Unit and integration tests for IPlug and IGraphics (Catch2/CTest). These run headlessly on all platforms and in CI.

The remaining tests are iPlug2 projects, for testing things by hand in a host:

- **[IGraphicsTest](https://iplug2.github.io/NANOVG/IGraphicsTest/)** : An IPlug project that includes many controls to test different functionality 
  of IGraphics, with different drawing and platform backends.
  
- **[IGraphicsStressTest](https://iplug2.github.io/NANOVG/IGraphicsStressTest/)** : An IPlug project to test drawing lots of things

- **[MetaParamTest]((https://iplug2.github.io/NANOVG/MetaParamTest/))** : An IPlug project to test parameters that affect other parameters, a.k.a. Meta Parameters