/*
  This is the initial ios app extension view controller, which is referenced by IPlugEffect-iOS-MainInterface storyboard
*/

import UIKit
import CoreAudioKit

public class IPlugEffectViewController: AUViewController {
  @IBOutlet weak var IGraphicsView: UIView!
  var parameterObserverToken: AUParameterObserverToken?
  var scrollView: UIScrollView!

  public var audioUnit: IPlugAUAudioUnit? {
    // this variable gets set when loading in stand-alone app, not when loaded by a host
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
    guard let paramTree = audioUnit?.parameterTree else { return }
    
    let frame = self.view.bounds;
    scrollView = UIScrollView(frame: frame)
    self.view.addSubview(scrollView)

    var y = 10
    var idx = 0
    let sliderRowHeight = 40
    
    for param in paramTree.allParameters {
    
      let stack = UIStackView(frame:CGRect(x: 10, y: y, width: Int(frame.width - 20), height: sliderRowHeight))
      stack.axis = UILayoutConstraintAxis.horizontal
      stack.distribution = UIStackViewDistribution.equalSpacing
      stack.alignment = UIStackViewAlignment.leading
      stack.spacing = CGFloat(sliderRowHeight)
      
//      let nameLabel = UILabel()
//      nameLabel.text = param.displayName
//      nameLabel.backgroundColor = #colorLiteral(red: 0.501960814, green: 0.501960814, blue: 0.501960814, alpha: 1)
//      nameLabel.textColor = #colorLiteral(red: 1.0, green: 1.0, blue: 1.0, alpha: 1.0)
//      stack.addArrangedSubview(nameLabel)
      
      let slider = UISlider()
      slider.minimumValue = param.minValue
      slider.maximumValue = param.maxValue
      slider.isContinuous = true
      slider.value = param.value
//      slider.heightAnchor.constraint(equalToConstant:100)
//      slider.widthAnchor.constraint(equalToConstant:100)
      slider.backgroundColor = #colorLiteral(red: 0.9607843161, green: 0.7058823705, blue: 0.200000003, alpha: 1)
      slider.tag = idx
      slider.addTarget(self, action: #selector(IPlugEffectViewController.sliderValueDidChange(_:)), for: .valueChanged)
      stack.addArrangedSubview(slider)
      
//      let valLabel = UILabel()
//      valLabel.text = "test"
//      stack.addArrangedSubview(valLabel)
      
      scrollView.addSubview(stack)
      
      y = y + sliderRowHeight
      idx = idx + 1
    }
    
    parameterObserverToken = paramTree.token(byAddingParameterObserver: { [weak self] address, value in
      guard let strongSelf = self else { return }
      DispatchQueue.main.async {
        let stack = strongSelf.scrollView.subviews[2] as! UIStackView
        let slider = stack.subviews[0] as! UISlider
        slider.value = value
        
        //TODO: update slider/label
      }
    })
  
  }
  
  @objc func sliderValueDidChange(_ sender:UISlider!)
  {
    print("Slider value changed")
    print("Slider tag value \(sender.tag)")
  }
}
