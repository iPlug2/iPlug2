import SwiftUI
import Accelerate
import vger      // C/C++/ObjC interface.
import vgerSwift // Swift nicities.

struct DemoView: View {

    let cyan = SIMD4<Float>(0,1,1,1)
    let magenta = SIMD4<Float>(1,0,1,1)

    func textAt(_ vger: vgerContext, _ x: Float, _ y: Float, _ string: String) {
        vgerSave(vger)
        vgerTranslate(vger, .init(x: x, y: y))
        vgerText(vger, string, cyan, 0)
        vgerRestore(vger)
    }

    func draw(vger: vgerContext, size: CGSize) {
        vgerSave(vger)

        let bezPaint = vgerLinearGradient(vger, .init(x: 50, y: 450), .init(x: 100, y: 450), cyan, magenta, 0.0)
        vgerStrokeBezier(vger, vgerBezierSegment(a: .init(x: 50, y: 450), b: .init(x: 100, y: 450), c: .init(x: 100, y: 500)), 1.0, bezPaint)
        textAt(vger, 150, 450, "Quadratic Bezier stroke")

        let rectPaint = vgerLinearGradient(vger, .init(x: 50, y: 350), .init(x: 100, y: 400), cyan, magenta, 0.0)
        vgerFillRect(vger, .init(x: 50, y: 350), .init(x: 100, y: 400), 10.0, rectPaint)
        textAt(vger, 150, 350, "Rounded rectangle")

        let circlePaint = vgerLinearGradient(vger, .init(x: 50, y: 250), .init(x: 100, y: 300), cyan, magenta, 0.0)
        vgerFillCircle(vger, .init(x: 75, y: 275), 25, circlePaint)
        textAt(vger, 150, 250, "Circle")

        let linePaint = vgerLinearGradient(vger, .init(x: 50, y: 150), .init(x: 100, y: 200), cyan, magenta, 0.0)
        vgerStrokeSegment(vger, .init(x: 50, y: 150), .init(x: 100, y: 200), 2.0, linePaint)
        textAt(vger, 150, 150, "Line segment")

        let theta: Float = 0.0 // orientation
        let aperture: Float = 0.5 * .pi

        let arcPaint = vgerLinearGradient(vger, .init(x: 50, y: 50), .init(x: 100, y: 100), cyan, magenta, 0.0)
        vgerStrokeArc(vger, .init(x: 75, y: 75), 25, 1.0, theta, aperture, arcPaint)
        textAt(vger, 150, 050, "Arc")

        vgerRestore(vger);
    }

    var body: some View {
      GeometryReader { geom in
        VgerView { vger in
          draw(vger: vger, size: geom.size)
        }
      }
    }
}

struct DemoView_Previews: PreviewProvider {
    static var previews: some View {
        DemoView()
    }
}

struct ContentView: View {
  @EnvironmentObject var state: IPlugVgerState
  let cyan = SIMD4<Float>(0,1,1,1)

  var body: some View {
    DemoView()
  }
}

struct SwiftUIView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      ContentView().environmentObject(IPlugVgerState(numParams: kNumParams))
        .frame(width: 300, height: 400, alignment: .center)
    }
  }
}
