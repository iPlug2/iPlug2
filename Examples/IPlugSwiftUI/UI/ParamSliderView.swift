import SwiftUI
#if os(macOS)
import AppKit
#endif

struct ParamSliderView: View {
  @EnvironmentObject var state: IPlugSwiftUIState
  @ObservedObject var param: Param
  var trackColor: Color = Color.white.opacity(0.15)
  var fillColor: Color = Color.white.opacity(0.05)
  var frameColor: Color = Color.white
  @State var hover = false
  @State var normalizedValue = 0.0

  var body: some View {
    VStack(alignment: .center, spacing: 4) { // New outer vertical stack
      
      HStack(alignment: .top, spacing: 4) {

        // Left column: max at the top, min at the bottom
        // VStack {
        //   Text("1.0")
        //     .font(.caption2)
        //     .foregroundColor(.white)
        //   Spacer()
        //   Text("0.0")
        //     .font(.caption2)
        //     .foregroundColor(.white)
        // }
        // Slider and the current value beneath it
        VStack(spacing: 4) {
          // Parameter name above the slider
          Text(param.name)
            .foregroundColor(.white)
            .font(.caption)
          GeometryReader { geometry in
            ZStack {
              Rectangle()
                .fill(trackColor)
              Rectangle()
                .fill(fillColor)
                .scaleEffect(x: 1.0, y: CGFloat(param.value), anchor: .bottomLeading)
            }
            .overlay(
              Rectangle().stroke(frameColor, lineWidth: 1)
            )
            .onSlider(
              value: $param.value,
              viewRect: geometry.frame(in: .local),
              beginEdit: { state.beginEdit(param.id) },
              doEdit: { state.doEdit(param.id, param.value) },
              endEdit: { state.endEdit(param.id) },
              onHover: { over in hover = over }
            )
          }
          // The current slider value in a tiny white font
          Text(param.getDisplay(value: param.value, normalized: true, withDisplayText: true))
            .foregroundColor(.white)
            .font(.caption2)
        }
      }
    }
  }
}

struct SliderViewModifier: ViewModifier {
  @Binding var value: Double
  let viewRect: CGRect
  let beginEdit: () -> Void
  let doEdit: () -> Void
  let endEdit: () -> Void
  let onHover: (Bool) -> Void

  @State private var hasBegun = false
  @State private var hasEnded = false
  @State private var lastLocation: CGPoint = CGPoint.zero
  
  func snapPosition(x: CGFloat, y: CGFloat) -> Double {
    let y = min(max(y, 0), viewRect.size.height)
    return Double(1.0 - (y) / viewRect.size.height)
  }
  
  func body(content: Content) -> some View {
    content.onHover { over in
      if !hasBegun {
        self.onHover(over)
      }
    }
    content.gesture(DragGesture(minimumDistance: 0)
      .onChanged { event in
        guard !self.hasEnded else { return }
        guard lastLocation != CGPoint.zero else {
          lastLocation = event.location
          #if os(macOS)
          NSCursor.hide()
          #endif
          self.beginEdit()
          self.value = snapPosition(x: event.location.x, y: event.location.y)
          self.doEdit()
          return
        }
        
        if self.hasBegun == false {
          self.hasBegun = true
        }
        else {
          self.value = snapPosition(x: event.location.x, y: event.location.y)
          self.doEdit()
        }
      }
      .onEnded { event in
        if !self.hasEnded {
          self.endEdit()
          self.onHover(false)
          #if os(macOS)
          NSCursor.unhide()
          #endif
        }
        self.hasBegun = false
        self.hasEnded = false
        lastLocation = CGPoint.zero
      })
  }
}

extension View {
  func onSlider(value: Binding<Double>,
                viewRect: CGRect,
                beginEdit: @escaping () -> Void = {},
                doEdit: @escaping () -> Void = {},
                endEdit: @escaping () -> Void = {},
                onHover: @escaping (Bool) -> Void = {_ in }
  ) -> some View {
    modifier(SliderViewModifier(value: value, viewRect: viewRect, beginEdit: beginEdit, doEdit: doEdit, endEdit: endEdit, onHover: onHover))
  }
}

struct ParamSliderView_Previews: PreviewProvider {
  @State static var dummyParam = Param(id: 0, name: "Volume", defaultValue: 0.5)
  static var previews: some View {
    ParamSliderView(param: dummyParam)
      .frame(width: 100, height: 300, alignment: .center)
      .padding()
  }
}
