//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// UIKit/UIKitPrivate.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <objc/runtime.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _UI_PRIVATE_CLS( symbol )   ( Private::Class::s_k ## symbol )
#define _UI_PRIVATE_SEL( accessor ) ( Private::Selector::s_k ## accessor )

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined( UI_PRIVATE_IMPLEMENTATION )

#define _UI_PRIVATE_VISIBILITY  __attribute__( ( visibility( "default" ) ) )
#define _UI_PRIVATE_IMPORT      __attribute__( ( weak_import ) )

#if __OBJC__
#define _UI_PRIVATE_OBJC_LOOKUP_CLASS( symbol  ) ( ( __bridge void* ) objc_lookUpClass( # symbol ) )
#else
#define _UI_PRIVATE_OBJC_LOOKUP_CLASS( symbol  ) objc_lookUpClass( # symbol )
#endif // __OBJC__

#define _UI_PRIVATE_DEF_CLS( symbol ) void* s_k ## symbol _UI_PRIVATE_VISIBILITY = _UI_PRIVATE_OBJC_LOOKUP_CLASS( symbol );
#define _UI_PRIVATE_DEF_SEL( accessor, symbol ) SEL s_k ## accessor _UI_PRIVATE_VISIBILITY = sel_registerName( symbol );
#define _UI_PRIVATE_DEF_CONST( type, symbol ) _NS_EXTERN type const  UI ## symbol _UI_PRIVATE_IMPORT; \
type const UI::symbol = ( nullptr != &UI ## symbol ) ? UI ## symbol : nullptr;

#else

#define _UI_PRIVATE_DEF_CLS( symbol ) extern void* s_k ## symbol;
#define _UI_PRIVATE_DEF_SEL( accessor, symbol ) extern SEL s_k ## accessor;
#define _UI_PRIVATE_DEF_CONST( type, symbol )

#endif // UI_PRIVATE_IMPLEMENTATION

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace UI::Private::Class {

_UI_PRIVATE_DEF_CLS( UIApplication );
_UI_PRIVATE_DEF_CLS( UIScreen );
_UI_PRIVATE_DEF_CLS( UIView );
_UI_PRIVATE_DEF_CLS( UIViewController );
_UI_PRIVATE_DEF_CLS( UIWindow );

} // Class

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace UI::Private::Selector
{

_UI_PRIVATE_DEF_SEL( addSubview_,
                    "addSubview:" );

_UI_PRIVATE_DEF_SEL( applicationDidFinishLaunching_withOptions_,
                    "applicationDidFinishLaunching:withOptions:" );

_UI_PRIVATE_DEF_SEL( applicationWillTerminate_,
                    "applicationWillTerminate:" );

_UI_PRIVATE_DEF_SEL( bounds,
                    "bounds" );

_UI_PRIVATE_DEF_SEL( initWithFrame_,
                    "initWithFrame:" );

_UI_PRIVATE_DEF_SEL( initWithNibName_bundle_,
                    "initWithNibName:bundle:" );

_UI_PRIVATE_DEF_SEL( mainScreen,
                    "mainScreen" );

_UI_PRIVATE_DEF_SEL( makeKeyAndVisible,
                    "makeKeyAndVisible" );

_UI_PRIVATE_DEF_SEL( setAutoresizingMask_,
                    "setAutoresizingMask:" );

_UI_PRIVATE_DEF_SEL( setDelegate_,
                    "setDelegate:" );

_UI_PRIVATE_DEF_SEL( setRootViewController_,
                    "setRootViewController:" );

_UI_PRIVATE_DEF_SEL( sharedApplication,
                    "sharedApplication" );

_UI_PRIVATE_DEF_SEL( view,
                    "view" );

_UI_PRIVATE_DEF_SEL( frame,
            "frame" );

_UI_PRIVATE_DEF_SEL( setFrame_,
                    "setFrame:" );
                    
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
