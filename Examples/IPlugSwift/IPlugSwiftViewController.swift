public typealias MIDINoteNumber = UInt8
public typealias MIDIVelocity = UInt8

class IPlugSwiftViewController: IPlugCocoaViewController {
  
  @IBOutlet var Sliders: [UISlider]!
  
  override func viewDidLoad() {
    super.viewDidLoad()
  }
  
  override func audioUnitInitialized() {
  }
  
  override func onMessage(_ msgTag: Int32, _ ctrlTag: Int32, _ msg: Data!) -> Bool {
    if(msgTag == kMsgTagData)
    {
      msg.withUnsafeBytes { (outputBytes: UnsafeRawBufferPointer) in
//        let floatData = outputBytes.bindMemory(to: Float.self)
//        let mutablePtr = UnsafeMutablePointer<Float>.init(mutating: floatData.baseAddress)
        // TODO: handle incoming buffer data
      }

      return true
    }
    
    return false
  }
  
  override func onMidiMsgUI(_ status: UInt8, _ data1: UInt8, _ data2: UInt8, _ offset: Int32) {
    print("Midi message received in UI: status: \(status), data1: \(data1), data2: \(data2)")
  }
  
  override func onSysexMsgUI(_ msg: Data!, _ offset: Int32) {
    print("Sysex message received")
  }
  
  override func onParamChangeUI(_ paramIdx: Int32, _ value: Double) {
    if(paramIdx == kParamGain) {
      for slider in Sliders {
        if (slider.tag == kCtrlTagVolumeSlider) {
          slider.value = Float(value)
          break
        }
      }
    }
  }
  
  @IBAction func editBegan(_ sender: UIControl) {
    if(sender.tag == kCtrlTagVolumeSlider) {
      beginInformHostOfParamChangeFromUI(paramIdx: kParamGain)
    }
  }
  
  @IBAction func editEnded(_ sender: UIControl) {
    if(sender.tag == kCtrlTagVolumeSlider) {
      endInformHostOfParamChangeFromUI(paramIdx:  kParamGain)
    }
  }
  
  @IBAction func sliderChanged(_ sender: UISlider) {
    if(sender.tag == kCtrlTagVolumeSlider) {
      sendParameterValueFromUI(paramIdx: kParamGain, normalizedValue: Double(sender.value))
    }
  }

  @IBAction func buttonClicked(_ sender: UIButton) {
    if(sender.tag == kCtrlTagButton) {
      sendArbitraryMsgFromUI(msgTag: kMsgTagHello, ctrlTag: kCtrlTagButton, msg:nil);
    }
  }
  
  deinit {
    Sliders = nil;
  }
}
