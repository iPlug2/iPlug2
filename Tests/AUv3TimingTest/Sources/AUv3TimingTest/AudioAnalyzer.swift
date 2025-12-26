import AVFoundation

/// Utilities for analyzing audio buffers
struct AudioAnalyzer {
    /// Threshold for detecting non-silent samples
    static let silenceThreshold: Float = 0.001

    /// Find the first sample index where audio exceeds silence threshold
    /// Returns nil if the entire buffer is silent
    static func findFirstNonSilentSample(in buffer: AVAudioPCMBuffer) -> Int? {
        guard let floatData = buffer.floatChannelData else { return nil }

        let frameCount = Int(buffer.frameLength)
        let channelCount = Int(buffer.format.channelCount)

        for sampleIdx in 0..<frameCount {
            for channel in 0..<channelCount {
                let sample = abs(floatData[channel][sampleIdx])
                if sample > silenceThreshold {
                    return sampleIdx
                }
            }
        }
        return nil
    }

    /// Calculate RMS energy of the buffer
    static func calculateRMS(in buffer: AVAudioPCMBuffer) -> Float {
        guard let floatData = buffer.floatChannelData else { return 0 }

        let frameCount = Int(buffer.frameLength)
        let channelCount = Int(buffer.format.channelCount)

        var sumSquares: Float = 0
        var totalSamples = 0

        for channel in 0..<channelCount {
            for sampleIdx in 0..<frameCount {
                let sample = floatData[channel][sampleIdx]
                sumSquares += sample * sample
                totalSamples += 1
            }
        }

        guard totalSamples > 0 else { return 0 }
        return sqrt(sumSquares / Float(totalSamples))
    }

    /// Dump first N samples to stdout for debugging
    static func dumpSamples(in buffer: AVAudioPCMBuffer, count: Int, channel: Int = 0) {
        guard let floatData = buffer.floatChannelData else {
            print("No float data available")
            return
        }

        let frameCount = min(count, Int(buffer.frameLength))
        print("First \(frameCount) samples (channel \(channel)):")

        for i in 0..<frameCount {
            let sample = floatData[channel][i]
            let bar = String(repeating: "#", count: min(40, Int(abs(sample) * 40)))
            print(String(format: "  [%3d] %+.6f %@", i, sample, bar))
        }
    }

    /// Find the sample value at a specific index
    static func sampleValue(in buffer: AVAudioPCMBuffer, at index: Int, channel: Int = 0) -> Float? {
        guard let floatData = buffer.floatChannelData,
              index < Int(buffer.frameLength),
              channel < Int(buffer.format.channelCount) else {
            return nil
        }
        return floatData[channel][index]
    }
}
