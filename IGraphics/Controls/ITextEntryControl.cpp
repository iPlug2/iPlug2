#include "ITextEntryControl.h"
#include "IPlugPlatform.h"

#ifdef OS_WEB
// from <emscripten/key_codes.h>
#define VK_CANCEL              0x03
#define VK_HELP                0x06
#define VK_BACK                0x08
#define VK_TAB                 0x09
#define VK_CLEAR               0x0C
#define VK_RETURN              0x0D
#define VK_ENTER               0x0E
#define VK_SHIFT               0x10
#define VK_CONTROL             0x11
#define VK_ALT                 0x12
#define VK_PAUSE               0x13
#define VK_CAPS_LOCK           0x14
#define VK_KANA                0x15
#define VK_HANGUL              0x15
#define VK_EISU                0x16
#define VK_JUNJA               0x17
#define VK_FINAL               0x18
#define VK_HANJA               0x19
#define VK_KANJI               0x19
#define VK_ESCAPE              0x1B
#define VK_CONVERT             0x1C
#define VK_NONCONVERT          0x1D
#define VK_ACCEPT              0x1E
#define VK_MODECHANGE          0x1F
#define VK_SPACE               0x20
#define VK_PAGE_UP             0x21
#define VK_PAGE_DOWN           0x22
#define VK_END                 0x23
#define VK_HOME                0x24
#define VK_LEFT                0x25
#define VK_UP                  0x26
#define VK_RIGHT               0x27
#define VK_DOWN                0x28
#define VK_SELECT              0x29
#define VK_PRINT               0x2A
#define VK_EXECUTE             0x2B
#define VK_PRINTSCREEN         0x2C
#define VK_INSERT              0x2D
#define VK_DELETE              0x2E
#define VK_0                   0x30
#define VK_1                   0x31
#define VK_2                   0x32
#define VK_3                   0x33
#define VK_4                   0x34
#define VK_5                   0x35
#define VK_6                   0x36
#define VK_7                   0x37
#define VK_8                   0x38
#define VK_9                   0x39
#define VK_COLON               0x3A
#define VK_SEMICOLON           0x3B
#define VK_LESS_THAN           0x3C
#define VK_EQUALS              0x3D
#define VK_GREATER_THAN        0x3E
#define VK_QUESTION_MARK       0x3F
#define VK_AT                  0x40
#define VK_A                   0x41
#define VK_B                   0x42
#define VK_C                   0x43
#define VK_D                   0x44
#define VK_E                   0x45
#define VK_F                   0x46
#define VK_G                   0x47
#define VK_H                   0x48
#define VK_I                   0x49
#define VK_J                   0x4A
#define VK_K                   0x4B
#define VK_L                   0x4C
#define VK_M                   0x4D
#define VK_N                   0x4E
#define VK_O                   0x4F
#define VK_P                   0x50
#define VK_Q                   0x51
#define VK_R                   0x52
#define VK_S                   0x53
#define VK_T                   0x54
#define VK_U                   0x55
#define VK_V                   0x56
#define VK_W                   0x57
#define VK_X                   0x58
#define VK_Y                   0x59
#define VK_Z                   0x5A
#define VK_WIN                 0x5B
#define VK_CONTEXT_MENU        0x5D
#define VK_SLEEP               0x5F
#define VK_NUMPAD0             0x60
#define VK_NUMPAD1             0x61
#define VK_NUMPAD2             0x62
#define VK_NUMPAD3             0x63
#define VK_NUMPAD4             0x64
#define VK_NUMPAD5             0x65
#define VK_NUMPAD6             0x66
#define VK_NUMPAD7             0x67
#define VK_NUMPAD8             0x68
#define VK_NUMPAD9             0x69
#define VK_MULTIPLY            0x6A
#define VK_ADD                 0x6B
#define VK_SEPARATOR           0x6C
#define VK_SUBTRACT            0x6D
#define VK_DECIMAL             0x6E
#define VK_DIVIDE              0x6F
#define VK_F1                  0x70
#define VK_F2                  0x71
#define VK_F3                  0x72
#define VK_F4                  0x73
#define VK_F5                  0x74
#define VK_F6                  0x75
#define VK_F7                  0x76
#define VK_F8                  0x77
#define VK_F9                  0x78
#define VK_F10                 0x79
#define VK_F11                 0x7A
#define VK_F12                 0x7B
#define VK_F13                 0x7C
#define VK_F14                 0x7D
#define VK_F15                 0x7E
#define VK_F16                 0x7F
#define VK_F17                 0x80
#define VK_F18                 0x81
#define VK_F19                 0x82
#define VK_F20                 0x83
#define VK_F21                 0x84
#define VK_F22                 0x85
#define VK_F23                 0x86
#define VK_F24                 0x87
#define VK_NUM_LOCK            0x90
#define VK_SCROLL_LOCK         0x91
#define VK_WIN_OEM_FJ_JISHO    0x92
#define VK_WIN_OEM_FJ_MASSHOU  0x93
#define VK_WIN_OEM_FJ_TOUROKU  0x94
#define VK_WIN_OEM_FJ_LOYA     0x95
#define VK_WIN_OEM_FJ_ROYA     0x96
#define VK_CIRCUMFLEX          0xA0
#define VK_EXCLAMATION         0xA1
#define VK_DOUBLE_QUOTE        0xA3
#define VK_HASH                0xA3
#define VK_DOLLAR              0xA4
#define VK_PERCENT             0xA5
#define VK_AMPERSAND           0xA6
#define VK_UNDERSCORE          0xA7
#define VK_OPEN_PAREN          0xA8
#define VK_CLOSE_PAREN         0xA9
#define VK_ASTERISK            0xAA
#define VK_PLUS                0xAB
#define VK_PIPE                0xAC
#define VK_HYPHEN_MINUS        0xAD
#define VK_OPEN_CURLY_BRACKET  0xAE
#define VK_CLOSE_CURLY_BRACKET 0xAF
#define VK_TILDE               0xB0
#define VK_VOLUME_MUTE         0xB5
#define VK_VOLUME_DOWN         0xB6
#define VK_VOLUME_UP           0xB7
#define VK_COMMA               0xBC
#define VK_PERIOD              0xBE
#define VK_SLASH               0xBF
#define VK_BACK_QUOTE          0xC0
#define VK_OPEN_BRACKET        0xDB
#define VK_BACK_SLASH          0xDC
#define VK_CLOSE_BRACKET       0xDD
#define VK_QUOTE               0xDE
#define VK_META                0xE0
#define VK_ALTGR               0xE1
#define VK_WIN_ICO_HELP        0xE3
#define VK_WIN_ICO_00          0xE4
#define VK_WIN_ICO_CLEAR       0xE6
#define VK_WIN_OEM_RESET       0xE9
#define VK_WIN_OEM_JUMP        0xEA
#define VK_WIN_OEM_PA1         0xEB
#define VK_WIN_OEM_PA2         0xEC
#define VK_WIN_OEM_PA3         0xED
#define VK_WIN_OEM_WSCTRL      0xEE
#define VK_WIN_OEM_CUSEL       0xEF
#define VK_WIN_OEM_ATTN        0xF0
#define VK_WIN_OEM_FINISH      0xF1
#define VK_WIN_OEM_COPY        0xF2
#define VK_WIN_OEM_AUTO        0xF3
#define VK_WIN_OEM_ENLW        0xF4
#define VK_WIN_OEM_BACKTAB     0xF5
#define VK_ATTN                0xF6
#define VK_CRSEL               0xF7
#define VK_EXSEL               0xF8
#define VK_EREOF               0xF9
#define VK_PLAY                0xFA
#define VK_ZOOM                0xFB
#define VK_PA1                 0xFD
#define VK_WIN_OEM_CLEAR       0xFE
#endif

