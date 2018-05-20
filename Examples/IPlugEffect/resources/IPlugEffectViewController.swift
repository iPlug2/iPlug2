/*
  This is the initial ios app extension view controller, which is referenced by IPlugEffect-iOS-MainInterface storyboard
*/

import UIKit
import CoreAudioKit

class IPlugEffectViewController: AUViewController {
  @IBOutlet weak var slider: UISlider!
  @IBOutlet weak var customView: UIView!
  
  public var audioUnit: IPlugAUAudioUnit? {
    // this variable gets set when loading in stand-alone app, not when loaded by a host
    didSet {
      DispatchQueue.main.async {
        if self.isViewLoaded {
        }
      }
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    // TODO: load and attach iGraphics subview
  }
  
  override func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
  }
}

//I would like to move this to the file IPlugEffectViewController+AUAudioUnitFactory but there is a problem because it doesn't find IPlugEffectViewController
extension IPlugEffectViewController: AUAudioUnitFactory {

  public override func beginRequest(with context: NSExtensionContext) { }

  public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
    audioUnit = try IPlugAUAudioUnit(componentDescription: componentDescription, options: [])
    return audioUnit!
  }
}
