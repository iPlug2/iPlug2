#include "IPlugNoIGraphicsTest.h"
#include "IPlug_include_in_plug_src.h"
#include <algorithm> // For std::max

IPlugNoIGraphicsTest::IPlugNoIGraphicsTest(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  m_hWnd = nullptr;
  m_hParent = nullptr;
  m_screenScale = 1.0f;
  m_desiredWidth = PLUG_WIDTH;  // Default desired width
  m_desiredHeight = PLUG_HEIGHT; // Default desired height
  m_hFont = nullptr;
}

IPlugNoIGraphicsTest::~IPlugNoIGraphicsTest()
{
  // Clean up font
  if (m_hFont)
  {
    DeleteObject(m_hFont);
    m_hFont = nullptr;
  }
}

void IPlugNoIGraphicsTest::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}

void IPlugNoIGraphicsTest::UpdateFont()
{
  // Delete existing font if any
  if (m_hFont)
  {
    DeleteObject(m_hFont);
    m_hFont = nullptr;
  }
  
  // Base font size (in points)
  const int baseFontSize = 50;
  
  // Calculate scaled font size - multiply by 10 to convert to logical units
  // Windows font sizes are in logical units (1/10th of a point)
  int scaledFontSize = static_cast<int>(baseFontSize * m_screenScale);
  
  // Ensure minimum font size
  scaledFontSize = std::max(scaledFontSize, 8);
  
  // Create a new font with the scaled size
  m_hFont = CreateFontW(
    scaledFontSize,                // Height
    0,                             // Width (0 = auto)
    0,                             // Escapement
    0,                             // Orientation
    FW_BOLD,                       // Weight - make it bold for better visibility
    FALSE,                         // Italic
    FALSE,                         // Underline
    FALSE,                         // StrikeOut
    DEFAULT_CHARSET,               // CharSet
    OUT_DEFAULT_PRECIS,            // OutPrecision
    CLIP_DEFAULT_PRECIS,           // ClipPrecision
    CLEARTYPE_QUALITY,             // Quality - use ClearType for better rendering
    DEFAULT_PITCH | FF_DONTCARE,   // PitchAndFamily
    L"Segoe UI"                    // Face name (modern Windows font)
  );
  
  // If window exists, set the font
  if (m_hWnd && m_hFont)
  {
    // Send the font to the window
    SendMessage(m_hWnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
    
    // Force a redraw to see the new font
    InvalidateRect(m_hWnd, nullptr, TRUE);
    UpdateWindow(m_hWnd);
  }
}

// Window procedure for our custom window
LRESULT CALLBACK IPlugNoIGraphicsTest::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      
      // Fill the background with a light gray color
      HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
      FillRect(hdc, &ps.rcPaint, hBrush);
      DeleteObject(hBrush);
      
      // Draw some text
      SetTextColor(hdc, RGB(255, 255, 255)); // White text for better contrast on red background
      SetBkMode(hdc, TRANSPARENT);
      
      // Get the font from the window
      HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
      HFONT hOldFont = nullptr;
      
      if (hFont)
      {
        hOldFont = (HFONT)SelectObject(hdc, hFont);
      }
      else
      {
        // If no font is set, create a temporary one
        LOGFONTW lf = {0};
        lf.lfHeight = 50; // Large size for visibility
        lf.lfWeight = FW_BOLD;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        
        HFONT hTempFont = CreateFontIndirectW(&lf);
        if (hTempFont)
        {
          hOldFont = (HFONT)SelectObject(hdc, hTempFont);
          DeleteObject(hTempFont); // We'll delete it after use
        }
      }
      
      RECT rect;
      GetClientRect(hWnd, &rect);
      
      // Draw text with a larger format flag for better visibility
      DrawTextW(hdc, L"IPlugNoIGraphicsTest Plugin", -1, &rect, 
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
      
      // Restore the old font
      if (hOldFont)
      {
        SelectObject(hdc, hOldFont);
      }
      
      EndPaint(hWnd, &ps);
      return 0;
    }
    
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  
  return DefWindowProc(hWnd, message, wParam, lParam);
}

