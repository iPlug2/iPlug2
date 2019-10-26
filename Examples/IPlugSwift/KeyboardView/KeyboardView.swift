//
//  KeyboardView.swift
//  AudioKitSynthOne
//
//  Created by AudioKit Contributors on 8/14/17.
//  Copyright © 2017 AudioKit. All rights reserved.
//

//import AudioKit
//import UIKit

// swiftlint:disable file_length

public typealias MIDINoteNumber = UInt8
public typealias MIDIVelocity = UInt8

/// Delegate for keyboard events
public protocol AKKeyboardDelegate: class {

    /// Note on evenets
    func noteOn(note: MIDINoteNumber, velocity: MIDIVelocity)
    
    /// Note off events
    func noteOff(note: MIDINoteNumber)
}

/// Clickable keyboard mainly used for AudioKit playgrounds
@IBDesignable open class KeyboardView: UIView/*, AKMIDIListener*/ {

    /// Number of octaves displayed at once
    @IBInspectable open var octaveCount: Int = 2

    /// Lowest octave dispayed
    @IBInspectable open var firstOctave: Int = 2 {
        didSet {
            setNeedsDisplay()
        }
    }

    /// Relative measure of the height of the black keys
    @IBInspectable open var topKeyHeightRatio: CGFloat = 0.55

    // Key Labels: 0 = don't display, 1 = every C note, 2 = every note
    @IBInspectable open var  labelMode: Int = 2

    /// Color of the polyphonic toggle button
    @IBInspectable open var polyphonicButton: UIColor = #colorLiteral(red: 1.000, green: 1.000, blue: 1.000, alpha: 1.000)

    /// White key color
    @IBInspectable open var  whiteKeyOff: UIColor = #colorLiteral(red: 1, green: 1, blue: 1, alpha: 1)

    /// Black key color
    @IBInspectable open var  blackKeyOff: UIColor = #colorLiteral(red: 0.06666666667, green: 0.06666666667, blue: 0.06666666667, alpha: 1)

    /// Activated key color
    @IBInspectable open var  keyOnUserColor: UIColor = #colorLiteral(red: 0.9607843137, green: 0.5098039216, blue: 0, alpha: 1)

    var keyOnColor: UIColor = #colorLiteral(red: 0.4549019608, green: 0.6235294118, blue: 0.7254901961, alpha: 1)

    /// Keyboard Mode, white or dark
    @IBInspectable open var  darkMode: Bool = false

    /// Class to handle user actions
    open weak var delegate: AKKeyboardDelegate?

    var oneOctaveSize = CGSize.zero

    var xOffset: CGFloat = 1

    var onKeys = Set<MIDINoteNumber>()

    var topKeyWidthIncrease: CGFloat = 4

    var holdMode = false {
        didSet {
            if !holdMode {
                allNotesOff()
            }
        }
    }

    let baseMIDINote = 24 // MIDINote 24 is C0

    var isShown = true // used to persist keyboard position when presets panel is displayed

    /// Allows multiple notes to play concurrently
    open var polyphonicMode = true {
        didSet {
            allNotesOff()
        }
    }

//    private var arpIsOn: Bool {
//        return Conductor.sharedInstance.synth.getSynthParameter(.arpIsOn) > 0 ? true : false
//    }
//
//    private var arpIsSequencer: Bool {
//        return Conductor.sharedInstance.synth.getSynthParameter(.arpIsSequencer) > 0 ? true : false
//    }

    let naturalNotes = ["C", "D", "E", "F", "G", "A", "B"]

    let notesWithSharps = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

    let topKeyNotes = [0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 11]

    let whiteKeyNotes = [0, 2, 4, 5, 7, 9, 11]

    func getNoteName(_ note: Int) -> String {
        let keyInOctave = note % 12
        return notesWithSharps[keyInOctave]
    }

    func getWhiteNoteName(_ keyIndex: Int) -> String {
         return naturalNotes[keyIndex]
    }

    // MARK: - Initialization

    /// Initialize the keyboard with default info
    public override init(frame: CGRect) {
        super.init(frame: frame)
        isMultipleTouchEnabled = true
    }

    /// Initialize the keyboard
    public init(width: Int, height: Int, firstOctave: Int = 4, octaveCount: Int = 3,
                polyphonic: Bool = true) {
        self.octaveCount = octaveCount
        self.firstOctave = firstOctave
        super.init(frame: CGRect(x: 0, y: 0, width: width, height: height))
        oneOctaveSize = CGSize(width: Double(width / octaveCount - width / (octaveCount * octaveCount * 7)),
                               height: Double(height))
        isMultipleTouchEnabled = true
        setNeedsDisplay()
    }

