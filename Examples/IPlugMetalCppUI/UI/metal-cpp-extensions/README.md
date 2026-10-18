# metal-cpp-extensions

C++ bindings into Apple's platform SDK to help with using Metal directly from C++. The majority of the code is from Apple, taken from their [Learn Metal with C++](https://developer.apple.com/metal/LearnMetalCPP.zip) example but with some things added by me. This will probably never be an exhaustive set of bindings, I just add what I need when I need them.

## Additions to Apple's code

* AppKit
    * NS::GridView
        * only creating from an NS::Array of NS::Arrays of NS::Views.
    * NS::StackView
        * only creating from an NS::Array NS::Views.
        * allows setting orientation.
        * allows setting the distribution.
* MetalKit
    * MTK::TextureLoader
        * only has the `MTL::Texture* newTexture( const NS::URL* URL, const NS::Dictionary* options, NS::Error** error );` method, and I haven't tested setting anything other than `nullptr` for the options.
* Metal Performance Shaders
    * MPS::Image
        * only has creation from MTL::Texture and MPS::ImageDescriptor; and conversion back to MTL::Texture.
    * MPS::ImageGaussianBlur
    * MPS::ImageGaussianPyramid
        * Only the in place `encode` method works. I think this is an MPS internal thing, see Apple's [documentation for MPSImageGaussianPyramid] (https://developer.apple.com/documentation/metalperformanceshaders/mpsimagegaussianpyramid?language=objc).
    * MPS::ImageSubtract
