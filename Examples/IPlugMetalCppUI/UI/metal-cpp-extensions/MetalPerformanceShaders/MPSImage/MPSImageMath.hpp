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
// MetalPerformanceShaders/MPSImage/MPSImageMath.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Metal/Metal.hpp>
#include <MetalPerformanceShaders/MetalPerformanceShadersPrivate.hpp>
namespace MPS
{
	class Image;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS
{
	class BinaryImageKernel : public NS::Referencing< BinaryImageKernel >
	{
	public:
		void encode( const MTL::CommandBuffer* pCommandBuffer, const MPS::Image* pPrimaryImage, MPS::Image* pSecondaryImage, MPS::Image* pDestinationImage ) const;
	};

	class ImageArithmetic : public NS::Referencing< ImageArithmetic, BinaryImageKernel >
	{
		// TODO: Add methods here when required
	};

	class ImageSubtract : public NS::Referencing< ImageSubtract, ImageArithmetic >
	{
	public:
		static ImageSubtract*	alloc();
		ImageSubtract*			init( const MTL::Device* pDevice );
	};
}

_NS_INLINE void MPS::BinaryImageKernel::encode( const MTL::CommandBuffer* pCommandBuffer, const MPS::Image* pPrimaryImage, MPS::Image* pSecondaryImage, MPS::Image* pDestinationImage ) const
{
	return NS::Object::sendMessage< void >( this, _MPS_PRIVATE_SEL( encodeToCommandBuffer_primaryImage_secondaryImage_destinationImage_ ), pCommandBuffer, pPrimaryImage, pSecondaryImage, pDestinationImage );
}

_NS_INLINE MPS::ImageSubtract* MPS::ImageSubtract::alloc()
{
	return NS::Object::alloc< ImageSubtract >( _MPS_PRIVATE_CLS( MPSImageSubtract ) );
}

_NS_INLINE MPS::ImageSubtract* MPS::ImageSubtract::init( const MTL::Device* pDevice )
{
	return NS::Object::sendMessage< ImageSubtract* >( this, _MPS_PRIVATE_SEL( initWithDevice_ ), pDevice );
}
