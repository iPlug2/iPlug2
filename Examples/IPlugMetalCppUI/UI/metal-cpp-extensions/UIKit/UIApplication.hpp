//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// UIKit/UIApplication.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Foundation/Foundation.hpp>
#include "UIKitPrivate.hpp"

#include <objc/message.h>

// Prototype for the  UIKit entry point. This *must* be called and allowed
// to run for the duration of the app life cycle, since it does all sorts
// of magic we can't possibly hope to replicate.
extern "C" int UIApplicationMain(int argc, char **, id, id);

namespace UI
{
    class Application;

	class ApplicationDelegate
	{
		public:
			virtual ~ApplicationDelegate() { }
            virtual bool applicationDidFinishLaunching( UI::Application* pApp, NS::Value *options ) { return true; }
            virtual void applicationWillTerminate( UI::Application* pApp ) { }
	};

	class Application : public NS::Referencing< Application >
	{
		public:
            // A reference to the shared application instance.
            // Returns null before UI::ApplicationMain is called
			static Application* sharedApplication();

			void setDelegate( const ApplicationDelegate* pDelegate );

        private:
            NS::Value *delegateWrapper;
	};
}

// A temporary reference to the C++ app delegate that is populated in UI::ApplicationMain
// and cleared once the shared application object has been created by UIKit
static UI::ApplicationDelegate *s_appDelegate = nullptr;

namespace UI {
    _NS_INLINE int ApplicationMain(int argc, char **argv, ApplicationDelegate *appDelegate) {
        s_appDelegate = appDelegate;
        return UIApplicationMain(argc, argv, nil, (id)CFSTR("BootstrapAppDelegate"));
    }
}

_NS_INLINE UI::Application* UI::Application::sharedApplication()
{
	return Object::sendMessage< Application* >( _UI_PRIVATE_CLS( UIApplication ), _UI_PRIVATE_SEL( sharedApplication ) );
}

_NS_INLINE void UI::Application::setDelegate( const ApplicationDelegate* pAppDelegate )
{
    if ( delegateWrapper != NULL) {
        delegateWrapper->release();
    }
    delegateWrapper = NS::Value::value( pAppDelegate );
    delegateWrapper->retain();

    typedef bool (*DidFinishLaunchingFunction)(NS::Value*, SEL, UI::Application*, NS::Value *);
    typedef void (*WillTerminateFunction)(NS::Value*, SEL, UI::Application *);

    DidFinishLaunchingFunction didFinishLaunching = []( NS::Value* pSelf, SEL, UI::Application *pApp, NS::Value *options ) {
		auto pDel = reinterpret_cast< UI::ApplicationDelegate* >( pSelf->pointerValue() );
		return pDel->applicationDidFinishLaunching( pApp, options );
	};

    WillTerminateFunction willTerminate = []( NS::Value* pSelf, SEL, UI::Application *pApplication ) {
		auto pDel = reinterpret_cast< UI::ApplicationDelegate* >( pSelf->pointerValue() );
        pDel->applicationWillTerminate(pApplication);
	};

	class_addMethod( (Class)objc_lookUpClass( "NSValue" ), _UI_PRIVATE_SEL( applicationDidFinishLaunching_withOptions_ ), (IMP)didFinishLaunching, "b@:@@" );
    class_addMethod( (Class)objc_lookUpClass( "NSValue" ), _UI_PRIVATE_SEL( applicationWillTerminate_ ), (IMP)willTerminate, "v@:@");

	Object::sendMessage< void >( this, _UI_PRIVATE_SEL( setDelegate_ ), delegateWrapper );
}

// A stripped-down app delegate whose only purpose is to receive application:didFinishLaunchingWithOptions:
// and wire up the actual C++ app delegate
struct BootstrapAppDelegate {
    Class isa;
};

int BootstrapAppDelegate_didFinishLaunching(struct BootstrapAppDelegate *self, SEL _cmd, void *application, void *options)
{
    if (s_appDelegate) {
        UI::Application *pApp = UI::Application::sharedApplication();
        pApp->setDelegate(s_appDelegate);
        s_appDelegate->applicationDidFinishLaunching(UI::Application::sharedApplication(), NS::Value::value( options ) );
        s_appDelegate = NULL;
    }
    return 1;
}

static Class s_BootstrapAppDelegate;

// Ths function runs before main() to inform the Obj-C runtime about our bootstrap app delegate type
__attribute__((constructor))
static void StaticInitApplicationRuntime()
{
    s_BootstrapAppDelegate = objc_allocateClassPair(objc_getClass("UIResponder"), "BootstrapAppDelegate", 0);
    class_addMethod(s_BootstrapAppDelegate, sel_getUid("application:didFinishLaunchingWithOptions:"), (IMP)BootstrapAppDelegate_didFinishLaunching, "i@:@@");
    objc_registerClassPair(s_BootstrapAppDelegate);
}

