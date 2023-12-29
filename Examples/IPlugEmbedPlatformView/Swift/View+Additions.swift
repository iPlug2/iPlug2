//
//  View+Additions.swift
//  Periphony: Spatial Audio Player
//  Copyright © Oli Larkin Plug-ins Ltd. 2022. All rights reserved.
//  Developed by Kartik Venugopal
//

#if os(macOS)
import Cocoa
#elseif os(iOS)
import UIKit
#endif

extension PlatformView {
    
    // MARK: Platform-agnostic extensions
    
    ///
    /// Pins / anchors this view to all 4 edges of its superview, so that
    /// this view effectively has the same size as its superview whenever
    /// resized.
    ///
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
    
    ///
    /// Pins / anchors this view to the specified edges of the given view.
    ///
    func pinToEdges(_ edges: ViewEdges, ofView view: PlatformView) {
        
        translatesAutoresizingMaskIntoConstraints = false
        
        var constraints: [NSLayoutConstraint] = []
        
        if edges.contains(.leading) {
            constraints.append(leadingAnchor.constraint(equalTo: view.leadingAnchor))
        }
        
        if edges.contains(.trailing) {
            constraints.append(trailingAnchor.constraint(equalTo: view.trailingAnchor))
        }
        
        if edges.contains(.bottom) {
            constraints.append(bottomAnchor.constraint(equalTo: view.bottomAnchor))
        }
        
        if edges.contains(.top) {
            constraints.append(topAnchor.constraint(equalTo: view.topAnchor))
        }
        
        NSLayoutConstraint.activate(constraints)
    }
    
    ///
    /// Convenience function to trigger a redrawing of this view and also
    /// invalidate its layout, forcing a fresh layout.
    ///
    func setNeedsDisplayAndLayout() {
        
        setNeedsDisplay()
        setNeedsLayout()
    }
    
    ///
    /// Removes all sublayers from the root layer of this view.
    ///
    /// This is typically done immediately prior to a fresh view redraw
    /// that will add fresh sublayers.
    ///
    func removeAllSublayers() {

        #if os(macOS)
        layer?.sublayers?.removeAll()
        #elseif os(iOS)
        layer.sublayers?.removeAll()
        #endif
    }

    ///
    /// A convenience function to add the given sublayer to this view's root layer
    /// in a platform-agnostic way without requiring platform checks.
    ///
    func addSublayer(_ sublayer: CALayer) {

        #if os(macOS)
        layer?.addSublayer(sublayer)
        #elseif os(iOS)
        layer.addSublayer(sublayer)
        #endif
    }
    
    func constraints(forSubView subView: PlatformView, attributes: [NSLayoutConstraint.Attribute]) -> [NSLayoutConstraint] {
        
        constraints.filter {
            
            let firstItemMatch: Bool = $0.firstItem === subView && attributes.contains($0.firstAttribute)
            let secondItemMatch: Bool = $0.secondItem === subView && attributes.contains($0.secondAttribute)
            return firstItemMatch || secondItemMatch
        }
    }
    
    func constraints(forSubView subView: PlatformView) -> [NSLayoutConstraint] {
        constraints.filter {$0.firstItem === subView || $0.secondItem === subView}
    }
    
    // ------------------------------------------------------------------------------
    
    // MARK: macOS extensions
    
    #if os(macOS)
    
    /// Computes the frame of this view relative to its farthest ancestor (i.e. window contentView).
    var frameWithinWindow: NSRect {
        
        var parent = self.superview
        var rootAncestor: NSView = self
        
        while parent != nil {
            
            parent = parent?.superview
            
            if let ancestor = parent?.superview {
                rootAncestor = ancestor
            }
        }
        
        // The returned frame will be of the same size but its origin will
        // be relative to the containing window.
        
        let origin = convert(NSPoint.zero, to: rootAncestor)
        return NSRect(origin: origin, size: frame.size)
    }
    
    ///
    /// Convenience function to trigger a redraw of this view, matching the
    /// iOS function ``setNeedsDisplay()``.
    ///
    /// This function allows us to trigger view redraws in a platform-agnostic
    /// manner without requiring platform checks.
    ///
    func setNeedsDisplay() {
        needsDisplay = true
    }
    
    ///
    /// Convenience function to trigger a fresh layout of this view, matching the
    /// iOS function ``setNeedsLayout()``.
    ///
    /// This function allows us to trigger view layout in a platform-agnostic
    /// manner without requiring platform checks.
    ///
    func setNeedsLayout() {
        needsLayout = true
    }
    
    ///
    /// Brings this view to the front, relative to all its sibling views.
    ///
    func bringToFront() {
        
        let superView = self.superview
        self.removeFromSuperview()
        superView?.addSubview(self, positioned: .above, relativeTo: nil)
    }
    
    #elseif os(iOS)
    
    ///
    /// Brings this view to the front, relative to all its sibling views.
    ///
    func bringToFront() {
      superview?.bringSubviewToFront(self)
    }
    
    #endif
    
    ///
    /// Brings this view to the front, relative to all its sibling views, and
    /// re-activates all constraints that were removed by bringing this
    /// view to the front, i.e. it ensures that no constraints are lost in
    /// the process.
    ///
    func bringToFrontAndReactivateConstraints() {
        
        guard let superView = superview else {return}
        
        let myConstraints = superView.constraints.filter({$0.firstItem === self || $0.secondItem === self})
        
        bringToFront()
        
        superView.addConstraints(myConstraints)
        NSLayoutConstraint.activate(myConstraints)
    }
}

// ------------------------------------------------------------------------------

// MARK: Auxiliary types

///
/// An ``OptionSet`` to conveniently represent any combination of the 4 edges of a view.
///
struct ViewEdges: OptionSet {
    
    let rawValue: Int
    
    static let leading = ViewEdges(rawValue: 1 << 0)
    static let trailing = ViewEdges(rawValue: 1 << 1)
    static let bottom = ViewEdges(rawValue: 1 << 2)
    static let top = ViewEdges(rawValue: 1 << 3)
    
    /// A convenience constant representing all 4 edges.
    static let all: ViewEdges = [leading, trailing, bottom, top]
}
