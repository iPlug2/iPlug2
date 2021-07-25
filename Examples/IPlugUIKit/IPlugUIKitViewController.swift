public typealias MIDINoteNumber = UInt8
public typealias MIDIVelocity = UInt8

func floatValue(data: Data) -> Float {
  return Float(bitPattern: UInt32(littleEndian: data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt32.self) }))
}

class IPlugUIKitViewController: IPlugCocoaViewController {
  
  @IBOutlet var Sliders: [UISlider]!
  @IBOutlet weak var MeterView: UIProgressView!
  
  override func viewDidLoad() {
    super.viewDidLoad()
  }

  override func sendControlMsgFromDelegate(ctrlTag: Int, msgTag: Int, msg: Data!) {
    if msgTag==kUpdateMessage {
      let db = 20.0 * log10(floatValue(data: msg) + 0.0001);
      let val = ((db+80.0)/80.0); // dirty linear to log
      
      MeterView.setProgress(val, animated: false)
    }
  }
  
  override func onMidiMsgUI(_ status: UInt8, _ data1: UInt8, _ data2: UInt8, _ offset: Int) {
    print("Midi message received in UI: status: \(status), data1: \(data1), data2: \(data2)")
  }
  
  override func onSysexMsgUI(_ msg: Data!, _ offset: Int) {
    print("Sysex message received")
  }
  
  override func onParamChangeUI(_ paramIdx: Int, _ value: Double) {
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
