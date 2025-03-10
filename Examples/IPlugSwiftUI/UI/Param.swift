import SwiftUI
import Foundation

enum ParamShape: Equatable {
    case linear
    case powCurve(Double)
    case exponential
    
    // MARK: - Shape conversions
    
    func normalizedToValue(_ normalized: Double, in param: Param) -> Double {
        let minVal = param.minValue
        let maxVal = param.maxValue
        
        switch self {
        case .linear:
            return minVal + normalized * (maxVal - minVal)
            
        case .powCurve(let shape):
            return minVal + pow(normalized, shape) * (maxVal - minVal)
            
        case .exponential:
            let safeMin = max(1.0e-8, minVal)
            let safeMax = maxVal
            let mAdd = log(safeMin)
            let mMul = log(safeMax / safeMin)
            return exp(mAdd + normalized * mMul)
        }
    }
    
    func valueToNormalized(_ value: Double, in param: Param) -> Double {
        let minVal = param.minValue
        let maxVal = param.maxValue
        
        switch self {
        case .linear:
            return (value - minVal) / (maxVal - minVal)
            
        case .powCurve(let shape):
            let ratio = (value - minVal) / (maxVal - minVal)
            return pow(ratio, 1.0 / shape)
            
        case .exponential:
            let safeMin = max(1.0e-8, minVal)
            let safeMax = maxVal
            let mAdd = log(safeMin)
            let mMul = log(safeMax / safeMin)
            
            return (log(value) - mAdd) / mMul
        }
    }
}

func swiftShape(fromID id: Int, exponent: Double) -> ParamShape {
    switch id {
    case 0:  return .linear
    case 1:  return .powCurve(exponent)
    case 2:  return .exponential
    default: return .linear
    }
}

struct ParamDisplayText {
    let value: Double
    let text: String
}

struct ParamFlags: OptionSet {
    let rawValue: Int
    
    static let none            = ParamFlags([])
    static let cannotAutomate  = ParamFlags(rawValue: 1 << 0) // 0x1
    static let stepped         = ParamFlags(rawValue: 1 << 1) // 0x2
    static let negateDisplay   = ParamFlags(rawValue: 1 << 2) // 0x4
    static let signDisplay     = ParamFlags(rawValue: 1 << 3) // 0x8
    static let meta            = ParamFlags(rawValue: 1 << 4) // 0x10
}

class Param: ObservableObject {
    let id: Int
    var name: String
    var defaultValue: Double
    var step: Double
    var minValue: Double
    var maxValue: Double
    var label: String
    var group: String
    var flags: ParamFlags = .none
    var shape: ParamShape = .linear
    var displayPrecision = 2
    
    @Published var value: Double
    
    /// A list of special display mappings: if `value` == this, show `text`
    private var displayTexts: [ParamDisplayText] = []
    
    init(id: Int,
         name: String = "",
         defaultValue: Double = 0.0,
         minValue: Double = 0.0,
         maxValue: Double = 1.0,
         step: Double = 0.0,
         label: String = "",
         group: String = "",
         shape: ParamShape = .linear,
         flags: ParamFlags = .none)
    {
        self.id = id
        self.name = name
        self.defaultValue = defaultValue
        self.minValue = minValue
        self.maxValue = maxValue
        self.step = step
        self.label = label
        self.group = group
        self.shape = shape
        
        self.value = defaultValue
        self.value = self.constrain(self.value)
    }
}

// MARK: - Display texts

extension Param {
    func setDisplayText(for rawValue: Double, text: String) {
        if let index = displayTexts.firstIndex(where: { $0.value == rawValue }) {
            displayTexts[index] = ParamDisplayText(value: rawValue, text: text)
        } else {
            displayTexts.append(ParamDisplayText(value: rawValue, text: text))
        }
    }
    
    /// Retrieve a custom display string, if one is set for `value`.
    /// Returns `nil` if no special text is configured.
    func getDisplayText(for rawValue: Double) -> String? {
        guard let dt = displayTexts.first(where: { $0.value == rawValue }) else {
            return nil
        }
        return dt.text
    }
}

// MARK: - Normalized <-> Value conversions

extension Param {
    func toNormalized(_ rawValue: Double) -> Double {
        let constrained = constrain(rawValue)
        let norm = shape.valueToNormalized(constrained, in: self)
        return norm.clamped(to: 0.0...1.0)
    }
    
