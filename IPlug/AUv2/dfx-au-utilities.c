/*
	Destroy FX AU Utilities is a collection of helpful utility functions 
	for creating and hosting Audio Unit plugins.
	Copyright (C) 2003-2008  Sophia Poirier
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without 
	modification, are permitted provided that the following conditions 
	are met:
	
	*	Redistributions of source code must retain the above 
		copyright notice, this list of conditions and the 
		following disclaimer.
	*	Redistributions in binary form must reproduce the above 
		copyright notice, this list of conditions and the 
		following disclaimer in the documentation and/or other 
		materials provided with the distribution.
	*	Neither the name of Destroy FX nor the names of its 
		contributors may be used to endorse or promote products 
		derived from this software without specific prior 
		written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE 
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
	OF THE POSSIBILITY OF SUCH DAMAGE.
	
	To contact the author, please visit http://destroyfx.org/ 
	and use the contact form.
*/

// this is a modified version of dfx-au-utilities.c keeping only CFAUPreset related functionality

#include "dfx-au-utilities.h"

#include <AudioToolbox/AudioUnitUtilities.h>	// for AUEventListenerNotify and AUParameterListenerNotify

#pragma mark -
#pragma mark Factory Presets CFArray

//-----------------------------------------------------------------------------
// The following defines and implements CoreFoundation-like handling of 
// an AUPreset container object:  CFAUPreset
//-----------------------------------------------------------------------------

const UInt32 kCFAUPreset_CurrentVersion = 0;

typedef struct {
	AUPreset auPreset;
	UInt32 version;
	CFAllocatorRef allocator;
	CFIndex retainCount;
} CFAUPreset;

//-----------------------------------------------------------------------------
// create an instance of a CFAUPreset object
CFAUPresetRef CFAUPresetCreate(CFAllocatorRef inAllocator, SInt32 inPresetNumber, CFStringRef inPresetName)
{
	CFAUPreset * newPreset = (CFAUPreset*) CFAllocatorAllocate(inAllocator, sizeof(CFAUPreset), 0);
	if (newPreset != NULL)
	{
		newPreset->auPreset.presetNumber = inPresetNumber;
		newPreset->auPreset.presetName = NULL;
		// create our own a copy rather than retain the string, in case the input string is mutable, 
		// this will keep it from changing under our feet
		if (inPresetName != NULL)
			newPreset->auPreset.presetName = CFStringCreateCopy(inAllocator, inPresetName);
		newPreset->version = kCFAUPreset_CurrentVersion;
		newPreset->allocator = inAllocator;
		newPreset->retainCount = 1;
	}
	return (CFAUPresetRef)newPreset;
}

//-----------------------------------------------------------------------------
// retain a reference of a CFAUPreset object
CFAUPresetRef CFAUPresetRetain(CFAUPresetRef inPreset)
{
	if (inPreset != NULL)
	{
		CFAUPreset * incomingPreset = (CFAUPreset*) inPreset;
		// retain the input AUPreset's name string for this reference to the preset
		if (incomingPreset->auPreset.presetName != NULL)
			CFRetain(incomingPreset->auPreset.presetName);
		incomingPreset->retainCount += 1;
	}
	return inPreset;
}

//-----------------------------------------------------------------------------
// release a reference of a CFAUPreset object
void CFAUPresetRelease(CFAUPresetRef inPreset)
{
	CFAUPreset * incomingPreset = (CFAUPreset*) inPreset;
	// these situations shouldn't happen
	if (inPreset == NULL)
		return;
	if (incomingPreset->retainCount <= 0)
		return;

	// first release the name string, CF-style, since it's a CFString
	if (incomingPreset->auPreset.presetName != NULL)
		CFRelease(incomingPreset->auPreset.presetName);
	incomingPreset->retainCount -= 1;
	// check if this is the end of this instance's life
	if (incomingPreset->retainCount == 0)
	{
		// wipe out the data so that, if anyone tries to access stale memory later, it will be (semi)invalid
		incomingPreset->auPreset.presetName = NULL;
		incomingPreset->auPreset.presetNumber = 0;
		// and finally, free the memory for the CFAUPreset struct
		CFAllocatorDeallocate(incomingPreset->allocator, (void*)inPreset);
	}
}

