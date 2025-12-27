import AVFoundation
import AudioToolbox

/// Errors that can occur during testing
enum TestError: Error, CustomStringConvertible {
    case pluginNotFound
    case noMIDIBlock
    case engineSetupFailed(String)
    case renderFailed(String)
    case allSilent

    var description: String {
        switch self {
        case .pluginNotFound:
            return "IPlugInstrument AUv3 not found. Make sure it's built and registered."
        case .noMIDIBlock:
            return "Audio unit does not support MIDI scheduling"
        case .engineSetupFailed(let msg):
            return "Engine setup failed: \(msg)"
        case .renderFailed(let msg):
            return "Render failed: \(msg)"
        case .allSilent:
            return "Output buffer is completely silent - plugin may not be responding"
        }
    }
}

/// Result of a single timing test
struct TestResult {
    let scheduledOffset: Int
    let detectedOffset: Int?
    let passed: Bool
    let tolerance: Int

    var delta: Int? {
        guard let detected = detectedOffset else { return nil }
        return detected - scheduledOffset
    }
}

/// Convert a 4-character string to OSType (FourCC)
func fourCharCode(_ string: String) -> OSType {
    var result: OSType = 0
    for char in string.utf8.prefix(4) {
        result = (result << 8) | OSType(char)
    }
    return result
}

/// Core test logic for AUv3 sample-accurate MIDI timing
class AUv3TimingTester {
    // Configuration
    let sampleRate: Double
    let bufferSize: UInt32
    let testNote: UInt8 = 60       // Middle C
    let testVelocity: UInt8 = 127
    let tolerance: Int

    // Audio engine and AU
    private var engine: AVAudioEngine!
    private var avAudioUnit: AVAudioUnit!

    // Verbose mode
    var verbose: Bool = false

    init(sampleRate: Double = 44100.0, bufferSize: UInt32 = 512, tolerance: Int = 2) {
        self.sampleRate = sampleRate
        self.bufferSize = bufferSize
        self.tolerance = tolerance
    }

    /// Find and instantiate the IPlugInstrument AUv3
    func instantiateAUv3() async throws {
        // Build AudioComponentDescription for IPlugInstrument
        // From config.h: PLUG_TYPE=1 (aumu), PLUG_UNIQUE_ID='PmBl', PLUG_MFR_ID='Acme'
        var desc = AudioComponentDescription(
            componentType: kAudioUnitType_MusicDevice,
            componentSubType: fourCharCode("PmBl"),
            componentManufacturer: fourCharCode("Acme"),
            componentFlags: 0,
            componentFlagsMask: 0
        )

        if verbose {
            print("Looking for AUv3 with description:")
            print("  Type: \(String(format: "%c%c%c%c", (desc.componentType >> 24) & 0xFF, (desc.componentType >> 16) & 0xFF, (desc.componentType >> 8) & 0xFF, desc.componentType & 0xFF))")
            print("  SubType: \(String(format: "%c%c%c%c", (desc.componentSubType >> 24) & 0xFF, (desc.componentSubType >> 16) & 0xFF, (desc.componentSubType >> 8) & 0xFF, desc.componentSubType & 0xFF))")
            print("  Manufacturer: \(String(format: "%c%c%c%c", (desc.componentManufacturer >> 24) & 0xFF, (desc.componentManufacturer >> 16) & 0xFF, (desc.componentManufacturer >> 8) & 0xFF, desc.componentManufacturer & 0xFF))")
        }

        // Check if component exists
        guard AudioComponentFindNext(nil, &desc) != nil else {
            throw TestError.pluginNotFound
        }

        // Instantiate via AVAudioUnit using withCheckedThrowingContinuation
        // Must use loadInProcess for command-line tool (no XPC/sandbox)
        avAudioUnit = try await withCheckedThrowingContinuation { continuation in
            AVAudioUnit.instantiate(with: desc, options: .loadInProcess) { audioUnit, error in
                if let error = error {
                    continuation.resume(throwing: error)
                } else if let audioUnit = audioUnit {
                    continuation.resume(returning: audioUnit)
                } else {
                    continuation.resume(throwing: TestError.pluginNotFound)
                }
            }
        }

        if verbose {
            print("  Found: \(avAudioUnit.auAudioUnit.audioUnitName ?? "unknown")")
            print("  Manufacturer: \(avAudioUnit.auAudioUnit.manufacturerName ?? "unknown")")
        }
    }

    /// Setup the audio engine in manual rendering mode
    func setupManualRendering() throws {
        engine = AVAudioEngine()
        engine.attach(avAudioUnit)

        let format = AVAudioFormat(standardFormatWithSampleRate: sampleRate, channels: 2)!

        // Enable offline/manual rendering mode
        try engine.enableManualRenderingMode(
            .offline,
            format: format,
            maximumFrameCount: bufferSize
        )

        // Connect AU to main mixer
        engine.connect(avAudioUnit, to: engine.mainMixerNode, format: format)

        // Prepare and start
        engine.prepare()
        try engine.start()

        if verbose {
            print("Engine configured for manual rendering:")
            print("  Sample rate: \(sampleRate) Hz")
            print("  Buffer size: \(bufferSize) samples")
            print("  Format: \(format)")
        }
    }

    /// Reset the engine for a new test
    func resetForNewTest() throws {
        engine.stop()
        engine.reset()

        // Re-enable manual rendering
        let format = AVAudioFormat(standardFormatWithSampleRate: sampleRate, channels: 2)!
        try engine.enableManualRenderingMode(
            .offline,
            format: format,
            maximumFrameCount: bufferSize
        )
        engine.prepare()
        try engine.start()
    }