    func fromNormalized(_ normalized: Double) -> Double {
        let clippedNorm = normalized.clamped(to: 0.0...1.0)
        let rawVal = shape.normalizedToValue(clippedNorm, in: self)
        return constrain(rawVal)
    }
    
    func setNormalized(_ normalized: Double) {
        self.value = fromNormalized(normalized)
    }
    
    private func constrain(_ x: Double) -> Double {
        var out = x
        
        if step > 0 {
            out = (out / step).rounded() * step
        }
        if out < minValue { out = minValue }
        if out > maxValue { out = maxValue }
        
        return out
    }
}

extension Comparable {
    func clamped(to range: ClosedRange<Self>) -> Self {
        return min(max(self, range.lowerBound), range.upperBound)
    }
}

extension Param {
    /// Produce a user-facing string for the current parameter value.
    /// If we find a custom display text (and `exactValueMatch = true`), return that;
    /// otherwise format the numeric value.
    func displayString() -> String {
        displayString(for: value)
    }
    
    /// If `normalized = true`, interpret `v` as [0…1], convert to raw, then produce a string.
    func displayString(for v: Double, normalized: Bool = false) -> String {
        let raw = normalized ? fromNormalized(v) : v
        
        if let text = getDisplayText(for: raw) {
            return text
        }
      
        // TODO: is this nessecary?
        // TODO: Param precision and negating
        let numberFormatter = NumberFormatter()
        numberFormatter.minimumFractionDigits = 0
        numberFormatter.maximumFractionDigits = 2
      
        let numStr = numberFormatter.string(from: NSNumber(value: raw)) ?? "\(raw)"
        
        if !label.isEmpty {
            return "\(numStr) \(label)"
        } else {
            return numStr
        }
    }
}

extension Param {
    /// - Parameters:
    ///   - value: The "input" parameter value (raw or normalized).
    ///   - normalized: If `true`, `value` is in [0..1], and we must expand via `fromNormalized`.
    ///   - withDisplayText: If `true`, we try custom strings (e.g. "Off", "∞") if exact match.
    /// - Returns: A string representation suitable for display.
    func getDisplay(value: Double,
                    normalized: Bool = false,
                    withDisplayText: Bool = true) -> String
    {
        let rawValue = normalized ? fromNormalized(value) : value
        
        if withDisplayText, let customText = getDisplayText(for: rawValue) {
            return customText
        }
        
        var displayVal = rawValue
        
        if flags.contains(.negateDisplay) {
            displayVal = -displayVal
        }
        
        let numberFormatter = NumberFormatter()
        numberFormatter.minimumFractionDigits = 0
        numberFormatter.maximumFractionDigits = displayPrecision
        
        if flags.contains(.signDisplay) {
            numberFormatter.positivePrefix = "+"
            numberFormatter.negativePrefix = "-"
        }
        
        let numericStr = numberFormatter.string(from: displayVal as NSNumber) ?? "\(displayVal)"
        
        if !label.isEmpty {
            return "\(numericStr) \(label)"
        } else {
            return numericStr
        }
    }
    
    /// Shortcut: `getDisplay(value: self.value, normalized: false, withDisplayText: true)`
    func getDisplay() -> String {
        getDisplay(value: self.value,
                   normalized: false,
                   withDisplayText: true)
    }
}

extension Param {
    /// Converts the parameter's stored value (assumed to be an index)
    /// into an enum value. The enum must conform to RawRepresentable & CaseIterable,
    /// with RawValue of Int.
    func asEnum<T: RawRepresentable & CaseIterable>() -> T where T.RawValue == Int {
        let cases = Array(T.allCases)
        let idx = Int(round(self.value))
        let boundedIdx = min(max(idx, 0), cases.count - 1)
        return cases[boundedIdx]
    }
    
    /// Sets the parameter's value based on the provided enum.
    /// The enum's raw value is assumed to be (or correspond to) an index
    /// that is stored in the parameter.
    func setEnum<T: RawRepresentable & CaseIterable>(_ newEnum: T) where T.RawValue == Int {
        let cases = Array(T.allCases)
        if let idx = cases.firstIndex(where: { $0.rawValue == newEnum.rawValue }) {
            self.value = Double(idx)
        }
    }
}
