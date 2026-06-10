import SwiftUI

struct GridLabel {
    let text: String
    let position: LabelPosition
    
    enum LabelPosition {
        case left, right, top, bottom
    }
}

struct GridLinesView: View {
    let rows: Int
    let columns: Int
    let showOuterLines: Bool
    let opacity: Double

    var body: some View {
        GeometryReader { geometry in
            Path { path in
                let width = geometry.size.width
                let height = geometry.size.height
                let rowHeight = height / CGFloat(rows)
                let columnWidth = width / CGFloat(columns)

                // Draw horizontal lines
                for i in 0...rows {
                    if !showOuterLines && (i == 0 || i == rows) {
                        continue
                    }
                    let y = rowHeight * CGFloat(i)
                    path.move(to: CGPoint(x: 0, y: y))
                    path.addLine(to: CGPoint(x: width, y: y))
                }

                // Draw vertical lines
                for i in 0...columns {
                    if !showOuterLines && (i == 0 || i == columns) {
                        continue
                    }
                    let x = columnWidth * CGFloat(i)
                    path.move(to: CGPoint(x: x, y: 0))
                    path.addLine(to: CGPoint(x: x, y: height))
                }
            }
            .stroke(.white, lineWidth: 1)
            .opacity(opacity)
        }
    }
}

// New view for drawing labels.
struct GridLabelsView: View {
    let labels: [GridLabel]

    var body: some View {
        GeometryReader { geometry in
            ForEach(labels, id: \.text) { label in
                switch label.position {
                case .left:
                    Text(label.text)
                        .foregroundColor(.white)
                        .rotationEffect(.degrees(-90))
                        .position(x: -20, y: geometry.size.height / 2)
                case .right:
                    Text(label.text)
                        .foregroundColor(.white)
                        .rotationEffect(.degrees(90))
                        .position(x: geometry.size.width + 20, y: geometry.size.height / 2)
                case .top:
                    Text(label.text)
                        .foregroundColor(.white)
                        .position(x: geometry.size.width / 2, y: -20)
                case .bottom:
                    Text(label.text)
                        .foregroundColor(.white)
                        .position(x: geometry.size.width / 2, y: geometry.size.height + 20)
                }
            }
        }
    }
}

struct GridView: View {
    let rows: Int
    let columns: Int
    let showOuterLines: Bool
    let labels: [GridLabel]
    let opacity: Double
    
    init(
        rows: Int, 
        columns: Int, 
        showOuterLines: Bool = false, 
        labels: [GridLabel] = [], 
        opacity: Double = 1.0
    ) {
        self.rows = rows
        self.columns = columns
        self.showOuterLines = showOuterLines
        self.labels = labels
        self.opacity = opacity
    }
    
    var body: some View {
        ZStack {
            GridLinesView(rows: rows, columns: columns, showOuterLines: showOuterLines, opacity: opacity)
            GridLabelsView(labels: labels)
        }
        .padding(40)
    }
}