    /// Initialization within Interface Builder
    required public init?(coder aDecoder: NSCoder) {
        super.init(coder: aDecoder)
        isMultipleTouchEnabled = true
    }

    // MARK: - Storyboard Rendering

    /// Set up the view for rendering in Interface Builder
    override open func prepareForInterfaceBuilder() {
        super.prepareForInterfaceBuilder()

        let width = Int(self.frame.width)
        let height = Int(self.frame.height)
        oneOctaveSize = CGSize(width: Double(width / octaveCount - width / (octaveCount * octaveCount * 7)),
                               height: Double(height))

        contentMode = .redraw
        clipsToBounds = true
    }

    /// Keyboard view size
    override open var intrinsicContentSize: CGSize {
        return CGSize(width: 1_024, height: 84)
    }

    /// Require constraints
    open class override var requiresConstraintBasedLayout: Bool {
        return true
    }

    // MARK: - Drawing

    /// Draw the view
    override open func draw(_ rect: CGRect) {

        let width = Int(self.frame.width)
        let height = Int(self.frame.height)
        oneOctaveSize = CGSize(width: Double(width / octaveCount - width / (octaveCount * octaveCount * 7)),
                               height: Double(height))
        for i in 0 ..< octaveCount {
            drawOctaveCanvas(i)
        }
        let tempWidth = CGFloat(width) - CGFloat((octaveCount * 7) - 1) * whiteKeySize.width - 1
        let backgroundPath = UIBezierPath(rect: CGRect(x: oneOctaveSize.width * CGFloat(octaveCount),
                                                       y: 0,
                                                       width: tempWidth,
                                                       height: oneOctaveSize.height))
        UIColor.black.setFill()
        backgroundPath.fill()
        let lastCRect = CGRect(x: whiteKeyX(0, octaveNumber: octaveCount),
                               y: 1,
                               width: tempWidth / 2,
                               height: whiteKeySize.height)
        let lastC = UIBezierPath(roundedRect: lastCRect, byRoundingCorners: [.bottomLeft, .bottomRight],
                                 cornerRadii: CGSize(width: 5, height: 5))
        whiteKeyColor(0, octaveNumber: octaveCount).setFill()
        lastC.fill()
        addLabels(i: 0, octaveNumber: octaveCount, whiteKeysRect: lastCRect)
    }

    /// Draw one octave
    func drawOctaveCanvas(_ octaveNumber: Int) {
        let width = Int(self.frame.width)
        let height = Int(self.frame.height)
        oneOctaveSize = CGSize(width: Double(width / octaveCount - width / (octaveCount * octaveCount * 7)),
                               height: Double(height))

        //// background Drawing
        let backgroundPath = UIBezierPath(rect: CGRect(x: 0 + oneOctaveSize.width * CGFloat(octaveNumber),
                                                       y: 0,
                                                       width: oneOctaveSize.width,
                                                       height: oneOctaveSize.height))
        UIColor.black.setFill()
        backgroundPath.fill()
        var whiteKeysPaths = [UIBezierPath]()
        for i in 0 ..< 7 {
            let whiteKeysRect = CGRect(x: whiteKeyX(i, octaveNumber: octaveNumber),
                                       y: 1,
                                       width: whiteKeySize.width - 1,
                                       height: whiteKeySize.height)
            whiteKeysPaths.append(UIBezierPath(roundedRect: whiteKeysRect,
                                               byRoundingCorners: [.bottomLeft, .bottomRight],
                                               cornerRadii: CGSize(width: 5, height: 5)))
            whiteKeyColor(i, octaveNumber: octaveNumber).setFill()
            whiteKeysPaths[i].fill()
            addLabels(i: i, octaveNumber: octaveNumber, whiteKeysRect: whiteKeysRect)
        }

        var topKeyPaths = [UIBezierPath]()

        for i in 0 ..< 28 {
            let topKeysRect = CGRect(x: topKeyX(i, octaveNumber: octaveNumber),
                                     y: 1,
                                     width: topKeySize.width + topKeyWidthIncrease,
                                     height: topKeySize.height)
            topKeyPaths.append(UIBezierPath(roundedRect: topKeysRect,
                             byRoundingCorners: [.bottomLeft, .bottomRight],
                             cornerRadii: CGSize(width: 3, height: 3)))
            topKeyColor(i, octaveNumber: octaveNumber).setFill()
            topKeyPaths[i].fill()

            // Add fancy paintcode blackkey code
        }
    }

