import AudioKitUI

public typealias MIDINoteNumber = UInt8
public typealias MIDIVelocity = UInt8


class IPlugSwiftViewController: IPlugCocoaViewController, AudioKitUI.AKKeyboardDelegate {
  
  @IBOutlet var KeyboardView: AudioKitUI.AKKeyboardView!
  @IBOutlet var Sliders: [UISlider]!
  @IBOutlet var Plot: AKNodeOutputPlot!
  @IBOutlet var PresetLoader: AKPresetLoaderView!
  var presetsInitialized = false
  
  func presetSelected(_ preset:String) {
    if let idx = PresetLoader.presets.firstIndex(of: preset) {
      sendArbitraryMsgFromUI(msgTag: kMsgTagRestorePreset, ctrlTag: Int32(idx), msg: nil)
    }
  }
  
  func initializePresetList() {
    if let presets = getAudioUnit()?.factoryPresets {
      var presetNames : [String] = []
      for preset in presets {
        presetNames.append(preset.name)
      }
      
      PresetLoader.presets = presetNames
      PresetLoader.callback = presetSelected
      presetsInitialized = true
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    KeyboardView.delegate = self
    
    if (getAudioUnit() != nil && !presetsInitialized) {
      initializePresetList()
    }
  }
  
  override func audioUnitInitialized() {
    
    if (presetsInitialized && PresetLoader != nil) {
      initializePresetList()
    }
  }
  
  override func onMessage(_ msgTag: Int32, _ ctrlTag: Int32, _ msg: Data!) -> Bool {
    if(msgTag == kMsgTagData)
    {
      msg.withUnsafeBytes { (outputBytes: UnsafeRawBufferPointer) in
        let floatData = outputBytes.bindMemory(to: Float.self)
        let mutablePtr = UnsafeMutablePointer<Float>.init(mutating: floatData.baseAddress)
        Plot.updateBuffer(mutablePtr, withBufferSize: kDataPacketSize)
      }

      return true
    }
    
    return false
  }
  
  override func onMidiMsgUI(_ status: UInt8, _ data1: UInt8, _ data2: UInt8, _ offset: Int32) {
    print("Midi message received in UI: status: \(status), data1: \(data1), data2: \(data2)")
    
    
    KeyboardView.programmaticNoteOn(data1);
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
  
  func noteOn(note: MIDINoteNumber) {
    guard note < 128 else { return }
    sendMidiMsgFromUI(status: 0x90, data1: note, data2: 127, offset: 0);
  }
  
  public func noteOn(note: MIDINoteNumber, velocity: MIDIVelocity = 127) {
    guard note < 128 else { return }
    sendMidiMsgFromUI(status: 0x90, data1: note, data2: velocity, offset: 0);
  }
  
  public func noteOff(note: MIDINoteNumber) {
    guard note < 128 else { return }
    sendMidiMsgFromUI(status: 0x80, data1: note, data2: 0, offset: 0);
  }
}
