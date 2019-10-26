class IPlugSwiftViewController: IPlugCocoaViewController {
  @IBOutlet var KeyboardView: KeyboardView!
  @IBOutlet var Sliders: [UISlider]!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    KeyboardView?.delegate = self
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
      beginInformHostOfParamChangeFromUI(paramIdx: kParamGain)
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

extension IPlugSwiftViewController: AKKeyboardDelegate {
  
  public func noteOn(note: MIDINoteNumber, velocity: MIDIVelocity = 127) {
    guard note < 128 else { return }
    sendMidiMsgFromUI(status: 0x90, data1: note, data2: velocity, offset: 0);
  }
  
  public func noteOff(note: MIDINoteNumber) {
    guard note < 128 else { return }
    sendMidiMsgFromUI(status: 0x80, data1: note, data2: 0, offset: 0);
  }
}
