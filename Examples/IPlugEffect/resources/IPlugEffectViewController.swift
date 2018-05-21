/*
  This is the initial ios app extension view controller, which is referenced by IPlugEffect-iOS-MainInterface storyboard
*/

import UIKit
import CoreAudioKit

public class IPlugEffectViewController: AUViewController {
  @IBOutlet weak var IGraphicsView: UIView!
  @IBOutlet weak var GenericView: GenericUI!
  
  public var audioUnit: IPlugAUAudioUnit? {
    // this variable gets set when loading in process in stand-alone app, i.e. not when loaded by a host
    didSet {
      DispatchQueue.main.async {
        if self.isViewLoaded {
          self.connectViewWithAU()
        }
      }
    }
  }
  
  // this is where it happens when auv3 is loaded by a host
  override public func viewDidLoad() {
    super.viewDidLoad()
    guard audioUnit != nil else { return }
    connectViewWithAU()
  }
  
  override public func didReceiveMemoryWarning() {
    super.didReceiveMemoryWarning()
  }
  
  func connectViewWithAU() {
    // TODO: load and attach iGraphics subview
    GenericView.createGenericUI(audioUnit!)
    
    let incoming = audioUnit?.openWindow()
    
    print(incoming?.bounds ??  CGRect(x: 0, y: 0, width: 100, height: 100))
    IGraphicsView.addSubview(incoming!)
    
//    let button = UIButton(frame: CGRect(x: 0, y: 0, width: 100, height: 100))
//    button.backgroundColor = #colorLiteral(red: 1, green: 0.1491314173, blue: 0, alpha: 1)
//    IGraphicsView.addSubview(button)
  }
}
