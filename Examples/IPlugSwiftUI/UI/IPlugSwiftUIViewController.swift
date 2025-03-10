import SwiftUI

public typealias MIDINoteNumber = UInt8
public typealias MIDIVelocity = UInt8

func floatValue(data: Data) -> Float {
  return Float(bitPattern: UInt32(littleEndian: data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt32.self) }))
}


@objc class IPlugSwiftUIViewController: IPlugCocoaViewController {
  lazy var state = IPlugSwiftUIState(beginEdit: self.beginInformHostOfParamChangeFromUI,
                                        doEdit: self.sendParameterValueFromUI,
                                       endEdit: self.endInformHostOfParamChangeFromUI,
                                       sendMsg: {
                                        self.sendArbitraryMsgFromUI(msgTag: kMsgTagHello, ctrlTag: kCtrlTagButton, msg:nil);
                                       })
  
  override init(nibName nibNameOrNil: PlatformNibName?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
  }
  
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    
  }
  
  override init!(editorDelegateAndBundleID editorDelegate: UnsafeMutableRawPointer!, _ bundleID: UnsafePointer<CChar>!) {
    super.init(editorDelegateAndBundleID: editorDelegate, bundleID)
  }
  
  deinit {
  }
  
  #if os(macOS)
  override func loadView() {
    self.view = NSView()
  }
  #endif
  
  override func viewDidLoad() {
    super.viewDidLoad()
      
    for idx in 0..<parameterCount() {
      state.params.append(Param(id: idx,
                                name: getParameterName(idx),
                                defaultValue: getParameterDefault(idx),
                                minValue: getParameterMin(idx),
                                maxValue: getParameterMax(idx),
                                step: getParameterStep(idx),
                                label: getParameterLabel(idx),
                                group: getParameterGroup(idx)))
    }
    
    let contentView = ContentView().environmentObject(state)
    let hostingController = PlatformHostingController(rootView: contentView)
    state.bundleID = getBundleID()
    BundleLocator.shared.initialize(with: state.bundleID)

    view.addSubview(hostingController.view)
    hostingController.view.pinToSuperviewEdges()
  }
  
  override func onMessage(_ msgTag: Int, _ ctrlTag: Int, _ msg: Data!) -> Bool {
    return false
  }
  
  override func sendControlMsgFromDelegate(ctrlTag: Int, msgTag: Int, msg: Data!) {
    var floatArray: [Float] = []

    if (ctrlTag == kCtrlTagScope)
    {
      msg.withUnsafeBytes { (pointer: UnsafeRawBufferPointer) in
          let intSize = MemoryLayout<Int32>.size
          let byteOffset = intSize * 3
          let floatPointer = pointer.baseAddress!.advanced(by: byteOffset)
          let floats = UnsafeRawBufferPointer(start: floatPointer, count: pointer.count - byteOffset).bindMemory(to: Float.self)
          floatArray = Array(floats.prefix(kScopeBufferSize))
      }
      
      state.waveform = floatArray
    }
  }
  
  override func onMidiMsgUI(_ status: UInt8, _ data1: UInt8, _ data2: UInt8, _ offset: Int) {
    print("Midi message received in UI: status: \(status), data1: \(data1), data2: \(data2)")
  }
  
  override func onSysexMsgUI(_ msg: Data!, _ offset: Int) {
    print("Sysex message received")
  }
  
  override func onParamChangeUI(_ paramIdx: Int, _ value: Double) {
    state.params[Int(paramIdx)].value = value;
  }

}
