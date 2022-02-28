#if os(iOS)
import UIKit
import SwiftUI
public typealias PlatformView = UIView
public typealias PlatformColor = UIColor
public typealias PlatformRect = CGRect
public typealias PlatformHostingController = UIHostingController
public typealias PlatformNibName = String
#elseif os(macOS)
import AppKit
import SwiftUI
public typealias PlatformView = NSView
public typealias PlatformColor = NSColor
public typealias PlatformRect = NSRect
public typealias PlatformHostingController = NSHostingController
public typealias PlatformHostingView = NSHostingView
public typealias PlatformNibName = NSNib.Name
#endif

public extension PlatformView {
  func pinToSuperviewEdges() {
    guard let superview = superview else { return }
    translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      topAnchor.constraint(equalTo: superview.topAnchor),
      leadingAnchor.constraint(equalTo: superview.leadingAnchor),
      bottomAnchor.constraint(equalTo: superview.bottomAnchor),
      trailingAnchor.constraint(equalTo: superview.trailingAnchor)
    ])
  }
}
