//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// UIKit/UIView.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "UIKitPrivate.hpp"
#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>

namespace UI
{
    _NS_OPTIONS(uint32_t, ViewAutoresizing) {
        ViewAutoresizingNone                 = 0,
        ViewAutoresizingFlexibleLeftMargin   = 1 << 0,
        ViewAutoresizingFlexibleWidth        = 1 << 1,
        ViewAutoresizingFlexibleRightMargin  = 1 << 2,
        ViewAutoresizingFlexibleTopMargin    = 1 << 3,
        ViewAutoresizingFlexibleHeight       = 1 << 4,
        ViewAutoresizingFlexibleBottomMargin = 1 << 5
    };

    class View : public NS::Referencing< View >
	{
		public:
			View* init( CGRect frame );

            void addSubview( const View *view );

            void setAutoresizingMask( ViewAutoresizing resizingMask );
            void setFrame( CGRect frame );

            struct CGRect bounds();
            struct CGRect frame();
	};
}


_NS_INLINE UI::View* UI::View::init( CGRect frame )
{
	return Object::sendMessage< View* >( _UI_PRIVATE_CLS( UIView ), _UI_PRIVATE_SEL( initWithFrame_ ), frame );
}

_NS_INLINE void UI::View::addSubview( const View *view )
{
    Object::sendMessage< void >( this, _UI_PRIVATE_SEL ( addSubview_ ), view );
}

_NS_INLINE void UI::View::setAutoresizingMask( ViewAutoresizing resizingMask )
{
    Object::sendMessage< void >(this, _UI_PRIVATE_SEL( setAutoresizingMask_ ), resizingMask );
}

_NS_INLINE struct CGRect UI::View::bounds()
{
  return Object::sendMessage< struct CGRect >( this, _UI_PRIVATE_SEL( bounds ) );
}

_NS_INLINE struct CGRect UI::View::frame()
{
  return Object::sendMessage< struct CGRect >( this, _UI_PRIVATE_SEL( frame ) );
}

_NS_INLINE void UI::View::setFrame( CGRect frame )
{
	return Object::sendMessage< void >( this, _UI_PRIVATE_SEL( setFrame_ ), frame );
}
