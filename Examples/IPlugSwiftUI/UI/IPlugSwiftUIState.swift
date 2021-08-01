import SwiftUI

class Param : ObservableObject {
  let id: Int
  var name: String
  var defaultValue: Double
  var step: Double
  var minValue: Double
  var maxValue: Double
  var label: String
  var group: String

  @Published var value: Double
  
  init(id: Int,
       name: String = "",
       defaultValue: Double = 0.0,
       minValue: Double = 0.0,
       maxValue: Double = 1.0,
       step: Double = 0.1,
       label: String = "",
       group: String = ""
  ) {
    self.id = id
    self.name = name
    self.defaultValue = defaultValue
    self.value = defaultValue
    self.minValue = minValue
    self.maxValue = maxValue
    self.step = step
    self.label = label
    self.group = group
  }
}

class IPlugSwiftUIState: ObservableObject {
  var params: [Param] = []
  var lastVolume = Float(0.0)

  let beginEdit: (Int) -> Void
  let doEdit: (Int, Double) -> Void
  let endEdit: (Int) -> Void
  let sendMsg: () -> Void

  public init(beginEdit: @escaping (Int) -> Void = {_ in },
              doEdit: @escaping (Int, Double) -> Void  = {_,_ in },
              endEdit: @escaping (Int) -> Void  = {_ in },
                sendMsg: @escaping () -> Void  = {}) {
    self.beginEdit = beginEdit;
    self.doEdit = doEdit;
    self.endEdit = endEdit;
    self.sendMsg = sendMsg
  }
  
  convenience init(numParams: Int) {
    self.init()
    for paramIdx in 0...numParams {
      params.append(Param(id: paramIdx))
    }
  }
}