    func addLabels(i: Int, octaveNumber: Int, whiteKeysRect: CGRect) {
        var textColor: UIColor =  #colorLiteral(red: 0.5098039216, green: 0.5098039216, blue: 0.5294117647, alpha: 1)
        if darkMode {
            textColor = #colorLiteral(red: 0.3176470588, green: 0.337254902, blue: 0.3647058824, alpha: 1)
        }

        // labelMode == 1, Only C, labelMode == 2, All notes
        if labelMode == 1 && i == 0 || labelMode == 2 {
            // Add Label
            guard let context = UIGraphicsGetCurrentContext(),
                let font = UIFont(name: "AvenirNextCondensed-Regular", size: 14) else { return }
            let whiteKeysTextContent = getWhiteNoteName(i) + String(firstOctave + octaveNumber)
            let whiteKeysStyle = NSMutableParagraphStyle()
            whiteKeysStyle.alignment = .center
            let whiteKeysFontAttributes  = [
                NSAttributedString.Key.font: font,
                NSAttributedString.Key.foregroundColor: textColor,
                NSAttributedString.Key.paragraphStyle: whiteKeysStyle
                ] as [NSAttributedString.Key: Any]
            let whiteKeysTextHeight: CGFloat = whiteKeysTextContent.boundingRect(
                with: CGSize(width: whiteKeysRect.width,
                             height: CGFloat.infinity),
                options: .usesLineFragmentOrigin,
                attributes: whiteKeysFontAttributes,
                context: nil).height
            context.saveGState()
            context.clip(to: whiteKeysRect)

            // adjust for keyboard being hidden
            whiteKeysTextContent.draw(in: CGRect(x: whiteKeysRect.minX,
                                                 y: whiteKeysRect.minY + whiteKeysRect.height - whiteKeysTextHeight - 6,
                                                 width: whiteKeysRect.width,
                                                 height: whiteKeysTextHeight),
                                      withAttributes: whiteKeysFontAttributes)
            context.restoreGState()
        }
    }

    // MARK: - Touch Handling

    func notesFromTouches(_ touches: Set<UITouch>) -> [MIDINoteNumber] {
        var notes = [MIDINoteNumber]()
        for touch in touches {
            if let note = noteFromTouchLocation(touch.location(in: self)) {
                notes.append(note)
            }
        }
        return notes
    }

    func noteFromTouchLocation(_ location: CGPoint ) -> MIDINoteNumber? {
        guard bounds.contains(location) else {
            return nil
        }
        let x = location.x - xOffset
        let y = location.y
        var note = 0
        if y > oneOctaveSize.height * topKeyHeightRatio {
            let octNum = Int(x / oneOctaveSize.width)
            let scaledX = x - CGFloat(octNum) * oneOctaveSize.width
            note = (firstOctave + octNum) * 12 + whiteKeyNotes[max(0, Int(scaledX / whiteKeySize.width))] + baseMIDINote
        } else {
            let octNum = Int(x / oneOctaveSize.width)
            let scaledX = x - CGFloat(octNum) * oneOctaveSize.width
            note = (firstOctave + octNum) * 12 + topKeyNotes[max(0, Int(scaledX / topKeySize.width))] + baseMIDINote
        }
        if note >= 0 {
            return MIDINoteNumber(note)
        } else {
            return nil
        }
    }

