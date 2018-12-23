import UIKit
import AudioToolbox
import IPlugInstrumentFramework

// thanks audiokit
public func fourCC(_ string: String) -> UInt32 {
  let utf8 = string.utf8
  precondition(utf8.count == 4, "Must be a 4 char string")
  var out: UInt32 = 0
  for char in utf8 {
    out <<= 8
    out |= UInt32(char)
  }
  return out
}

class IPlugInstrumentAppViewController: UIViewController {
  @IBOutlet weak var playButton: UIButton!
  @IBOutlet weak var auContainerView: UIView!
  
  var playEngine: SimplePlayEngine!
  var mAudioUnitViewController: IPlugInstrumentViewController!
  
  override func viewDidLoad() {
    super.viewDidLoad()

    embedPlugInView()
    
    playEngine = SimplePlayEngine(componentType: kAudioUnitType_MusicDevice)

    var desc = AudioComponentDescription()
    desc.componentType = kAudioUnitType_MusicDevice
    desc.componentSubType = fourCC("Ipef") //TODO: hardcoded!
    desc.componentManufacturer = fourCC("Acme") //TODO: hardcoded!
    desc.componentFlags = 0
    desc.componentFlagsMask = 0

    AUAudioUnit.registerSubclass(IPlugAUAudioUnit.self, as: desc, name:"iPlug: Local IPlugInstrument", version: UInt32.max)

    playEngine.selectAudioUnitWithComponentDescription(desc) {
      self.mAudioUnitViewController.audioUnit = self.playEngine.testAudioUnit as? IPlugAUAudioUnit
    }
  }

  func embedPlugInView() {
    let builtInPlugInsURL = Bundle.main.builtInPlugInsURL!
    let pluginURL = builtInPlugInsURL.appendingPathComponent("IPlugInstrumentAppExtension.appex")
    let appExtensionBundle = Bundle(url: pluginURL)
    
    let storyboard = UIStoryboard(name: "IPlugInstrument-iOS-MainInterface", bundle: appExtensionBundle)
    mAudioUnitViewController = storyboard.instantiateInitialViewController() as? IPlugInstrumentViewController

    if let view = mAudioUnitViewController.view {
      addChild(mAudioUnitViewController)
      view.frame = auContainerView.bounds

      auContainerView.addSubview(view)
      mAudioUnitViewController.didMove(toParent: self)
    }
  }

  @IBAction func togglePlay(_ sender: AnyObject?) {
    let isPlaying = playEngine.togglePlay()
    let titleText = isPlaying ? "Stop" : "Play"
    playButton.setTitle(titleText, for: UIControl.State())
  }
}
