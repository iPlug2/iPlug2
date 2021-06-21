import SwiftUI

class Param : ObservableObject {
  let id: Int
  var name: String
  @Published var value: Double/*, minVal: Double, maxVal: Double, step: Double, label: String, flags: Int, group: String*/
  
  init(id: Int, name: String, value: Double) {
    self.id = id
    self.name = name
    self.value = value
  }
}

class IPlugSwiftUIState: ObservableObject {
  var params: [Param] = []
  var lastVolume = Float(0.0)

  let beginEdit: (Int) -> Void
  let doEdit: (Int, Double) -> Void
  let endEdit: (Int) -> Void
  let sendMsg: () -> Void

  public init(beginEdit: @escaping (Int) -> Void,
                 doEdit: @escaping (Int, Double) -> Void,
                endEdit: @escaping (Int) -> Void,
                sendMsg: @escaping () -> Void) {
    self.beginEdit = beginEdit;
    self.doEdit = doEdit;
    self.endEdit = endEdit;
    self.sendMsg = sendMsg
  }
}
