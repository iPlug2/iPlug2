//
//  VUView.swift
//  AttenuatorAU
//
//  Created by Vlad Gorlov on 05.04.20.
//  Copyright © 2020 Vlad Gorlov. All rights reserved.
//

import Foundation
import Metal
import MetalKit
import SwiftUI

#if os(macOS)
struct VUSwiftUIView : NSViewRepresentable {
  let onRender: (() -> Float)

  func makeNSView(context: Context) -> VUView {
    return VUView(onRender: self.onRender)
  }
  
  func updateNSView(_ nsView: VUView, context: Context) {
  }
}
#else
struct VUSwiftUIView : UIViewRepresentable {
  let onRender: (() -> Float)

  func makeUIView(context: Context) -> VUView {
    return VUView(onRender: self.onRender)
  }

  func updateUIView(_ uiView: VUView, context: Context) {
  }
}
#endif

class VUView: MTKView {
  
  public enum Error: Swift.Error {
     case unableToInitialize(Any.Type)
  }
  
  private(set) var viewportSize = vector_float2(100, 100)
  
  private var metalDevice: MTLDevice!
  private var library: MTLLibrary!
  private var commandQueue: MTLCommandQueue!
  private var pipelineState: MTLRenderPipelineState!

  private var colorData = vector_float4(0, 0, 1, 1)
  private var verticesData = [vector_float2]()
  private var level: Float = 0
  
  var onRender: (() -> Float)?
  
  init(onRender: @escaping () -> Float) /* throws*/ {
     self.onRender = onRender
     let device = MTLCreateSystemDefaultDevice()
     super.init(frame: .zero, device: device)
     
     // Clear color. See: https://forums.developer.apple.com/thread/26461
     clearColor = MTLClearColorMake(0, 0, 0, 0)
     
     do {
        if let device = device {
          metalDevice = device
          colorPixelFormat = MTLPixelFormat.bgra8Unorm // Actually it is default value
          delegate = self
        } else {
          throw Error.unableToInitialize(MTLDevice.self)
        }
        
        guard let url = Bundle(for: type(of: self)).url(forResource: "default", withExtension: "metallib") else {
          throw Error.unableToInitialize(URL.self)
        }
        
        library = try metalDevice.makeLibrary(filepath: url.path)
        guard let commandQueue = metalDevice.makeCommandQueue() else {
          throw Error.unableToInitialize(MTLCommandQueue.self)
        }
        self.commandQueue = commandQueue
        
        guard let vertexProgram = library.makeFunction(name: "vertex_line") else {
          throw Error.unableToInitialize(MTLFunction.self)
        }
        guard let fragmentProgram = library.makeFunction(name: "fragment_line") else {
          throw Error.unableToInitialize(MTLFunction.self)
        }
        
        let pipelineStateDescriptor = MTLRenderPipelineDescriptor()
        pipelineStateDescriptor.vertexFunction = vertexProgram
        pipelineStateDescriptor.fragmentFunction = fragmentProgram
        // Alternatively can be set from drawable.texture.pixelFormat
        pipelineStateDescriptor.colorAttachments[0].pixelFormat = colorPixelFormat
        pipelineState = try metalDevice.makeRenderPipelineState(descriptor: pipelineStateDescriptor)
         
     } catch {
        assertionFailure()
        print(String(describing: error))
     }
  }
  
  required init(coder: NSCoder) {
     fatalError()
  }
}

extension VUView: MTKViewDelegate {
  
  func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
     viewportSize.x = Float(size.width)
     viewportSize.y = Float(size.height)
  }
  
  func draw(in view: MTKView) {
    #if os(macOS)
     if inLiveResize {
        return
     }
    #endif
     if let drawable = currentDrawable, let descriptor = currentRenderPassDescriptor {
        autoreleasepool {
          do {
             try render(drawable: drawable, renderPassDescriptor: descriptor)
          } catch {
             print(String(describing: error))
             assertionFailure(String(describing: error))
          }
        }
     }
  }
  
}

extension VUView {
  
  func render(drawable: CAMetalDrawable, renderPassDescriptor: MTLRenderPassDescriptor) throws {
    guard let commandBuffer = commandQueue.makeCommandBuffer() else {
      throw Error.unableToInitialize(MTLCommandBuffer.self)
    }

    // Transparent Metal background. See: https://forums.developer.apple.com/thread/26461
    renderPassDescriptor.colorAttachments[0].loadAction = .clear

    guard let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
      throw Error.unableToInitialize(MTLRenderCommandEncoder.self)
    }

    do {
      renderEncoder.setRenderPipelineState(pipelineState)
        
      let width = Double(viewportSize.x)
      let height = Double(viewportSize.y)
      let viewPort = MTLViewport(originX: 0, originY: 0, width: width, height: height, znear: 0, zfar: 1)
      renderEncoder.setViewport(viewPort)
      try prepareEncoder(encoder: renderEncoder)

      renderEncoder.endEncoding()

      commandBuffer.present(drawable)
      commandBuffer.commit()
    } catch {
      renderEncoder.endEncoding()
      throw error
    }
  }
  
  func prepareEncoder(encoder: MTLRenderCommandEncoder) throws {
    verticesData.removeAll(keepingCapacity: true)
    level = onRender?() ?? 0
    if level <= 0 {
      return
    }
    
    let y = max(Float(viewportSize.y * level), 1)
    let vertices = makeRectangle(xMin: 0, xMax: viewportSize.x, yMin: 0, yMax: y)
    verticesData += vertices
    
    encoder.setVertexBytes(&verticesData, length: verticesData.count * MemoryLayout<vector_float2>.stride, index: 0)
    encoder.setVertexBytes(&colorData, length: MemoryLayout<vector_float4>.stride, index: 1)
    encoder.setVertexBytes(&viewportSize, length: MemoryLayout<vector_float2>.stride, index: 2)
    
    encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: verticesData.count)
  }
  
  func makeRectangle(xMin: Float, xMax: Float, yMin: Float, yMax: Float) -> [vector_float2] {
    // Adding 2 triangles to represent rectangle.
    return [vector_float2(xMin, yMin), vector_float2(xMin, yMax), vector_float2(xMax, yMax),
            vector_float2(xMin, yMin), vector_float2(xMax, yMax), vector_float2(xMax, yMin)]
  }
}
