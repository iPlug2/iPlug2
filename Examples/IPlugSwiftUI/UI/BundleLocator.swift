import Foundation

final class BundleLocator {
    /// The shared singleton instance
    static let shared = BundleLocator()
    
    /// Stores the bundle identifier for the application
    private var bundleId: String?
    
    /// Private initializer to enforce singleton pattern
    private init() {}
    
    /// Sets the bundle identifier to use throughout the application
    func initialize(with bundleId: String) {
        self.bundleId = bundleId
    }
    
    /// Returns the appropriate bundle for resource loading using the stored bundle ID
    func getBundle() -> Bundle? {
        guard let bundleId = self.bundleId else {
            print("Error: BundleLocator not initialized with a bundle ID")
            return nil
        }
        
        return getBundle(bundleId: bundleId)
    }
    
    /// Returns the appropriate bundle for resource loading with an explicit bundle ID
    func getBundle(bundleId: String) -> Bundle {
        let frameworkBundle = Bundle(for: BundleLocator.self)
        let bundle = (frameworkBundle.bundleIdentifier != Bundle.main.bundleIdentifier)
            ? frameworkBundle
            : Bundle(identifier: bundleId)!
        return bundle
    }
}