#define VIRTUAL_KEY_BIT 0x80000000
#define STB_TEXTEDIT_K_SHIFT 0x40000000
#define STB_TEXTEDIT_K_CONTROL 0x20000000
#define STB_TEXTEDIT_K_ALT 0x10000000
// key-bindings
#define STB_TEXTEDIT_K_LEFT (VIRTUAL_KEY_BIT | VK_LEFT)
#define STB_TEXTEDIT_K_RIGHT (VIRTUAL_KEY_BIT | VK_RIGHT)
#define STB_TEXTEDIT_K_UP (VIRTUAL_KEY_BIT | VK_UP)
#define STB_TEXTEDIT_K_DOWN (VIRTUAL_KEY_BIT | VK_DOWN)
#define STB_TEXTEDIT_K_LINESTART (VIRTUAL_KEY_BIT | VK_HOME)
#define STB_TEXTEDIT_K_LINEEND (VIRTUAL_KEY_BIT | VK_END)
#define STB_TEXTEDIT_K_WORDLEFT (STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_WORDRIGHT (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTSTART (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTEND (STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_DELETE (VIRTUAL_KEY_BIT | VK_DELETE)
#define STB_TEXTEDIT_K_BACKSPACE (VIRTUAL_KEY_BIT | VK_BACK)
#define STB_TEXTEDIT_K_UNDO (STB_TEXTEDIT_K_CONTROL | 'z')
#define STB_TEXTEDIT_K_REDO (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_SHIFT | 'z')
#define STB_TEXTEDIT_K_INSERT (VIRTUAL_KEY_BIT | VK_INSERT)
#define STB_TEXTEDIT_K_PGUP (VIRTUAL_KEY_BIT | VK_PAGEUP)
#define STB_TEXTEDIT_K_PGDOWN (VIRTUAL_KEY_BIT | VK_PAGEDOWN)
// functions
#define STB_TEXTEDIT_STRINGLEN(tc) ITextEntryControl::GetLength (tc)
#define STB_TEXTEDIT_LAYOUTROW ITextEntryControl::Layout
#define STB_TEXTEDIT_GETWIDTH(tc, n, i) ITextEntryControl::GetCharWidth (tc, n, i)
#define STB_TEXTEDIT_KEYTOTEXT(key)                                                                \
((key & VIRTUAL_KEY_BIT) ? 0 : ((key & STB_TEXTEDIT_K_CONTROL) ? 0 : (key & (~0xF0000000))));
#define STB_TEXTEDIT_GETCHAR(tc, i) ITextEntryControl::GetChar (tc, i)
#define STB_TEXTEDIT_NEWLINE '\n'
//#define STB_TEXTEDIT_IS_SPACE(ch) isSpace (ch)
#define STB_TEXTEDIT_DELETECHARS ITextEntryControl::DeleteChars
#define STB_TEXTEDIT_INSERTCHARS ITextEntryControl::InsertChars

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"

