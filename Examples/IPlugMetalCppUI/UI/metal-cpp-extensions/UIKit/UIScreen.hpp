//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// UIKit/UIScreen.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "UIKitPrivate.hpp"
#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>

namespace UI
{
	class Screen : public NS::Referencing< Screen >
	{
		public:
            static Screen *mainScreen();

            struct CGRect bounds();
	};
}


_NS_INLINE UI::Screen* UI::Screen::mainScreen()
{
	return Object::sendMessage< Screen* >( _UI_PRIVATE_CLS( UIScreen ), _UI_PRIVATE_SEL( mainScreen ) );
}

_NS_INLINE struct CGRect UI::Screen::bounds()
{
    return Object::sendMessage< struct CGRect >( this, _UI_PRIVATE_SEL( bounds ) );
}
