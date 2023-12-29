
import SwiftUI
import Foundation

struct SearchBar: View {
  @Binding var text: String
  
  var body: some View {
    HStack {
      TextField("Search", text: $text)
        .padding(.horizontal, 25)
        .padding(.vertical, 5)
        .background(.bar)
        .overlay(
          HStack {
            Image(systemName: "magnifyingglass")
              .foregroundColor(.gray)
              .frame(minWidth: 0, maxWidth: .infinity, alignment: .leading)
              .padding(.leading, 8)
            
            if !text.isEmpty {
              Button(action: {
                self.text = ""
              }) {
                Image(systemName: "multiply.circle.fill")
                  .foregroundColor(.black)
              }
              .frame(minWidth: 25, maxWidth: 25, alignment: .trailing)
              .padding(.trailing, 4)
              .buttonStyle(.plain)
            }
          }
        )
    }
  }
}


class FileBrowserViewModel: ObservableObject {
  @Published var files: [String] = []
  @Published var searchText = ""
  @Published var selectedFile: String?
  
  var filteredFiles: [String] {
    if searchText.isEmpty {
      return files
    } else {
      return files.filter { $0.localizedCaseInsensitiveContains(searchText) }
    }
  }
  
  func loadFiles(from folderPath: String) {
    let fileManager = FileManager.default
    guard let fileURLs = try? fileManager.contentsOfDirectory(atPath: folderPath) else { return }
    files = fileURLs
  }
}

struct FileBrowserView: View {
  @ObservedObject var viewModel = FileBrowserViewModel()
  let folderPath: String
  var onFileSelected: (String) -> Void  // Closure for handling file selection
  
  init(folderPath: String, onFileSelected: @escaping (String) -> Void) {
    self.folderPath = folderPath
    self.onFileSelected = onFileSelected
    viewModel.loadFiles(from: folderPath)
  }
  
  var body: some View {
    VStack {
      SearchBar(text: $viewModel.searchText)
        List(viewModel.filteredFiles, id: \.self) { file in
          HStack {
              Text(file)
              Spacer()
          }
          .padding(5)
          .contentShape(Rectangle())
          .background(viewModel.selectedFile == file ? Color.blue.opacity(0.3) : Color.clear)
          .onTapGesture {
              viewModel.selectedFile = file
              let selectedFilePath = "\(folderPath)/\(file)"
              self.onFileSelected(selectedFilePath)
          }

        }
    }
  }
}

#Preview {
  FileBrowserView(folderPath: "/Users/oli/Dev/iPlug2/Examples/IPlugEmbedPlatformView", onFileSelected: {selectedFilePath in
  })
}
