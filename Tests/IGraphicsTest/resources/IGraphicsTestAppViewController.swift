import UIKit
import AudioToolbox
import IGraphicsTestFramework

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

class IGraphicsTestAppViewController: UIViewController {
  @IBOutlet weak var playButton: UIButton!
  @IBOutlet weak var auContainerView: UIView!
  
  var playEngine: SimplePlayEngine!
  var mAudioUnitViewController: IGraphicsTestViewController!
  
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

    AUAudioUnit.registerSubclass(IPlugAUAudioUnit.self, as: desc, name:"iPlug: Local IGraphicsTest", version: UInt32.max)

    playEngine.selectAudioUnitWithComponentDescription(desc) {
      self.mAudioUnitViewController.audioUnit = self.playEngine.testAudioUnit as? IPlugAUAudioUnit
    }
  }

  func embedPlugInView() {
    let builtInPlugInsURL = Bundle.main.builtInPlugInsURL!
    let pluginURL = builtInPlugInsURL.appendingPathComponent("IGraphicsTestAppExtension.appex")
    let appExtensionBundle = Bundle(url: pluginURL)
    
    let storyboard = UIStoryboard(name: "IGraphicsTest-iOS-MainInterface", bundle: appExtensionBundle)
    mAudioUnitViewController = storyboard.instantiateInitialViewController() as? IGraphicsTestViewController

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
