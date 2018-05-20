import UIKit
import AudioToolbox
import IPlugEffectFramework

func fourCharCodeFrom(string : String) -> FourCharCode
{
  assert(string.characters.count == 4, "String length must be 4")
  var result : FourCharCode = 0
  for char in string.utf16 {
    result = (result << 8) + FourCharCode(char)
  }
  return result
}

class IPlugEffectAppViewController: UIViewController {
  @IBOutlet weak var playButton: UIButton!
  @IBOutlet weak var auContainerView: UIView!
  
  var playEngine: SimplePlayEngine!
  var mAudioUnitViewController: IPlugEffectViewController!
  
  override func viewDidLoad() {
    super.viewDidLoad()

    embedPlugInView()
    
    playEngine = SimplePlayEngine(componentType: kAudioUnitType_Effect)

    var desc = AudioComponentDescription()
    desc.componentType = kAudioUnitType_Effect
    desc.componentSubType = fourCharCodeFrom(string: "Ipef") //TODO: hardcoded!
    desc.componentManufacturer = fourCharCodeFrom(string: "Acme") //TODO: hardcoded!
    desc.componentFlags = 0
    desc.componentFlagsMask = 0

    AUAudioUnit.registerSubclass(IPlugAUAudioUnit.self, as: desc, name:"iPlug: Local IPlugEffect", version: UInt32.max)

    playEngine.selectAudioUnitWithComponentDescription(desc) {
      let audioUnit = self.playEngine.testAudioUnit as! IPlugAUAudioUnit
      self.mAudioUnitViewController.audioUnit = audioUnit
    }
  }

  func embedPlugInView() {
    let builtInPlugInsURL = Bundle.main.builtInPlugInsURL!
    let pluginURL = builtInPlugInsURL.appendingPathComponent("IPlugEffectAppExtension.appex")
    let appExtensionBundle = Bundle(url: pluginURL)
    
    let storyboard = UIStoryboard(name: "IPlugEffect-iOS-MainInterface", bundle: appExtensionBundle)
    mAudioUnitViewController = storyboard.instantiateInitialViewController() as! IPlugEffectViewController

    if let view = mAudioUnitViewController.view {
      addChildViewController(mAudioUnitViewController)
      view.frame = auContainerView.bounds

      auContainerView.addSubview(view)
      mAudioUnitViewController.didMove(toParentViewController: self)
    }
  }

  @IBAction func togglePlay(_ sender: AnyObject?) {
    let isPlaying = playEngine.togglePlay()
    let titleText = isPlaying ? "Stop" : "Play"
    playButton.setTitle(titleText, for: UIControlState())
  }
}