void IPlugNoIGraphicsTest::SetScreenScale(float scale)
{
  if (m_screenScale != scale)
  {
    m_screenScale = scale;
    Trace(TRACELOC, "SetScreenScale: %f", scale);

    int windowWidth = GetEditorWidth() * scale;
    int windowHeight = GetEditorHeight() * scale;

    assert(windowWidth > 0 && windowHeight > 0 && "Window dimensions invalid");

    bool parentResized = EditorResizeFromUI(windowWidth, windowHeight, true);
    
    // Update font for the new scale
    UpdateFont();
    
    // If window is already created, update its size
    if (m_hWnd)
    {
      UpdateParentWindowSize();
    }
  }
}

void IPlugNoIGraphicsTest::SetParentWindowSize(int width, int height)
{
  if (m_desiredWidth != width || m_desiredHeight != height)
  {
    m_desiredWidth = width;
    m_desiredHeight = height;
    Trace(TRACELOC, "SetParentWindowSize: %d x %d", width, height);
    
    // If parent window is already set, update its size
    if (m_hParent)
    {
      UpdateParentWindowSize();
    }
  }
}

void IPlugNoIGraphicsTest::UpdateParentWindowSize()
{
  if (!m_hParent)
    return;
    
  // Get the current window style
  DWORD style = GetWindowLong(m_hParent, GWL_STYLE);
  DWORD exStyle = GetWindowLong(m_hParent, GWL_EXSTYLE);
  
  // Calculate the required window size including borders and title bar
  RECT rect = {0, 0, m_desiredWidth, m_desiredHeight};
  AdjustWindowRectEx(&rect, style, FALSE, exStyle);
  
  int windowWidth = rect.right - rect.left;
  int windowHeight = rect.bottom - rect.top;
  
  // Get the current position
  RECT currentRect;
  GetWindowRect(m_hParent, &currentRect);
  
  // Set the new size while maintaining the current position
  SetWindowPos(m_hParent, nullptr, 
               currentRect.left, currentRect.top, 
               windowWidth, windowHeight,
               SWP_NOZORDER | SWP_NOACTIVATE);
               
  // After parent window is resized, update our child window size
  if (m_hWnd)
  {
    UpdateWindowSize();
  }
}

void IPlugNoIGraphicsTest::UpdateWindowSize()
{
  if (!m_hWnd)
    return;
    
  // Calculate scaled dimensions directly from our desired dimensions
  int scaledWidth = static_cast<int>(m_desiredWidth * m_screenScale);
  int scaledHeight = static_cast<int>(m_desiredHeight * m_screenScale);
  
  // Set the window position and size
  SetWindowPos(m_hWnd, nullptr, 0, 0, scaledWidth, scaledHeight, 
               SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
               
  // Force a redraw
  InvalidateRect(m_hWnd, nullptr, TRUE);
  UpdateWindow(m_hWnd);
}

void* IPlugNoIGraphicsTest::OpenWindow(void* pParent)
{
  m_hParent = (HWND)pParent;
  
  // Register window class if not already registered
  static bool registered = false;
  if (!registered)
  {
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"IPlugNoIGraphicsTestClass";
    
    if (RegisterClassExW(&wcex))
    {
      registered = true;
    }
    else
    {
      return nullptr;
    }
  }

  SetScreenScale(GetScaleForHWND(m_hParent));
  
  // Update parent window size if needed
  if (m_hParent)
  {
    UpdateParentWindowSize();
  }
  
  // Calculate scaled dimensions directly from our desired dimensions
  // instead of using the parent's client area
  int scaledWidth = static_cast<int>(m_desiredWidth * m_screenScale);
  int scaledHeight = static_cast<int>(m_desiredHeight * m_screenScale);
  
  // Create child window
  m_hWnd = CreateWindowW(
    L"IPlugNoIGraphicsTestClass",
    L"IPlugNoIGraphicsTest",
    WS_CHILD | WS_VISIBLE,
    0, 0, scaledWidth, scaledHeight,
    m_hParent,
    nullptr,
    GetModuleHandle(nullptr),
    nullptr
  );
  
  if (m_hWnd)
  {
    // Create and set the initial font
    UpdateFont();
    
    // Show the window
    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);
    
    return m_hWnd;
  }
  
  return nullptr;
}