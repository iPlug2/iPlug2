import SwiftUI
import Accelerate

struct ContentView: View {
  @EnvironmentObject var state: IPlugSwiftUIState
  var body: some View {
    ZStack {
      MeshGradient(width: 2, height: 2, points: [
          [0, 0], [1, 0], [0, 1], [1, 1]
      ], colors: [.red, .orange, .blue, .yellow])
      VStack {
        Text("iPlug2 + SwiftUI + Metal")
          .font(.title)
          .padding()
        HStack(alignment: .center) {
//          ParamSliderView(param: state.params[kParamGain])
//            .frame(width: 50, height: 200, alignment: .center)
          OscilloscopeView()
              .environmentObject(state)
        }
//        Spacer()
//        Button(action: {
//          state.sendMsg()
//        },
//        label: {
//          Text("Send a message to C++")
//            .foregroundStyle(.black)
//        })
      }
      .padding()
    }
    .edgesIgnoringSafeArea(.all)
  }
}

struct SwiftUIView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      ContentView().environmentObject(IPlugSwiftUIState(numParams: kNumParams))
        .frame(width: 300, height: 400, alignment: .center)
    }
  }
}
