/*
 
  This is the initial app extension view controller, which is linked by IPlugEffect-iOS-MainInterface storyboard
 
 
*/

import UIKit

class IPlugEffectViewController: UIViewController {
  @IBOutlet weak var slider: UISlider!
  @IBOutlet weak var customView: UIView!
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    // load and attach subview
  }
  
  override func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
  }
}