    /// Run a single timing test with MIDI scheduled at a specific sample offset
    func runSingleTest(noteOffset: Int) throws -> TestResult {
        // Get the AU's scheduleMIDIEventBlock
        guard let scheduleMIDI = avAudioUnit.auAudioUnit.scheduleMIDIEventBlock else {
            throw TestError.noMIDIBlock
        }

        // Create output buffer
        guard let outputBuffer = AVAudioPCMBuffer(
            pcmFormat: engine.manualRenderingFormat,
            frameCapacity: bufferSize
        ) else {
            throw TestError.engineSetupFailed("Could not create output buffer")
        }

        // Schedule note-on at specific sample offset
        // AUEventSampleTime is the absolute sample time
        let noteOnData: [UInt8] = [0x90, testNote, testVelocity]
        noteOnData.withUnsafeBufferPointer { ptr in
            scheduleMIDI(AUEventSampleTime(noteOffset), 0, 3, ptr.baseAddress!)
        }

        // Render the buffer
        do {
            let status = try engine.renderOffline(AVAudioFrameCount(bufferSize), to: outputBuffer)
            if status != .success {
                throw TestError.renderFailed("Status: \(status.rawValue)")
            }
        } catch {
            throw TestError.renderFailed(error.localizedDescription)
        }

        // Analyze the output
        let detectedOffset = AudioAnalyzer.findFirstNonSilentSample(in: outputBuffer)

        if verbose {
            print("\n  Buffer analysis:")
            AudioAnalyzer.dumpSamples(in: outputBuffer, count: min(20, Int(bufferSize)))
            if let detected = detectedOffset {
                print("  First non-silent sample: \(detected)")
            } else {
                print("  Buffer is completely silent!")
            }
        }

        // Send note-off for cleanup
        let noteOffData: [UInt8] = [0x80, testNote, 0]
        noteOffData.withUnsafeBufferPointer { ptr in
            scheduleMIDI(AUEventSampleTime(bufferSize), 0, 3, ptr.baseAddress!)
        }

        // Determine pass/fail
        let passed: Bool
        if let detected = detectedOffset {
            let delta = abs(detected - noteOffset)
            passed = delta <= tolerance
        } else {
            passed = false
        }

        return TestResult(
            scheduledOffset: noteOffset,
            detectedOffset: detectedOffset,
            passed: passed,
            tolerance: tolerance
        )
    }

    /// Run all tests with multiple offsets
    func runAllTests(offsets: [Int]) async throws -> [TestResult] {
        print("================================================================================")
        print("AUv3 Sample-Accurate MIDI Timing Test")
        print("================================================================================\n")

        print("Finding and instantiating AUv3...")
        try await instantiateAUv3()
        print("  Audio Unit instantiated successfully\n")

        print("Setting up manual rendering mode...")
        try setupManualRendering()
        print("  Engine ready\n")

        print("Configuration:")
        print("  Plugin: IPlugInstrument (aumu/PmBl/Acme)")
        print("  Sample Rate: \(sampleRate) Hz")
        print("  Buffer Size: \(bufferSize) samples")
        print("  Tolerance: +/- \(tolerance) samples")
        print()

        print("Running timing tests...\n")

        var results: [TestResult] = []

        for (index, offset) in offsets.enumerated() {
            // Reset engine state between tests (except first)
            if index > 0 {
                try resetForNewTest()
            }

            print("Test \(index + 1): MIDI scheduled at sample \(offset)")
            let result = try runSingleTest(noteOffset: offset)

            if let detected = result.detectedOffset {
                let delta = detected - offset
                let passed = abs(delta) <= tolerance
                print("  Expected audio at: sample \(offset)")
                print("  Detected audio at: sample \(detected)")
                print("  Timing error: \(delta) samples")
                print("  Result: \(passed ? "PASS" : "FAIL")\n")

                results.append(TestResult(
                    scheduledOffset: offset,
                    detectedOffset: detected,
                    passed: passed,
                    tolerance: tolerance
                ))
            } else {
                print("  Expected audio at: sample \(offset)")
                print("  Detected audio at: (silent)")
                print("  Result: FAIL (no audio detected)\n")

                results.append(TestResult(
                    scheduledOffset: offset,
                    detectedOffset: nil,
                    passed: false,
                    tolerance: tolerance
                ))
            }
        }

        return results
    }

    /// Print summary and return overall pass/fail
    func printSummary(results: [TestResult]) -> Bool {
        let passCount = results.filter { $0.passed }.count
        let failCount = results.count - passCount

        print("================================================================================")
        print("SUMMARY")
        print("================================================================================")
        print("Tests Run: \(results.count)")
        print("Tests Passed: \(passCount)")
        print("Tests Failed: \(failCount)")
        print()

        let allPassed = failCount == 0
        if allPassed {
            print("Overall Result: PASS")
            print()
            print("Sample-accurate MIDI timing is working correctly.")
            print("All MIDI events triggered at their exact scheduled sample positions.")
        } else {
            print("Overall Result: FAIL")
            print()
            print("Sample-accurate MIDI timing is NOT working correctly.")
            print("MIDI events are not triggering at their scheduled sample positions.")
            print()
            print("Without the fix, all events would trigger at sample 0 of each buffer,")
            print("causing large timing errors proportional to the scheduled offset.")
        }
        print("================================================================================")

        return allPassed
    }
}