    /// Handle new touches
    override open func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        let notes = notesFromTouches(touches)
        for note in notes {
            pressAdded(note)
        }
        if !holdMode { verifyTouches(event?.allTouches) }
        setNeedsDisplay()
    }

    /// Handle touches completed
    override open func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard !holdMode else { return }
        for touch in touches {
            if let note = noteFromTouchLocation(touch.location(in: self)) {
                // verify that there isn't still a touch remaining on same key from another finger
                if var otherTouches = event?.allTouches {
                    otherTouches.remove(touch)
                    if !notesFromTouches(otherTouches).contains(note) {
                        pressRemoved(note, touches: event?.allTouches)
                    }
                }
            }
        }
        verifyTouches(event?.allTouches)
        setNeedsDisplay()
    }

    /// Handle moved touches
    override open func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard !holdMode else { return }
        for touch in touches {
            if let key = noteFromTouchLocation(touch.location(in: self)),
                key != noteFromTouchLocation(touch.previousLocation(in: self)) {
                pressAdded(key)
                setNeedsDisplay()
            }
        }
        verifyTouches(event?.allTouches)
    }

    /// Handle stopped touches
    override open func touchesCancelled(_ touches: Set<UITouch>?, with event: UIEvent?) {
        verifyTouches(event?.allTouches)
    }

    // MARK: - Executing Key Presses

    func pressAdded(_ newNote: MIDINoteNumber, velocity: MIDIVelocity = 127) {
        var noteIsAlreadyOn = false
        if holdMode {
            for key in onKeys where key == newNote {
               noteIsAlreadyOn = true
                pressRemoved(key)
            }
        }
        if !polyphonicMode {
            for key in onKeys where key != newNote {
                pressRemoved(key)
            }
        }
        if !onKeys.contains(newNote) && !noteIsAlreadyOn {
            onKeys.insert(newNote)
            delegate?.noteOn(note: newNote, velocity: velocity)
        }
        setNeedsDisplay()
    }

    func pressRemoved(_ note: MIDINoteNumber, touches: Set<UITouch>? = nil, isFromMIDI: Bool = false) {
        guard onKeys.contains(note) else {
            return
        }
        onKeys.remove(note)
        if !isFromMIDI {
            delegate?.noteOff(note: note)
        }
        if !polyphonicMode {
            // in mono mode, replace with note from highest remaining touch, if it exists
            var remainingNotes = notesFromTouches(touches ?? Set<UITouch>())
            remainingNotes = remainingNotes.filter { $0 != note }
            if let highest = remainingNotes.max() {
                pressAdded(highest)
            }
        }
        setNeedsDisplay()
    }

    func allNotesOff() {
        for note in onKeys {
            delegate?.noteOff(note: note)
        }
        onKeys.removeAll()
        setNeedsDisplay()
    }

    private func verifyTouches(_ touches: Set<UITouch>?) {

        // check that current touches conforms to onKeys, remove stuck notes
        let notes = notesFromTouches(touches ?? Set<UITouch>() )
        let disjunct = onKeys.subtracting(notes)
        if !disjunct.isEmpty {
            for note in disjunct {
                pressRemoved(note)
            }
        }
    }

    // MARK: - Private helper properties and functions

    var whiteKeySize: CGSize {
        return CGSize(width: oneOctaveSize.width / 7.0, height: oneOctaveSize.height - 2)
    }

    var topKeySize: CGSize {
        return CGSize(width: oneOctaveSize.width / (4 * 7), height: oneOctaveSize.height * topKeyHeightRatio)
    }

    // swiftlint:disable identifier_name:min_length
    func whiteKeyX(_ n: Int, octaveNumber: Int) -> CGFloat {
        return CGFloat(n) * whiteKeySize.width + xOffset + oneOctaveSize.width * CGFloat(octaveNumber)
    }

    func topKeyX(_ n: Int, octaveNumber: Int) -> CGFloat {
        return CGFloat(n) * topKeySize.width - (topKeyWidthIncrease / 2) + xOffset +
            oneOctaveSize.width * CGFloat(octaveNumber)
    }

    func whiteKeyColor(_ n: Int, octaveNumber: Int) -> UIColor {
        if darkMode {
            whiteKeyOff = #colorLiteral(red: 0.1333333333, green: 0.1333333333, blue: 0.1333333333, alpha: 1)
            keyOnColor = #colorLiteral(red: 0.2941176471, green: 0.2941176471, blue: 0.3137254902, alpha: 1)
        } else {
            whiteKeyOff = #colorLiteral(red: 1.0, green: 1.0, blue: 1.0, alpha: 1.0)
            keyOnColor = keyOnUserColor
        }
        return onKeys.contains(
            MIDINoteNumber((firstOctave + octaveNumber) * 12 + whiteKeyNotes[n] + baseMIDINote ))  ?
                keyOnColor : whiteKeyOff
    }

    func topKeyColor(_ n: Int, octaveNumber: Int) -> UIColor {
        if darkMode {
            blackKeyOff = #colorLiteral(red: 0.2352941176, green: 0.2352941176, blue: 0.2549019608, alpha: 1)
            keyOnColor = #colorLiteral(red: 0.9607843137, green: 0.5098039216, blue: 0, alpha: 1)
        } else {
            blackKeyOff = #colorLiteral(red: 0.09411764706, green: 0.09411764706, blue: 0.09411764706, alpha: 1)
            keyOnColor = #colorLiteral(red: 0.9607843137, green: 0.5098039216, blue: 0, alpha: 1)
        }
        if notesWithSharps[topKeyNotes[n]].range(of: "#") != nil {
            return onKeys.contains(
                MIDINoteNumber((firstOctave + octaveNumber) * 12 + topKeyNotes[n] + baseMIDINote)
                )  ? keyOnColor : blackKeyOff
        }
        return #colorLiteral(red: 1.000, green: 1.000, blue: 1.000, alpha: 0.000)
    }
}