ITextEntryControl::ITextEntryControl(IGEditorDelegate& dlg)
: IControl(dlg, IRECT())
{
  stb_textedit_initialize_state(&mEditState, true);
  
  SetActionFunction([&](IControl* pCaller) {
    
    mDrawCursor = true;
    
    SetAnimation([&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();
      
      if(progress > 0.5) {
        mDrawCursor = false;
        pCaller->SetDirty(false);
      }
   
      if(progress > 1.) {
        pCaller->OnEndAnimation();
        return;
      }
      
    },
    500);
  });
}

void ITextEntryControl::Draw(IGraphics& g)
{
//  if(mDrawCursor)
  g.FillRect(mText.mTextEntryBGColor, mRECT);
  g.DrawText(mText, mEditString.Get(), mRECT);
}

template<typename Proc>
bool ITextEntryControl::CallSTB(Proc proc)
{
  auto oldState = mEditState;
  proc();
  
  if(memcmp (&oldState, &mEditState, sizeof (STB_TexteditState)) != 0)
  {
    OnStateChanged(); //TODO:
    return true;
  }
 
  return false;
}

void ITextEntryControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mod.L)
  {
    CallSTB ([&]() {
      stb_textedit_click(this, &mEditState, x, y);
    });
    
    SetDirty(true);
  }
}

bool ITextEntryControl::OnKeyDown(float x, float y, int key)
{
//  if (mRecursiveKeyGuard)
//    return -1;
//
//  mRecursiveKeyGuard = !mRecursiveKeyGuard;
  
//  if (callback->platformOnKeyDown (code))
//    return 1;
//
//  if (code.character == 0 && code.virt == 0)
//    return -1;
  
//  if (code.modifier == MODIFIER_CONTROL)
//  {
//    switch (code.character)
//    {
//      case 'a':
//      {
//        selectAll ();
//        return 1;
//      }
//      case 'x':
//      {
//        if (doCut ())
//          return 1;
//        return -1;
//      }
//      case 'c':
//      {
//        if (doCopy ())
//          return 1;
//        return -1;
//      }
//      case 'v':
//      {
//        if (doPaste ())
//          return 1;
//        return -1;
//      }
//    }
//  }
  
//  auto key = code.character;
//  if (key)
//  {
//    if (auto text = getFrame ()->getPlatformFrame ()->convertCurrentKeyEventToText ())
//    {
//      if (text->length () != 1)
//        return -1;
//      key = text->getString ()[0];
//    }
//  }
//  if (code.virt)
//  {
//    switch (code.virt)
//    {
//      case VK_SPACE:
//      {
//        key = 0x20;
//        break;
//      }
//      case VK_TAB:
//      {
//        return -1;
//      }
//      default:
//      {
//        key = code.virt | VIRTUAL_KEY_BIT;
//        break;
//      }
//    }
//  }
//  if (code.modifier & MODIFIER_CONTROL)
//    key |= STB_TEXTEDIT_K_CONTROL;
//  if (code.modifier & MODIFIER_ALTERNATE)
//    key |= STB_TEXTEDIT_K_ALT;
//  if (code.modifier & MODIFIER_SHIFT)
//    key |= STB_TEXTEDIT_K_SHIFT;
  return CallSTB([&]() { stb_textedit_key(this, &mEditState, key); }) ? 1 : -1;
}

