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
// MetalPerformanceShaders/MPSImage/MPSImageConvolution.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Metal/Metal.hpp>
#include <MetalPerformanceShaders/MetalPerformanceShadersPrivate.hpp>
#include <MetalPerformanceShaders/MetalPerformanceShadersDefines.hpp>
#include "MPSImageKernel.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MPS
{

	class ImageGaussianBlur : public NS::Referencing< ImageGaussianBlur, UnaryImageKernel >
	{
		public:
			static ImageGaussianBlur*	alloc();
			ImageGaussianBlur*			init( const MTL::Device* pDevice, float sigma );

			void						setSigma( float sigma );
			float						sigma() const;
	};

	class ImageSobel : public NS::Referencing<ImageSobel, UnaryImageKernel>
	{
	public:
		static ImageSobel*	alloc();
		MPS::ImageSobel* init( const MTL::Device* device );
		MPS::ImageSobel* init( const MTL::Device* device, const float* transform );
		const float* colorTransform() const;
	}; // end of class ImageSobel

	class ImageCanny : public NS::Referencing<ImageCanny, UnaryImageKernel>
	{
	public:
		static ImageCanny*	alloc();
		MPS::ImageCanny* init(const MTL::Device* device);
		MPS::ImageCanny* init(const MTL::Device* device, const float * transform, const float sigma);
		const float* colorTransform() const;
		float sigma() const;
		float highThreshold() const;
		void setHighThreshold(float highThreshold);
		float lowThreshold() const;
		void setLowThreshold(float lowThreshold);
		BOOL useFastMode() const;
		void setUseFastMode(BOOL useFastMode);
	}; // end of class ImageCanny

	class ImagePyramid : public NS::Referencing< ImagePyramid, UnaryImageKernel >
	{
		public:
			NS::UInteger				kernelHeight() const;
			NS::UInteger				kernelWidth() const;
	};

	class ImageGaussianPyramid : public NS::Referencing< ImageGaussianPyramid, ImagePyramid >
	{
		public:
			static ImageGaussianPyramid* alloc();
			ImageGaussianPyramid*		init( const MTL::Device* pDevice, float centerWeight );
	};
}

_MPS_INLINE MPS::ImageGaussianBlur* MPS::ImageGaussianBlur::alloc()
{
	return NS::Object::alloc< ImageGaussianBlur >( _MPS_PRIVATE_CLS( MPSImageGaussianBlur ) );
}

_MPS_INLINE MPS::ImageGaussianBlur* MPS::ImageGaussianBlur::init( const MTL::Device* pDevice, float sigma )
{
	return NS::Object::sendMessage< ImageGaussianBlur* >( this, _MPS_PRIVATE_SEL( initWithDevice_sigma_ ), pDevice, sigma );
}

_MPS_INLINE void MPS::ImageGaussianBlur::setSigma( float sigma )
{
	NS::Object::sendMessage< void >( this, _MPS_PRIVATE_SEL( setSigma_ ), sigma );
}

_MPS_INLINE float MPS::ImageGaussianBlur::sigma() const
{
	return NS::Object::sendMessage< float >( this, _MPS_PRIVATE_SEL( sigma ) );
}

// ------------------------------------------------------------

_MPS_INLINE MPS::ImageSobel* MPS::ImageSobel::alloc()
{
	return NS::Object::alloc<ImageSobel>( _MPS_PRIVATE_CLS( MPSImageSobel ) );
}

_MPS_INLINE MPS::ImageSobel* MPS::ImageSobel::init(const MTL::Device* device)
{
	return Object::sendMessage<MPS::ImageSobel*>( this, _MPS_PRIVATE_SEL(initWithDevice_), device );
}

_MPS_INLINE MPS::ImageSobel* MPS::ImageSobel::init(const MTL::Device* device, const float * transform)
{
	return Object::sendMessage<MPS::ImageSobel*>( this, _MPS_PRIVATE_SEL(initWithDevice_linearGrayColorTransform_), device, transform );
}

