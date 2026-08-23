/*
 *
 * Copyright 2020-2021 Apple Inc.
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
// AppKit/NSView.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "AppKitPrivate.hpp"
#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>

namespace NS
{
	_NS_OPTIONS(NS::UInteger, AutoresizingMaskOptions) {
		ViewNotSizable			=  0,
		ViewMinXMargin			=  1,
		ViewWidthSizable		=  2,
		ViewMaxXMargin			=  4,
		ViewMinYMargin			=  8,
		ViewHeightSizable		= 16,
		ViewMaxYMargin			= 32
	};

	class View : public NS::Referencing< View >
	{
		public:
			View*						init( CGRect frame );
			NS::AutoresizingMaskOptions autoresizingMask() const;
			void                        setAutoresizingMask( NS::AutoresizingMaskOptions newMask );
			void                        setFrameSize( CGSize newSize );
			void                        setFrame( CGRect frame );
			void						addSubview( const View* view );
			struct CGRect 				bounds();
			struct CGRect 				frame();
	};
}


_NS_INLINE NS::View* NS::View::init( CGRect frame )
{
	return Object::sendMessage< View* >( _APPKIT_PRIVATE_CLS( NSView ), _APPKIT_PRIVATE_SEL( initWithFrame_ ), frame );
}

_NS_INLINE NS::AutoresizingMaskOptions NS::View::autoresizingMask() const
{
	return Object::sendMessage< NS::AutoresizingMaskOptions >( this, _APPKIT_PRIVATE_SEL( autoresizingMask_ ) );
}

_NS_INLINE void NS::View::setAutoresizingMask( NS::AutoresizingMaskOptions newMask )
{
	return Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setAutoresizingMask_ ), newMask );
}

_NS_INLINE void NS::View::setFrameSize( CGSize newSize )
{
	return Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setFrameSize_ ), newSize );
}

_NS_INLINE void NS::View::addSubview( const NS::View* view )
{
	return Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( addSubview_ ), view );
}

_NS_INLINE struct CGRect NS::View::bounds()
{
	return Object::sendMessage< struct CGRect >( this, _APPKIT_PRIVATE_SEL( bounds ) );
}

_NS_INLINE struct CGRect NS::View::frame()
{
	return Object::sendMessage< struct CGRect >( this, _APPKIT_PRIVATE_SEL( frame ) );
}

_NS_INLINE void NS::View::setFrame( CGRect frame )
{
	return Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setFrame_ ), frame );
}
