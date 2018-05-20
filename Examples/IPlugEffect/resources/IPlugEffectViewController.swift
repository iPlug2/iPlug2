/*
  This is the initial app extension view controller, which is referenced by IPlugEffect-iOS-MainInterface storyboard
*/

import UIKit

class IPlugEffectViewController: UIViewController {
  @IBOutlet weak var slider: UISlider!
  @IBOutlet weak var customView: UIView!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    var auv3ViewController = IPlugViewController();
    
    // load and attach subview
  }
  
  override func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
  }
}
