/*
  This is the initial ios app extension view controller, which is referenced by IPlugControls-iOS-MainInterface storyboard
*/

import UIKit
import CoreAudioKit

public class IPlugControlsViewController: AUViewController {

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
    // To use the generic UI (swift), change the view in IPlugControls-iOS-MainInterface.storyboard to custom class: GenericUI  module: IPlugControlsFramework
    if let view = self.view as? GenericUI {
      view.createGenericUI(audioUnit!)
    }
    else {
      audioUnit?.openWindow(self.view)
    }
  }
  
  override public func viewDidLayoutSubviews()
  {
//    audioUnit?.resize(self.view.bounds)
  }
}
