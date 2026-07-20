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
// MetalPerformanceShaders/MPSCore/MPSImage.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Metal/Metal.hpp>
#include <MetalPerformanceShaders/MetalPerformanceShadersPrivate.hpp>
#include "MPSCoreTypes.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS
{
	class ImageDescriptor : public NS::Referencing< ImageDescriptor >
	{
		public:
			static ImageDescriptor*	imageDescriptor( MPS::ImageFeatureChannelFormat channelFormat, NS::UInteger width, NS::UInteger height, NS::UInteger featureChannels );
	};

	class Image : public NS::Referencing< Image >
	{
		public:
			static Image*	alloc();
			Image*			init( const MTL::Device* pDevice, const MPS::ImageDescriptor* pImageDescriptor );
			Image*			init( const MTL::Texture* pTexture, unsigned featureChannels );
			MTL::Texture*	texture();
	};
}

_NS_INLINE MPS::ImageDescriptor* MPS::ImageDescriptor::imageDescriptor( MPS::ImageFeatureChannelFormat channelFormat, NS::UInteger width, NS::UInteger height, NS::UInteger featureChannels )
{
	return NS::Object::sendMessage< ImageDescriptor* >( _MPS_PRIVATE_CLS( MPSImageDescriptor ), _MPS_PRIVATE_SEL( imageDescriptorWithChannelFormat_width_height_featureChannels_ ), channelFormat, width, height, featureChannels );
}

_NS_INLINE MPS::Image* MPS::Image::alloc()
{
	return NS::Object::alloc< Image >( _MPS_PRIVATE_CLS( MPSImage ) );
}

_NS_INLINE MPS::Image* MPS::Image::init( const MTL::Device* pDevice, const MPS::ImageDescriptor* pImageDescriptor )
{
	return NS::Object::sendMessage< Image* >( this, _MPS_PRIVATE_SEL( initWithDevice_imageDescriptor_ ), pDevice, pImageDescriptor );
}

_NS_INLINE MPS::Image* MPS::Image::init( const MTL::Texture* pTexture, unsigned featureChannels )
{
	return NS::Object::sendMessage< Image* >( this, _MPS_PRIVATE_SEL( initWithTexture_featureChannels_ ), pTexture, featureChannels );
}

_NS_INLINE MTL::Texture* MPS::Image::texture()
{
	return NS::Object::sendMessage< MTL::Texture* >( this, _MPS_PRIVATE_SEL( texture ) );
}
