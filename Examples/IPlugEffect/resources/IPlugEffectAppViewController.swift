import UIKit
import AudioToolbox
import Foundation
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

class AppViewController: UIViewController {
  @IBOutlet var playButton: UIButton!
  @IBOutlet var auContainerView: UIView!
  var playEngine: SimplePlayEngine!
  var mPlugViewController: UIViewController!
  
  override func viewDidLoad() {
    super.viewDidLoad()

    embedPlugInView()
    
    playEngine = SimplePlayEngine(componentType: kAudioUnitType_Effect)

    var desc = AudioComponentDescription()
    desc.componentType = kAudioUnitType_Effect
    desc.componentSubType = fourCharCodeFrom(string: "IpeF")
    desc.componentManufacturer = fourCharCodeFrom(string: "Acme")
    desc.componentFlags = 0
    desc.componentFlagsMask = 0

    AUAudioUnit.registerSubclass(IPlugAUAudioUnit.self, as: desc, name:"iPlug: Local IPlugEffect", version: UInt32.max)

    playEngine.selectAudioUnitWithComponentDescription(desc) {
    // This is an asynchronous callback when complete. Finish audio unit setup.

    }
  }

  func embedPlugInView() {
    let builtInPlugInsURL = Bundle.main.builtInPlugInsURL!
    let pluginURL = builtInPlugInsURL.appendingPathComponent("IPlugEffectAppExtension.appex")
    let appExtensionBundle = Bundle(url: pluginURL)

    let storyboard = UIStoryboard(name: "IPlugEffect-iOS-AUv3", bundle: appExtensionBundle)
    mPlugViewController = storyboard.instantiateInitialViewController()

    if let view = mPlugViewController.view {
      addChildViewController(mPlugViewController)
      view.frame = auContainerView.bounds

      auContainerView.addSubview(view)
      mPlugViewController.didMove(toParentViewController: self)
    }
  }

  @IBAction func togglePlay(_ sender: AnyObject?) {
    let isPlaying = playEngine.togglePlay()
    let titleText = isPlaying ? "Stop" : "Play"
    playButton.setTitle(titleText, for: UIControlState())
  }
}
