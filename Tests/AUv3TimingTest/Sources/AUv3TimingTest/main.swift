import Foundation
import Dispatch

/// Parse command-line arguments
struct Arguments {
    var verbose: Bool = false
    var offsets: [Int] = [50, 100, 256, 400]
    var sampleRate: Double = 44100.0
    var bufferSize: UInt32 = 512
    var tolerance: Int = 2

    init() {
        let args = CommandLine.arguments

        for (index, arg) in args.enumerated() {
            switch arg {
            case "--verbose", "-v":
                verbose = true

            case "--offsets":
                if index + 1 < args.count {
                    offsets = args[index + 1]
                        .split(separator: ",")
                        .compactMap { Int($0.trimmingCharacters(in: .whitespaces)) }
                }

            case "--sample-rate":
                if index + 1 < args.count, let rate = Double(args[index + 1]) {
                    sampleRate = rate
                }

            case "--buffer-size":
                if index + 1 < args.count, let size = UInt32(args[index + 1]) {
                    bufferSize = size
                }

            case "--tolerance":
                if index + 1 < args.count, let tol = Int(args[index + 1]) {
                    tolerance = tol
                }

            case "--help", "-h":
                printUsage()
                exit(0)

            default:
                break
            }
        }
    }

    func printUsage() {
        print("""
        AUv3 Sample-Accurate MIDI Timing Test

        Usage: AUv3TimingTest [options]

        Options:
          --verbose, -v          Show detailed output including buffer dumps
          --offsets <list>       Comma-separated list of sample offsets to test
                                 (default: 0,100,256,400)
          --sample-rate <rate>   Audio sample rate in Hz (default: 44100)
          --buffer-size <size>   Buffer size in samples (default: 512)
          --tolerance <samples>  Allowed timing variance (default: 2)
          --help, -h             Show this help message

        Examples:
          AUv3TimingTest
          AUv3TimingTest --verbose
          AUv3TimingTest --offsets 0,50,100,200,300
          AUv3TimingTest --buffer-size 1024 --offsets 0,256,512,768

        Prerequisites:
          - IPlugInstrument AUv3 must be built and registered
          - Build the macOS-APP target from Examples/IPlugInstrument

        This test verifies that AUv3 plugins correctly handle sample-accurate
        MIDI timing. With the fix, MIDI events should trigger at their exact
        scheduled sample position. Without the fix, all events trigger at
        sample 0 of each buffer.
        """)
    }
}

/// Run the async main function
func runAsyncMain() async -> Int32 {
    let args = Arguments()

    let tester = AUv3TimingTester(
        sampleRate: args.sampleRate,
        bufferSize: args.bufferSize,
        tolerance: args.tolerance
    )
    tester.verbose = args.verbose

    do {
        let results = try await tester.runAllTests(offsets: args.offsets)
        let passed = tester.printSummary(results: results)
        return passed ? 0 : 1
    } catch {
        print("\nError: \(error)")
        print()
        print("Make sure IPlugInstrument AUv3 is built and registered.")
        print("Build the 'macOS-APP' target from Examples/IPlugInstrument/")
        return 2
    }
}

// Entry point - use dispatchMain to keep runloop alive
var exitCode: Int32 = 0

// Spawn the async task on main actor queue
DispatchQueue.main.async {
    Task {
        exitCode = await runAsyncMain()
        exit(exitCode)
    }
}

// Keep the main runloop alive until exit() is called
dispatchMain()
