// Copyright AudioKit. All Rights Reserved. Revision History at http://github.com/AudioKit/AudioKitUI/

import SwiftUI

struct KnobDefaults {
    static let knobPreferredWidth: CGFloat = 60
    static let knobBgColor = Color(hue: 0.1, saturation: 1.0, brightness: 1.0, opacity: 1.0)
    static let knobBgCornerRadius: CGFloat = 1.0 * 0.25 * knobPreferredWidth
    static let knobLineCap: CGLineCap = .round
    static let knobCircleWidth = 0.7 * knobPreferredWidth
    static let knobStrokeWidth = 2 * knobPreferredWidth / 25
    static let knobRotationRange: CGFloat = 0.875
    static let knobTrimMin: CGFloat = (1 - knobRotationRange) / 2.0
    static let knobTrimMax: CGFloat = 1 - knobTrimMin
    static let knobDragSensitivity: CGFloat = 0.005
}

public struct Knob: View {

    @Binding var value: Double
    var range: ClosedRange<Double>
    var title: String = ""
    @State var displayString: String = ""

    public init(value: Binding<Double>, range: ClosedRange<Double>, title: String) {
        self._value = value
        self.range = range
        self.title = title
        self.displayString = title
    }

    var normalizedValue: Double {
        min(1.0, max(0.0, (value - range.lowerBound) / (range.upperBound - range.lowerBound)))
    }

    @State private var lastLocation: CGPoint = CGPoint(x: 0, y: 0)
    private var dragGesture: some Gesture {
        DragGesture()
            .onChanged { dragPoint in
                guard lastLocation != CGPoint.zero else {
                    lastLocation = dragPoint.location
                    return
                }
                var change = Double((dragPoint.location.x - lastLocation.x) * KnobDefaults.knobDragSensitivity)
                change -= Double((dragPoint.location.y - lastLocation.y) * KnobDefaults.knobDragSensitivity)
                value = value +  change * ((range.upperBound - range.lowerBound) + range.lowerBound)
                value = min(range.upperBound, max(range.lowerBound, value))
                lastLocation = dragPoint.location
                displayString = String(format: "%0.4f", value)
            }
            .onEnded { _ in
                DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                    displayString = title
                }
                lastLocation = CGPoint.zero
            }
    }

    public var body: some View {
        let trim = KnobDefaults.knobTrimMin + CGFloat(normalizedValue) * (KnobDefaults.knobTrimMax - KnobDefaults.knobTrimMin)
        GeometryReader { geometry in
            VStack {
                ZStack(alignment: .center) {

                    // Background
                    RoundedRectangle(cornerRadius: KnobDefaults.knobBgCornerRadius)
                        .fill(KnobDefaults.knobBgColor)
                        .frame(width: KnobDefaults.knobPreferredWidth, height: KnobDefaults.knobPreferredWidth)

                    // Stroke entire trim of knob
                    Circle()
                        .trim(from: KnobDefaults.knobTrimMin, to: KnobDefaults.knobTrimMax)
                        .rotation(.degrees(-270))
                        .stroke(Color.black ,style: StrokeStyle(lineWidth: KnobDefaults.knobStrokeWidth, lineCap: KnobDefaults.knobLineCap))
                        .frame(width: KnobDefaults.knobCircleWidth, height: KnobDefaults.knobCircleWidth)

                    // Stroke value trim of knob
                    Circle()
                        .trim(from: KnobDefaults.knobTrimMin, to: trim)
                        .rotation(.degrees(-270))
                        .stroke(Color.white ,style: StrokeStyle(lineWidth: KnobDefaults.knobStrokeWidth + 1, lineCap: KnobDefaults.knobLineCap))
                        .frame(width: KnobDefaults.knobCircleWidth, height: KnobDefaults.knobCircleWidth)
                }
                .gesture(dragGesture)

                // Title of Knob
                Text(displayString == "" ? title : displayString)
            }
            .frame(width: geometry.size.width, height: geometry.size.height, alignment: .center)
        }
    }
}
