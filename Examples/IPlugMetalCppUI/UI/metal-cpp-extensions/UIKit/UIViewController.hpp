//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// UIKit/UIViewController.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "UIKitPrivate.hpp"
#include "UIView.hpp"

#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>

namespace UI
{
	class ViewController : public NS::Referencing< ViewController >
	{
		public:
            static ViewController* alloc();
			ViewController* init( NS::String *nibNameOrNil, NS::Value *nibBundleOrNil );

            View *view();
	};
}

_NS_INLINE UI::ViewController* UI::ViewController::alloc()
{
    return NS::Object::alloc< ViewController >( _UI_PRIVATE_CLS( UIViewController ) );
}

_NS_INLINE UI::ViewController* UI::ViewController::init( NS::String *nibNameOrNil, NS::Value *nibBundleOrNil )
{
	return Object::sendMessage< ViewController* >( this, _UI_PRIVATE_SEL( initWithNibName_bundle_ ), nibNameOrNil, nibBundleOrNil );
}

_NS_INLINE UI::View* UI::ViewController::view()
{
    return Object::sendMessage< View* >( this, _UI_PRIVATE_SEL( view ) );
}
