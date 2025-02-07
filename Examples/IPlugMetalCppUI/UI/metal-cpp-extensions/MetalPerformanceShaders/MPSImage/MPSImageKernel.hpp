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
// MetalPerformanceShaders/MPSImage/MPSImageKernel.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <functional>
#include <Metal/Metal.hpp>
#include <MetalPerformanceShaders/MetalPerformanceShadersPrivate.hpp>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS
{
	class Kernel : public NS::Referencing< Kernel >
	{
		// TODO: Add base methods here as and when required.
	};

	using CopyAllocator = MTL::Texture* (^)( MPS::Kernel* pFilter, MTL::CommandBuffer* pCommandBuffer, MTL::Texture* pSourceTexture );
	using CopyAllocatorFunction = std::function<MTL::Texture*( MPS::Kernel* pFilter,
	                                                           MTL::CommandBuffer* pCommandBuffer,
	                                                           MTL::Texture* pSourceTexture )>;

	class UnaryImageKernel : public NS::Referencing< UnaryImageKernel, Kernel >
	{
	public:
		bool encode( const MTL::CommandBuffer* pCommandBuffer, MTL::Texture** ppInPlaceTexture, const MPS::CopyAllocator fallbackCopyAllocator ) const;
		bool encode( const MTL::CommandBuffer* pCommandBuffer, MTL::Texture** ppInPlaceTexture, const MPS::CopyAllocatorFunction& fallbackCopyAllocator ) const;
		void encode( const MTL::CommandBuffer* pCommandBuffer, const MTL::Texture* pSourceTexture, MTL::Texture* pDestinationTexture ) const;
	};
}

_NS_INLINE bool MPS::UnaryImageKernel::encode( const MTL::CommandBuffer* pCommandBuffer, MTL::Texture** ppInPlaceTexture, const MPS::CopyAllocator fallbackCopyAllocator ) const
{
	return NS::Object::sendMessage< bool >( this, _MPS_PRIVATE_SEL( encodeToCommandBuffer_inPlaceTexture_fallbackCopyAllocator_ ), pCommandBuffer, ppInPlaceTexture, fallbackCopyAllocator );
}

_NS_INLINE bool MPS::UnaryImageKernel::encode( const MTL::CommandBuffer* pCommandBuffer, MTL::Texture** ppInPlaceTexture, const MPS::CopyAllocatorFunction& fallbackCopyAllocator ) const
{
	// No idea what this `__block` macro does, just copying it from the way Apple do their callback conversions.
	__block CopyAllocatorFunction function = fallbackCopyAllocator;
	return encode( pCommandBuffer, ppInPlaceTexture, ^MTL::Texture* (MPS::Kernel* pFilter, MTL::CommandBuffer* pCommandBuffers, MTL::Texture* pSourceTexture ){ return function( pFilter, pCommandBuffers, pSourceTexture ); } );
}

_NS_INLINE void MPS::UnaryImageKernel::encode( const MTL::CommandBuffer* pCommandBuffer, const MTL::Texture* pSourceTexture, MTL::Texture* pDestinationTexture ) const
{
	return NS::Object::sendMessage< void >( this, _MPS_PRIVATE_SEL( encodeToCommandBuffer_sourceTexture_destinationTexture_ ), pCommandBuffer, pSourceTexture, pDestinationTexture );
}
