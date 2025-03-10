import SwiftUI

class IPlugSwiftUIState: NSObject, ObservableObject {
  var params: [Param] = []
  @Published var bundleID = String("")
  @Published @objc dynamic var waveform: [Float] = Array(repeating: 0.0, count: kScopeBufferSize)

  let beginEdit: @MainActor (Int) -> Void
  let doEdit: @MainActor (Int, Double) -> Void
  let endEdit: @MainActor (Int) -> Void
  let sendMsg: @MainActor () -> Void

  public override init() {
    self.beginEdit = {_ in }
    self.doEdit = {_,_ in }
    self.endEdit = {_ in }
    self.sendMsg = {}
    super.init()
  }

  public init(beginEdit: @MainActor @escaping (Int) -> Void = {_ in },
              doEdit: @MainActor @escaping (Int, Double) -> Void  = {_,_ in },
              endEdit: @MainActor @escaping (Int) -> Void  = {_ in },
              sendMsg: @MainActor @escaping () -> Void  = {}) {
    self.beginEdit = beginEdit
    self.doEdit = doEdit
    self.endEdit = endEdit
    self.sendMsg = sendMsg
    super.init()
  }
  
  convenience init(numParams: Int) {
    self.init()
    for paramIdx in 0...numParams {
      params.append(Param(id: paramIdx))
    }
  }

}
