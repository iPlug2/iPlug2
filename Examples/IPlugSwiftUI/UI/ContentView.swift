import SwiftUI

struct ContentView: View {
  @EnvironmentObject var state: IPlugSwiftUIState
  var body: some View {
    HStack {
      ParamSliderView(param: state.params[kParamGain])
      ParamSliderView(param: state.params[kParamFreq])
      VUSwiftUIView(onRender: {
        let db = 20.0 * log10(state.lastVolume + 0.0001);
        return ((db+80.0)/80.0); // dirty linear to log
      })
        .frame(width: 20, height: .none, alignment: .leading)
      Button(action: {
        state.sendMsg()
      },
      label: {
        Text("Send a message")
      })
    }
    .padding(10.0)
//    .frame(width: 100, height: 200, alignment: /*@START_MENU_TOKEN@*/.center/*@END_MENU_TOKEN@*/)
  }
}

struct SwiftUIView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      ContentView()
    }
  }
}
