import SwiftUI

struct OscilloscopeView: View {
    @EnvironmentObject var state: IPlugSwiftUIState
    @State private var startDate = Date()
    
    var labels: [GridLabel] = [
        GridLabel(text: "Amplitude", position: .left),
        GridLabel(text: "Time", position: .bottom)
    ]
  
    var body: some View {
        VStack {
            GeometryReader { geo in
                ZStack {
                  let bundle = BundleLocator.shared.getBundle()!

                    TimelineView(.animation) { _ in
                        shaderOnGrid
                      #if os(macOS)
                            .blur(radius: 1.2)
                            .layerEffect(
                              ShaderLibrary.bundle(bundle)[dynamicMember: "crteffect"](
                                .float(-startDate.timeIntervalSinceNow),
                                .float2(geo.size)
                              ),
                              maxSampleOffset: .zero
                            )
                      #endif
                    }
                  
                  GridLabelsView(labels: labels)
                    .padding(40)
                }
            }

            Spacer()
        }
        .background(
            RadialGradient(stops: [
                .init(color: .white.opacity(0.2), location: 0),
                .init(color: .black, location: 1)
            ], center: .center, startRadius: 10, endRadius: 1000)
        )
    }
      
    var shaderOnGrid: some View {
        ZStack {
            TimelineView(.animation) { tl in
                let bundle = BundleLocator.shared.getBundle()!
                let waveform = state.waveform;

                Rectangle()
                    .fill(.white)
                    .visualEffect { content, proxy in
                        let shader = ShaderLibrary.bundle(bundle)[dynamicMember: "oscilloscope"](
                            .float2(proxy.size),
                            .floatArray(waveform)
                        )
                        return content.colorEffect(shader)
                    }
            }
            .padding(40)
          
            GridLinesView(rows: 8, columns: 4, showOuterLines: false, opacity: 0.5)
              .padding(40)
        }
        .background(Color.black)
    }
}

#Preview {
  OscilloscopeView().environmentObject(IPlugSwiftUIState())
}