//  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
//  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
//  void OnMouseOut() override;
//  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
void ITextEntryControl::OnEndAnimation()
{
  if(mEditing)
    SetDirty(true);
}

//static
int ITextEntryControl::DeleteChars(ITextEntryControl* _this, size_t pos, size_t num)
{
  _this->mEditString.DeleteSub((int) pos, (int) num);
  
  return true; // TODO: Error checking
}

//static
int ITextEntryControl::InsertChars(ITextEntryControl* _this, size_t pos, const char* text, size_t num)
{
  WDL_String str = WDL_String(text, (int) num);
  _this->mEditString.Insert(&str, (int) pos);
  return true; // TODO: Error checking
}

//static
char ITextEntryControl::GetChar(ITextEntryControl* _this, int pos)
{
  return _this->mEditString.Get()[pos];
}

//static
int ITextEntryControl::GetLength(ITextEntryControl* _this)
{
  return _this->mEditString.GetLength();
}

//static
void ITextEntryControl::Layout(StbTexteditRow* row, ITextEntryControl* _this, int start_i)
{
  assert (start_i == 0);

  _this->FillCharWidthCache();
  float textWidth = 0.;
  
  for (int i = 0; i < _this->mCharWidths.GetSize(); i++) {
    textWidth += _this->mCharWidths.Get()[i];
  }

  row->num_chars = _this->mEditString.GetLength();
  row->baseline_y_delta = 1.25;
  row->ymin = 0.f;
  row->ymax = static_cast<float> (_this->GetText().mSize);

  switch (_this->GetText().mAlign)
  {
    case IText::kAlignNear:
    {
      row->x0 = static_cast<float> (_this->GetRECT().L); // TODO: inset?
      row->x1 = row->x0 + textWidth;
      break;
    }
    case IText::kAlignCenter:
    {
      row->x0 =
      static_cast<float> ((_this->GetRECT().W() / 2.) - (textWidth / 2.));
      row->x1 = row->x0 + textWidth;
      break;
    }
    default:
    {
      assert(true);// not implemented
      break;
    }
  }
}

//static
float ITextEntryControl::GetCharWidth(ITextEntryControl* _this, int n, int i)
{
  _this->FillCharWidthCache();
  return _this->mCharWidths.Get()[i];
}

void ITextEntryControl::OnStateChanged()
{
  SetDirty(false);
}

void ITextEntryControl::OnTextChange()
{
}

void ITextEntryControl::FillCharWidthCache()
{
}
void ITextEntryControl::CalcCursorSizes()
{
}

float ITextEntryControl::GetCharWidth(char c, char pc)
{
  IRECT bounds;

  if (pc)
  {
    WDL_String pcstr; pcstr.SetFormatted(1, "%c", pc);
    WDL_String cstr; cstr.SetFormatted(1, "%c", c);

    GetUI()->MeasureText(mText, pcstr.Get(), bounds);
    float pcWidth = bounds.W();
    pcstr.Append(&cstr);
    GetUI()->MeasureText(mText, pcstr.Get(), bounds);
    float tcWidth = bounds.W();
    return tcWidth - pcWidth;
  }
  
  WDL_String cstr; cstr.SetFormatted(1, "%c", c);
  GetUI()->MeasureText(mText, cstr.Get(), bounds);
  return bounds.W();
}

void ITextEntryControl::CreateTextEntry(const IRECT& bounds, const IText& text, const char* str)
{
  SetTargetAndDrawRECTs(bounds);
  SetText(text);
  mEditString.Set(str);
  SetDirty(false);
  mEditing = true;
}