//-----------------------------------------------------------------------------
// The following 4 functions are CFArray callbacks for use when creating 
// an AU's factory presets array to support kAudioUnitProperty_FactoryPresets.
//-----------------------------------------------------------------------------

const void * CFAUPresetArrayRetainCallBack(CFAllocatorRef inAllocator, const void * inPreset);
void CFAUPresetArrayReleaseCallBack(CFAllocatorRef inAllocator, const void * inPreset);
Boolean CFAUPresetArrayEqualCallBack(const void * inPreset1, const void * inPreset2);
CFStringRef CFAUPresetArrayCopyDescriptionCallBack(const void * inPreset);
void CFAUPresetArrayCallBacks_Init(CFArrayCallBacks * outArrayCallBacks);

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is added to the CFArray, 
// or when a CFArray containing an AUPreset is retained.  
const void * CFAUPresetArrayRetainCallBack(CFAllocatorRef inAllocator, const void * inPreset)
{
	return CFAUPresetRetain(inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when an item (an AUPreset) is removed from the CFArray 
// or when the array is released.
// Since a reference to the data belongs to the array, we need to release that here.
void CFAUPresetArrayReleaseCallBack(CFAllocatorRef inAllocator, const void * inPreset)
{
	CFAUPresetRelease(inPreset);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to compare to items (AUPresets) 
// in the CFArray to see if they are equal or not.
// For our AUPresets, we will compare based on the preset number and the name string.
Boolean CFAUPresetArrayEqualCallBack(const void * inPreset1, const void * inPreset2)
{
	AUPreset * preset1 = (AUPreset*) inPreset1;
	AUPreset * preset2 = (AUPreset*) inPreset2;
	// the two presets are only equal if they have the same preset number and 
	// if the two name strings are the same (which we rely on the CF function to compare)
	return (preset1->presetNumber == preset2->presetNumber) && 
			(CFStringCompare(preset1->presetName, preset2->presetName, 0) == kCFCompareEqualTo);
}

//-----------------------------------------------------------------------------
// This function is called when someone wants to get a description of 
// a particular item (an AUPreset) as though it were a CF type.  
// That happens, for example, when using CFShow().  
// This will create and return a CFString that indicates that 
// the object is an AUPreset and tells the preset number and preset name.
CFStringRef CFAUPresetArrayCopyDescriptionCallBack(const void * inPreset)
{
	AUPreset * preset = (AUPreset*) inPreset;
	return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
									CFSTR("AUPreset:\npreset number = %d\npreset name = %@"), 
									(int)preset->presetNumber, preset->presetName);
}

//-----------------------------------------------------------------------------
// this will initialize a CFArray callbacks structure to use the above callback functions
void CFAUPresetArrayCallBacks_Init(CFArrayCallBacks * outArrayCallBacks)
{
	if (outArrayCallBacks == NULL)
		return;
	// wipe the struct clean
	memset(outArrayCallBacks, 0, sizeof(*outArrayCallBacks));
	// set all of the values and function pointers in the callbacks struct
	outArrayCallBacks->version = 0;	// currently, 0 is the only valid version value for this
	outArrayCallBacks->retain = CFAUPresetArrayRetainCallBack;
	outArrayCallBacks->release = CFAUPresetArrayReleaseCallBack;
	outArrayCallBacks->copyDescription = CFAUPresetArrayCopyDescriptionCallBack;
	outArrayCallBacks->equal = CFAUPresetArrayEqualCallBack;
}

//-----------------------------------------------------------------------------
//#ifdef __GNUC__
//const CFArrayCallBacks kCFAUPresetArrayCallBacks;
//static void kCFAUPresetArrayCallBacks_constructor() __attribute__((constructor));
//static void kCFAUPresetArrayCallBacks_constructor()
//{
//  CFAUPresetArrayCallBacks_Init( (CFArrayCallBacks*) &kCFAUPresetArrayCallBacks );
//}
//#else
// XXX I'll use this for other compilers, even though I hate initializing structs with all arguments at once
// (cuz what if you ever decide to change the order of the struct members or something like that?)
const CFArrayCallBacks kCFAUPresetArrayCallBacks = {
	0, 
	CFAUPresetArrayRetainCallBack, 
	CFAUPresetArrayReleaseCallBack, 
	CFAUPresetArrayCopyDescriptionCallBack, 
	CFAUPresetArrayEqualCallBack
};
//#endif

