import SwiftUI

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
//    if(direction == EDirection::Vertical)
    let y = min(max(y, 0), viewRect.size.height)
    return Double(1.0 - (y/*-top*/) / viewRect.size.height);
//    else
//    val = (x-bounds.L) / bounds.W();
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
        self.beginEdit()
        self.value = snapPosition(x: event.location.x, y: event.location.y)
        self.doEdit()
        return
      }
      
      if self.hasBegun == false {
        self.hasBegun = true
      }
      else {
    //      var deltaX = Double((event.location.x - lastLocation.x) * KnobDefaults.knobDragSensitivity)
    //      var deltaY = Double((event.location.y - lastLocation.y) * KnobDefaults.knobDragSensitivity)
        self.value = snapPosition(x: event.location.x, y: event.location.y)
        self.doEdit()
      }
    }
    .onEnded { event in
      if !self.hasEnded {
      self.endEdit()
      self.onHover(false)
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

extension Color {
    static var random: Color {
        return Color(
            red: .random(in: 0...1),
            green: .random(in: 0...1),
            blue: .random(in: 0...1)
        )
    }
}

struct ParamSliderView: View {
  @EnvironmentObject var state: IPlugSwiftUIState
  @ObservedObject var param: Param
  @State var hover = false;

  var body: some View {
  VStack {
//    Text(param.name)
    GeometryReader { geometry in
    ZStack {
      Rectangle()
      .fill(Color.white)
      Rectangle()
      .fill(Color.black)
      .scaleEffect(x: 1.0, y: CGFloat(param.value), anchor: .bottomLeading)
    }
    .onSlider(
      value: $param.value,
      viewRect: geometry.frame(in: .local),
      beginEdit: { state.beginEdit(param.id) },
      doEdit: { state.doEdit(param.id, param.value) },
      endEdit: { state.endEdit(param.id) },
      onHover: { over in hover = over }
    )
    }
  }
  }
}

struct ParamSliderView_Previews: PreviewProvider {
  @State static var dummyParam = Param(id: 0, name: "Volume", defaultValue: 0.5);
  static var previews: some View {
  ParamSliderView(param: dummyParam)
    .frame(width: 100, height: 300, alignment: .center)
    .padding()
  }
}
