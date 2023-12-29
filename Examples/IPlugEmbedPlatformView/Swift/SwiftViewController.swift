import Foundation

@objc class SwiftViewController : PlatformViewController
{
  required init?(coder: NSCoder) {

    super.init(coder: coder)
  }
  
  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
  }
  
  override func viewDidLoad() {

    let hostingVC = PlatformHostingController(rootView: FileBrowserView(folderPath: "/Users/oli/Library/Mobile Documents/iCloud~com~OliLarkin~NeuralAmpModeler/Documents/Models", onFileSelected: { selectedFilePath in
    
      print(selectedFilePath)
      
    }))
    
    addChild(hostingVC)
    
    #if os(iOS)

    hostingVC.didMove(toParent: self)

    #endif

    self.view.addSubview(hostingVC.view)
    hostingVC.view.pinToSuperviewEdges()
  }
}
