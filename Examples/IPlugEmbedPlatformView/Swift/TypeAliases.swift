import SwiftUI

// ----------------------------------------------------------------------------------------

// MARK: iOS

#if os(iOS)

import UIKit

typealias PlatformViewController = UIViewController
typealias PlatformHostingController = UIHostingController
typealias PlatformSplitViewController = UISplitViewController

typealias PlatformView = UIView
typealias PlatformVolumeSlider = UISlider
typealias PlatformButton = UIButton
typealias PlatformMenu = UIMenu
typealias PlatformPopUpButton = UIButton
typealias PlatformLabel = UILabel
typealias PlatformTextField = UITextField
typealias PlatformSegmentedControl = UISegmentedControl
typealias PlatformImageView = UIImageView

typealias PlatformColor = UIColor
typealias PlatformFont = UIFont
typealias PlatformRect = CGRect
typealias PlatformBezierPath = UIBezierPath
typealias PlatformImage = UIImage

typealias PlatformGestureRecognizer = UIGestureRecognizer
typealias PlatformPinchGestureRecognizer = UIPinchGestureRecognizer
typealias PlatformPanGestureRecognizer = UIPanGestureRecognizer
typealias PlatformTapGestureRecognizer = UITapGestureRecognizer
typealias PlatformGestureRecognizerDelegate = UIGestureRecognizerDelegate

typealias PlatformScreen = UIScreen

// ----------------------------------------------------------------------------------------

// MARK: macOS

#elseif os(macOS)

import AppKit

typealias PlatformViewController = NSViewController
typealias PlatformHostingController = NSHostingController
typealias PlatformSplitViewController = NSSplitViewController

typealias PlatformView = NSView
typealias PlatformSlider = NSSlider
//typealias PlatformRotationAngleSlider = RotationAngleSlider
typealias PlatformVolumeSlider = NSSlider
typealias PlatformButton = NSButton
typealias PlatformMenu = NSMenu
typealias PlatformPopUpButton = NSPopUpButton
typealias PlatformCheckButton = NSButton
typealias PlatformLabel = NSTextField
typealias PlatformTextField = NSTextField
//typealias PlatformLinkDetectingTextView = LinkDetectingNSTextView
typealias PlatformSwitch = NSSwitch
typealias PlatformSegmentedControl = NSSegmentedControl
typealias PlatformImageView = NSImageView

typealias PlatformColor = NSColor
typealias PlatformFont = NSFont
typealias PlatformRect = NSRect
typealias PlatformBezierPath = NSBezierPath
typealias PlatformImage = NSImage

typealias PlatformGestureRecognizer = NSGestureRecognizer
typealias PlatformPinchGestureRecognizer = NSMagnificationGestureRecognizer
typealias PlatformPanGestureRecognizer = NSPanGestureRecognizer
typealias PlatformTapGestureRecognizer = NSClickGestureRecognizer
typealias PlatformGestureRecognizerDelegate = NSGestureRecognizerDelegate

typealias PlatformScreen = NSScreen

#endif
