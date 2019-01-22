# How to Upgrade

iPlug2 is a significant reworking of the original code. Whilst we have thought about backwards compatibility, there are numerous changes in the various method signatures, and some different ways of doing things that mean that porting an iPlug1 plug-in is still a laborious task that will require patience. We would estimate that it would take a days work to transition a single plug-ins code base, but potentially more if you want to re-work the existing code base to do things the iPlug2 way - allowing distributed plug-ins et cetera. Any stability of previous products built on iPlug1, should be completely disregarded if you are going to port the code base to iPlug2 - i.e. so much has changed you should really beta test again, to catch any issues that arise from the significant changes under the hood.



* resource.h is now reserved for win32 resource fork - wo it can be manipulated with the visual studio resource editor... best not to edit it by hand.
* config.h includes most of what was in resource.h


* SetInputLabel(X, ...) -> SetChannelLabel(ERoute::kInput, X, ...)
* SetOutputLabel(X, ...) -> SetChannelLabel(ERoute::kOutput, X, ...)
* IsInChannelConnected(X) -> IsChannelConnected(ERoute::kInput, X)
* IsOutChannelConnected(X) -> IsChannelConnected(ERoute::kOutput, X)

* Resource ids are no longer needed, just file name;

  	pGraphics->AttachBackground(BG_ID, BG_FN); -> pGraphics->AttachBackground(BG_FN);
  	
* IGraphics::LoadIBitmap -> IGraphics::LoadBitmap
* IGraphics::FillIRect -> IGraphics::FillRect
* IGraphics::DrawIRect -> IGraphics::DrawRect
* Args for many methods are references not ptrs
* bool IControl::Draw(IGraphics* pGraphics) -> void IControl::Draw(IGraphics& g)
* Added IGraphics::DrawBitmapedText() (was in separate header)
* BOUNDED() macro -> Clip()
* IGraphics:DrawLine antiAlias argument replaced with lineWidth