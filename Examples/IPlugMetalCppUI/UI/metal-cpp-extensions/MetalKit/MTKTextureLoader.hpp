/*
 *
 * Copyright 2020-2021 Mark Grimes.
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
// MetalKit/MTKTextureLoader.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "MetalKitPrivate.hpp"

#include <Metal/Metal.hpp>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MTK
{

	class TextureLoader : public NS::Referencing< TextureLoader >
	{
		public:
			static TextureLoader*		alloc();
			TextureLoader*				init( const MTL::Device* pDevice );

			void						setDevice( const MTL::Device* pDevice );
			MTL::Device*				device() const;

			MTL::Texture*				newTexture( const NS::URL* URL, const NS::Dictionary* options, NS::Error** error );

	};
}

_NS_INLINE MTK::TextureLoader* MTK::TextureLoader::alloc()
{
	return NS::Object::alloc< TextureLoader >( _MTK_PRIVATE_CLS( MTKTextureLoader ) );
}

_NS_INLINE MTK::TextureLoader* MTK::TextureLoader::init( const MTL::Device* pDevice )
{
	return NS::Object::sendMessage< TextureLoader* >( this, _MTK_PRIVATE_SEL( initWithDevice_ ), pDevice );
}

_NS_INLINE void MTK::TextureLoader::setDevice( const MTL::Device* pDevice )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setDevice_ ), pDevice );
}

_NS_INLINE MTL::Device* MTK::TextureLoader::device() const
{
	return NS::Object::sendMessage< MTL::Device* >( this, _MTK_PRIVATE_SEL( device ) );
}

_NS_INLINE MTL::Texture* MTK::TextureLoader::newTexture( const NS::URL* URL, const NS::Dictionary* options, NS::Error** error )
{
	return NS::Object::sendMessage< MTL::Texture* >( this, _MTK_PRIVATE_SEL( newTextureWithContentsOfURL_options_error_ ), URL, options, error );
}
