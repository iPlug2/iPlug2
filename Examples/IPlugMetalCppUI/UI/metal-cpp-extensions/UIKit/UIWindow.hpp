//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// UIKit/UIWindow.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "UIKitPrivate.hpp"
#include "UIView.hpp"
#include "UIViewController.hpp"

#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>

namespace UI
{
	class Window : public NS::Referencing< Window, UI::View >
	{
		public:
			static Window* alloc();
			Window* init( CGRect frame );

            void setRootViewController(UI::ViewController *viewController);
            void makeKeyAndVisible();
	};

}


_NS_INLINE UI::Window* UI::Window::alloc()
{
    return NS::Object::alloc< Window >( _UI_PRIVATE_CLS( UIWindow ) );
}

_NS_INLINE UI::Window* UI::Window::init( CGRect frame )
{
	return Object::sendMessage< Window* >( this, _UI_PRIVATE_SEL( initWithFrame_ ), frame );
}

_NS_INLINE void UI::Window::setRootViewController(UI::ViewController *viewController)
{
    Object::sendMessage< void >( this, _UI_PRIVATE_SEL( setRootViewController_ ), viewController );
}

_NS_INLINE void UI::Window::makeKeyAndVisible()
{
	Object::sendMessage< void >( this, _UI_PRIVATE_SEL( makeKeyAndVisible ) );
}
