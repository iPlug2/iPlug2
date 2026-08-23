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
// AppKit/AppKitPrivate.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <objc/runtime.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _APPKIT_PRIVATE_CLS( symbol )				   ( Private::Class::s_k ## symbol )
#define _APPKIT_PRIVATE_SEL( accessor )				 ( Private::Selector::s_k ## accessor )

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined( NS_PRIVATE_IMPLEMENTATION )

#define _APPKIT_PRIVATE_VISIBILITY						__attribute__( ( visibility( "default" ) ) )
#define _APPKIT_PRIVATE_IMPORT						  __attribute__( ( weak_import ) )

#if __OBJC__
#define  _APPKIT_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   ( ( __bridge void* ) objc_lookUpClass( # symbol ) )
#else
#define  _APPKIT_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   objc_lookUpClass( # symbol )
#endif // __OBJC__

#define _APPKIT_PRIVATE_DEF_CLS( symbol )				void*				   s_k ## symbol 	_NS_PRIVATE_VISIBILITY = _NS_PRIVATE_OBJC_LOOKUP_CLASS( symbol );
#define _APPKIT_PRIVATE_DEF_SEL( accessor, symbol )	 SEL					 s_k ## accessor	_NS_PRIVATE_VISIBILITY = sel_registerName( symbol );
#define _APPKIT_PRIVATE_DEF_CONST( type, symbol )	   _NS_EXTERN type const   NS ## symbol   _NS_PRIVATE_IMPORT; \
													type const			  NS::symbol	 = ( nullptr != &NS ## symbol ) ? NS ## symbol : nullptr;


#else

#define _APPKIT_PRIVATE_DEF_CLS( symbol )				extern void*			s_k ## symbol;
#define _APPKIT_PRIVATE_DEF_SEL( accessor, symbol )	 extern SEL			  s_k ## accessor;
#define _APPKIT_PRIVATE_DEF_CONST( type, symbol )


#endif // NS_PRIVATE_IMPLEMENTATION

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS::Private::Class {

_APPKIT_PRIVATE_DEF_CLS( NSApplication );
_APPKIT_PRIVATE_DEF_CLS( NSRunningApplication );
_APPKIT_PRIVATE_DEF_CLS( NSView );
_APPKIT_PRIVATE_DEF_CLS( NSGridView );
_APPKIT_PRIVATE_DEF_CLS( NSStackView );
_APPKIT_PRIVATE_DEF_CLS( NSWindow );
_APPKIT_PRIVATE_DEF_CLS( NSMenu );
_APPKIT_PRIVATE_DEF_CLS( NSMenuItem );

} // Class

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS::Private::Selector
{

_APPKIT_PRIVATE_DEF_SEL( addItem_,
						"addItem:" );

_APPKIT_PRIVATE_DEF_SEL( addItemWithTitle_action_keyEquivalent_,
						"addItemWithTitle:action:keyEquivalent:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidBecomeActive_,
						"applicationDidBecomeActive:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidChangeOcclusionState_,
						"applicationDidChangeOcclusionState:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidChangeScreenParameters_,
						"applicationDidChangeScreenParameters:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidFinishLaunching_,
						"applicationDidFinishLaunching:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidHide_,
						"applicationDidHide:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidResignActive_,
						"applicationDidResignActive:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidUnhide_,
						"applicationDidUnhide:" );

_APPKIT_PRIVATE_DEF_SEL( applicationDidUpdate_,
						"applicationDidUpdate:" );

_APPKIT_PRIVATE_DEF_SEL( applicationProtectedDataDidBecomeAvailable_,
						"applicationProtectedDataDidBecomeAvailable:" );

_APPKIT_PRIVATE_DEF_SEL( applicationProtectedDataWillBecomeUnavailable_,
						"applicationProtectedDataWillBecomeUnavailable:" );

_APPKIT_PRIVATE_DEF_SEL( applicationShouldTerminateAfterLastWindowClosed_,
						"applicationShouldTerminateAfterLastWindowClosed:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillBecomeActive_,
						"applicationWillBecomeActive:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillFinishLaunching_,
						"applicationWillFinishLaunching:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillHide_,
						"applicationWillHide:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillResignActive_,
						"applicationWillResignActive:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillTerminate_,
						"applicationWillTerminate:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillUnhide_,
						"applicationWillUnhide:" );

_APPKIT_PRIVATE_DEF_SEL( applicationWillUpdate_,
						"applicationWillUpdate:" );

_APPKIT_PRIVATE_DEF_SEL( autoresizingMask_,
						"autoresizingMask:" );

_APPKIT_PRIVATE_DEF_SEL( close,
						"close" );

_APPKIT_PRIVATE_DEF_SEL( currentApplication,
						"currentApplication" );

_APPKIT_PRIVATE_DEF_SEL( distribution,
						"distribution" );

_APPKIT_PRIVATE_DEF_SEL( keyEquivalentModifierMask,
 						"keyEquivalentModifierMask" );

_APPKIT_PRIVATE_DEF_SEL( localizedName,
						"localizedName" );

_APPKIT_PRIVATE_DEF_SEL( sharedApplication,
						"sharedApplication" );

_APPKIT_PRIVATE_DEF_SEL( setDelegate_,
						"setDelegate:" );

_APPKIT_PRIVATE_DEF_SEL( setActivationPolicy_,
						"setActivationPolicy:" );

_APPKIT_PRIVATE_DEF_SEL( spacing,
						"spacing" );

_APPKIT_PRIVATE_DEF_SEL( activateIgnoringOtherApps_,
						"activateIgnoringOtherApps:" );

_APPKIT_PRIVATE_DEF_SEL( run,
						"run" );

_APPKIT_PRIVATE_DEF_SEL( terminate_,
						"terminate:" );

_APPKIT_PRIVATE_DEF_SEL( gridViewWithViews_,
						"gridViewWithViews:" );

_APPKIT_PRIVATE_DEF_SEL( stackViewWithViews_,
						"stackViewWithViews:" );

_APPKIT_PRIVATE_DEF_SEL( initWithContentRect_styleMask_backing_defer_,
						"initWithContentRect:styleMask:backing:defer:" );

_APPKIT_PRIVATE_DEF_SEL( initWithFrame_,
						"initWithFrame:" );

_APPKIT_PRIVATE_DEF_SEL( initWithTitle_,
						"initWithTitle:" );

_APPKIT_PRIVATE_DEF_SEL( setContentView_,
						"setContentView:" );

_APPKIT_PRIVATE_DEF_SEL( makeKeyAndOrderFront_,
						"makeKeyAndOrderFront:" );

_APPKIT_PRIVATE_DEF_SEL( orientation,
						"orientation" );

_APPKIT_PRIVATE_DEF_SEL( setAutoresizingMask_,
						"setAutoresizingMask:" );

_APPKIT_PRIVATE_DEF_SEL( setDistribution_,
						"setDistribution:" );

_APPKIT_PRIVATE_DEF_SEL( setFrameSize_,
						"setFrameSize:" );

_APPKIT_PRIVATE_DEF_SEL( setFrame_,
						"setFrame:" );

_APPKIT_PRIVATE_DEF_SEL( setKeyEquivalentModifierMask_,
						"setKeyEquivalentModifierMask:" );

_APPKIT_PRIVATE_DEF_SEL( setMainMenu_,
						"setMainMenu:" );

_APPKIT_PRIVATE_DEF_SEL( setOrientation_,
						"setOrientation:" );

_APPKIT_PRIVATE_DEF_SEL( setSpacing_,
						"setSpacing:" );

_APPKIT_PRIVATE_DEF_SEL( setSubmenu_,
						"setSubmenu:" );

_APPKIT_PRIVATE_DEF_SEL( setTitle_,
						"setTitle:" );

_APPKIT_PRIVATE_DEF_SEL( windows,
						"windows" );

_APPKIT_PRIVATE_DEF_SEL( addSubview_,
            "addSubview:" );

_APPKIT_PRIVATE_DEF_SEL( bounds,
            "bounds" );

_APPKIT_PRIVATE_DEF_SEL( frame,
            "frame" );
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