_MPS_INLINE const float* MPS::ImageSobel::colorTransform() const
{
	return Object::sendMessage<const float *>( this, _MPS_PRIVATE_SEL(colorTransform) );
}

// ------------------------------------------------------------

_MPS_INLINE MPS::ImageCanny* MPS::ImageCanny::alloc()
{
	return NS::Object::alloc<ImageCanny>( _MPS_PRIVATE_CLS( MPSImageCanny ) );
}

_MPS_INLINE MPS::ImageCanny* MPS::ImageCanny::init(const MTL::Device* device)
{
	return Object::sendMessage<MPS::ImageCanny*>( this, _MPS_PRIVATE_SEL(initWithDevice_), device );
}

_MPS_INLINE MPS::ImageCanny* MPS::ImageCanny::init(const MTL::Device* device, const float * transform, const float sigma)
{
	return Object::sendMessage<MPS::ImageCanny*>( this, _MPS_PRIVATE_SEL(initWithDevice_linearToGrayScaleTransform_sigma_), device, transform, sigma );
}

_MPS_INLINE const float* MPS::ImageCanny::colorTransform() const
{
	return Object::sendMessage<const float *>( this, _MPS_PRIVATE_SEL(colorTransform) );
}

_MPS_INLINE float MPS::ImageCanny::sigma() const
{
	return Object::sendMessage<float>( this, _MPS_PRIVATE_SEL(sigma) );
}

_MPS_INLINE float MPS::ImageCanny::highThreshold() const
{
	return Object::sendMessage<float>( this, _MPS_PRIVATE_SEL(highThreshold) );
}

_MPS_INLINE void MPS::ImageCanny::setHighThreshold(float highThreshold)
{
	return Object::sendMessage<void>( this, _MPS_PRIVATE_SEL(setHighThreshold_), highThreshold );
}

_MPS_INLINE float MPS::ImageCanny::lowThreshold() const
{
	return Object::sendMessage<float>( this, _MPS_PRIVATE_SEL(lowThreshold) );
}

_MPS_INLINE void MPS::ImageCanny::setLowThreshold(float lowThreshold)
{
	return Object::sendMessage<void>( this, _MPS_PRIVATE_SEL(setLowThreshold_), lowThreshold );
}

_MPS_INLINE BOOL MPS::ImageCanny::useFastMode() const
{
	return Object::sendMessage<BOOL>( this, _MPS_PRIVATE_SEL(useFastMode) );
}

_MPS_INLINE void MPS::ImageCanny::setUseFastMode(BOOL useFastMode)
{
	return Object::sendMessage<void>( this, _MPS_PRIVATE_SEL(setUseFastMode_), useFastMode );
}

// ------------------------------------------------------------

_MPS_INLINE NS::UInteger MPS::ImagePyramid::kernelHeight() const
{
	return NS::Object::sendMessage< NS::UInteger >( this, _MPS_PRIVATE_SEL( kernelHeight ) );
}

_MPS_INLINE NS::UInteger MPS::ImagePyramid::kernelWidth() const
{
	return NS::Object::sendMessage< NS::UInteger >( this, _MPS_PRIVATE_SEL( kernelWidth ) );
}

// ------------------------------------------------------------

_MPS_INLINE MPS::ImageGaussianPyramid* MPS::ImageGaussianPyramid::alloc()
{
	return NS::Object::alloc< ImageGaussianPyramid >( _MPS_PRIVATE_CLS( MPSImageGaussianPyramid ) );
}

_MPS_INLINE MPS::ImageGaussianPyramid* MPS::ImageGaussianPyramid::init( const MTL::Device* pDevice, float centerWeight )
{
	return NS::Object::sendMessage< ImageGaussianPyramid* >( this, _MPS_PRIVATE_SEL( initWithDevice_centerWeight_ ), pDevice, centerWeight );
}
