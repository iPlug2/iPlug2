public typealias MIDINoteNumber = UInt8
public typealias MIDIVelocity = UInt8

func floatValue(data: Data) -> Float {
  return Float(bitPattern: UInt32(littleEndian: data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt32.self) }))
}

class IPlugCocoaUIViewController: IPlugCocoaViewController {
  
  @IBOutlet var Slider: PlatformSlider!
  @IBOutlet weak var MeterView: PlatformProgressView!
  
  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
  }
  
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
  }
  
  //if you want to load a view programmatically, without IB, you need to uncomment this on macOS
//  #if os(macOS)
//  override func loadView() {
//    view = NSView()
//    view.wantsLayer = true
//    view.layer?.backgroundColor = PlatformColor.green.cgColor
//  }
//  #endif

  override func sendControlMsgFromDelegate(ctrlTag: Int, msgTag: Int, msg: Data!) {
    if msgTag==kUpdateMessage {
      let db = 20.0 * log10(floatValue(data: msg) + 0.0001);
      let val = ((db+80.0)/80.0); // dirty linear to log
      
#if os(iOS)
      MeterView.setProgress(val, animated: false)
#else
      MeterView.doubleValue = Double(val)
#endif
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
      if let slider = self.view.viewWithTag(kCtrlTagVolumeSlider) as? PlatformSlider {
        #if os(iOS)
        slider.value = Float(value)
        #else
        slider.doubleValue = value
        #endif
      }
    }
  }
  
  @IBAction func editBegan(_ sender: PlatformControl) {
    if(sender.tag == kCtrlTagVolumeSlider) {
      beginInformHostOfParamChangeFromUI(paramIdx: kParamGain)
    }
  }
  
  @IBAction func editEnded(_ sender: PlatformControl) {
    if(sender.tag == kCtrlTagVolumeSlider) {
      endInformHostOfParamChangeFromUI(paramIdx:  kParamGain)
    }
  }
  
  @IBAction func sliderChanged(_ sender: PlatformSlider) {
    if(sender.tag == kCtrlTagVolumeSlider) {
#if os(iOS)
      sendParameterValueFromUI(paramIdx: kParamGain, normalizedValue: Double(sender.value))
#else
      sendParameterValueFromUI(paramIdx: kParamGain, normalizedValue: sender.doubleValue)
#endif
    }
  }

  @IBAction func buttonClicked(_ sender: PlatformButton) {
    if(sender.tag == kCtrlTagButton) {
      sendArbitraryMsgFromUI(msgTag: kMsgTagHello, ctrlTag: kCtrlTagButton, msg:nil);
    }
  }
}
