/*
 *
 * Copyright 2022 Mark Grimes. Most/all of the work is copied from Apple so copyright is theirs if they want it.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// MetalPerformanceShaders/MetalPerformanceShadersPrivate.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <objc/runtime.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _MPS_PRIVATE_CLS( symbol )				   ( Private::Class::s_k ## symbol )
#define _MPS_PRIVATE_SEL( accessor )				 ( Private::Selector::s_k ## accessor )

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined( MPS_PRIVATE_IMPLEMENTATION )

#define _MPS_PRIVATE_VISIBILITY						__attribute__( ( visibility( "default" ) ) )
#define _MPS_PRIVATE_IMPORT						  __attribute__( ( weak_import ) )

#if __OBJC__
#define  _MPS_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   ( ( __bridge void* ) objc_lookUpClass( # symbol ) )
#else
#define  _MPS_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   objc_lookUpClass( # symbol )
#endif // __OBJC__

#define _MPS_PRIVATE_DEF_CLS( symbol )			   void*				   s_k ## symbol	   _MPS_PRIVATE_VISIBILITY = _MPS_PRIVATE_OBJC_LOOKUP_CLASS( symbol );
#define _MPS_PRIVATE_DEF_SEL( accessor, symbol )	 SEL					 s_k ## accessor	 _MPS_PRIVATE_VISIBILITY = sel_registerName( symbol );
#define _MPS_PRIVATE_DEF_CONST( type, symbol )	   _NS_EXTERN type const   MPS ## symbo		_MPS_PRIVATE_IMPORT; \
													 type const			  MPS::symbol	 = ( nullptr != &MPS ## symbol ) ? MPS ## symbol : nullptr;


#else

#define _MPS_PRIVATE_DEF_CLS( symbol )				extern void*			s_k ## symbol;
#define _MPS_PRIVATE_DEF_SEL( accessor, symbol )	 extern SEL			  s_k ## accessor;
#define _MPS_PRIVATE_DEF_CONST( type, symbol )


#endif // MPS_PRIVATE_IMPLEMENTATION

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS::Private::Class
{
	_MPS_PRIVATE_DEF_CLS( MPSImageDescriptor );
	_MPS_PRIVATE_DEF_CLS( MPSImage );
	_MPS_PRIVATE_DEF_CLS( MPSImageGaussianBlur );
	_MPS_PRIVATE_DEF_CLS( MPSImageSobel );
	_MPS_PRIVATE_DEF_CLS( MPSImageCanny );
	_MPS_PRIVATE_DEF_CLS( MPSImagePyramid );
	_MPS_PRIVATE_DEF_CLS( MPSImageGaussianPyramid );
	_MPS_PRIVATE_DEF_CLS( MPSBinaryImageKernel );
	_MPS_PRIVATE_DEF_CLS( MPSImageArithmetic );
	_MPS_PRIVATE_DEF_CLS( MPSImageSubtract );
} // Class

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS::Private::Selector
{
	_MPS_PRIVATE_DEF_SEL( colorTransform, "colorTransform" );
	_MPS_PRIVATE_DEF_SEL( encodeToCommandBuffer_inPlaceTexture_fallbackCopyAllocator_ "encodeToCommandBuffer:inPlaceTexture:fallbackCopyAllocator:" );
	_MPS_PRIVATE_DEF_SEL( encodeToCommandBuffer_primaryImage_secondaryImage_destinationImage_, "encodeToCommandBuffer:primaryImage:secondaryImage:destinationImage:" );
	_MPS_PRIVATE_DEF_SEL( encodeToCommandBuffer_sourceTexture_destinationTexture_, "encodeToCommandBuffer:sourceTexture:destinationTexture:" );
	_MPS_PRIVATE_DEF_SEL( highThreshold, "highThreshold" );
	_MPS_PRIVATE_DEF_SEL( imageDescriptorWithChannelFormat_width_height_featureChannels_, "imageDescriptorWithChannelFormat:width:height:featureChannels:" );
	_MPS_PRIVATE_DEF_SEL( initWithDevice_, "initWithDevice:" );
	_MPS_PRIVATE_DEF_SEL( initWithTexture_featureChannels_, "initWithTexture:featureChannels:" );
	_MPS_PRIVATE_DEF_SEL( initWithDevice_imageDescriptor_, "initWithDevice:imageDescriptor:" );
	_MPS_PRIVATE_DEF_SEL( initWithDevice_linearGrayColorTransform_, "initWithDevice:linearGrayColorTransform:" );
	_MPS_PRIVATE_DEF_SEL( initWithDevice_linearToGrayScaleTransform_sigma_, "initWithDevice:linearToGrayScaleTransform:sigma:" );
	_MPS_PRIVATE_DEF_SEL( initWithDevice_centerWeight_, "initWithDevice:centerWeight:" );
	_MPS_PRIVATE_DEF_SEL( initWithDevice_sigma_, "initWithDevice:sigma:" );
	_MPS_PRIVATE_DEF_SEL( kernelHeight, "kernelHeight" );
	_MPS_PRIVATE_DEF_SEL( kernelWidth, "kernelWidth" );
	_MPS_PRIVATE_DEF_SEL( lowThreshold, "lowThreshold" );
	_MPS_PRIVATE_DEF_SEL( setHighThreshold_, "setHighThreshold:" );
	_MPS_PRIVATE_DEF_SEL( setLowThreshold_, "setLowThreshold:" );
	_MPS_PRIVATE_DEF_SEL( setSigma_, "setSigma:" );
	_MPS_PRIVATE_DEF_SEL( setUseFastMode_, "setUseFastMode:" );
	_MPS_PRIVATE_DEF_SEL( sigma, "sigma" );
	_MPS_PRIVATE_DEF_SEL( texture, "texture" );
	_MPS_PRIVATE_DEF_SEL( useFastMode, "useFastMode" );
}

//---------
