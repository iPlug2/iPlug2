#include "ITextEntryControl.h"
#include "IPlugPlatform.h"

enum VstVirtualKey
{
  VKEY_BACK = 1,
  VKEY_TAB,
  VKEY_CLEAR,
  VKEY_RETURN,
  VKEY_PAUSE,
  VKEY_ESCAPE,
  VKEY_SPACE,
  VKEY_NEXT,
  VKEY_END,
  VKEY_HOME,
  
  VKEY_LEFT,
  VKEY_UP,
  VKEY_RIGHT,
  VKEY_DOWN,
  VKEY_PAGEUP,
  VKEY_PAGEDOWN,
  
  VKEY_SELECT,
  VKEY_PRINT,
  VKEY_ENTER,
  VKEY_SNAPSHOT,
  VKEY_INSERT,
  VKEY_DELETE,
  VKEY_HELP,
  VKEY_NUMPAD0,
  VKEY_NUMPAD1,
  VKEY_NUMPAD2,
  VKEY_NUMPAD3,
  VKEY_NUMPAD4,
  VKEY_NUMPAD5,
  VKEY_NUMPAD6,
  VKEY_NUMPAD7,
  VKEY_NUMPAD8,
  VKEY_NUMPAD9,
  VKEY_MULTIPLY,
  VKEY_ADD,
  VKEY_SEPARATOR,
  VKEY_SUBTRACT,
  VKEY_DECIMAL,
  VKEY_DIVIDE,
  VKEY_F1,
  VKEY_F2,
  VKEY_F3,
  VKEY_F4,
  VKEY_F5,
  VKEY_F6,
  VKEY_F7,
  VKEY_F8,
  VKEY_F9,
  VKEY_F10,
  VKEY_F11,
  VKEY_F12,
  VKEY_NUMLOCK,
  VKEY_SCROLL,
  
  VKEY_SHIFT,
  VKEY_CONTROL,
  VKEY_ALT,
  
  VKEY_EQUALS
};

#define VIRTUAL_KEY_BIT 0x80000000
#define STB_TEXTEDIT_K_SHIFT 0x40000000
#define STB_TEXTEDIT_K_CONTROL 0x20000000
#define STB_TEXTEDIT_K_ALT 0x10000000
// key-bindings
#define STB_TEXTEDIT_K_LEFT (VIRTUAL_KEY_BIT | VKEY_LEFT)
#define STB_TEXTEDIT_K_RIGHT (VIRTUAL_KEY_BIT | VKEY_RIGHT)
#define STB_TEXTEDIT_K_UP (VIRTUAL_KEY_BIT | VKEY_UP)
#define STB_TEXTEDIT_K_DOWN (VIRTUAL_KEY_BIT | VKEY_DOWN)
#ifdef OS_MAC
#  define STB_TEXTEDIT_K_LINESTART (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_LEFT)
#  define STB_TEXTEDIT_K_LINEEND (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_RIGHT)
#  define STB_TEXTEDIT_K_WORDLEFT (STB_TEXTEDIT_K_ALT | STB_TEXTEDIT_K_LEFT)
#  define STB_TEXTEDIT_K_WORDRIGHT (STB_TEXTEDIT_K_ALT | STB_TEXTEDIT_K_RIGHT)
#  define STB_TEXTEDIT_K_TEXTSTART (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_UP)
#  define STB_TEXTEDIT_K_TEXTEND (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_DOWN)
#else
#  define STB_TEXTEDIT_K_LINESTART (VIRTUAL_KEY_BIT | VKEY_HOME)
#  define STB_TEXTEDIT_K_LINEEND (VIRTUAL_KEY_BIT | VKEY_END)
#  define STB_TEXTEDIT_K_WORDLEFT (STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_CONTROL)
#  define STB_TEXTEDIT_K_WORDRIGHT (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#  define STB_TEXTEDIT_K_TEXTSTART (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#  define STB_TEXTEDIT_K_TEXTEND (STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_CONTROL)
#endif
#define STB_TEXTEDIT_K_DELETE (VIRTUAL_KEY_BIT | VKEY_DELETE)
#define STB_TEXTEDIT_K_BACKSPACE (VIRTUAL_KEY_BIT | VKEY_BACK)
#define STB_TEXTEDIT_K_UNDO (STB_TEXTEDIT_K_CONTROL | 'z')
#define STB_TEXTEDIT_K_REDO (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_SHIFT | 'z')
#define STB_TEXTEDIT_K_INSERT (VIRTUAL_KEY_BIT | VKEY_INSERT)
#define STB_TEXTEDIT_K_PGUP (VIRTUAL_KEY_BIT | VKEY_PAGEUP)
#define STB_TEXTEDIT_K_PGDOWN (VIRTUAL_KEY_BIT | VKEY_PAGEDOWN)
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

ITextEntryControl::ITextEntryControl(IGEditorDelegate& dlg, const IRECT& bounds)
: IControl(dlg, bounds)
{
  stb_textedit_initialize_state(&mEditState, true);
}

void ITextEntryControl::Draw(IGraphics& g)
{
  g.FillRect(COLOR_RED, mRECT);
  g.DrawText(mText, mEditString.Get(), mRECT);
}

template<typename Proc>
bool ITextEntryControl::CallSTB(Proc proc)
{
  auto oldState = mEditState;
  proc();
  
  if(memcmp (&oldState, &mEditState, sizeof (STB_TexteditState)) != 0)
  {
    //    OnStateChanged(); //TODO:
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
  }
}

bool ITextEntryControl::OnKeyDown(float x, float y, int key)
{
  if (mRecursiveKeyGuard)
    return -1;

  mRecursiveKeyGuard = !mRecursiveKeyGuard;
  
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
//      case VKEY_SPACE:
//      {
//        key = 0x20;
//        break;
//      }
//      case VKEY_TAB:
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
//  void OnEndAnimation() override;

//static
int ITextEntryControl::DeleteChars(ITextEntryControl* _this, size_t pos, size_t num)
{
  _this->mEditString.DeleteSub((int) pos, (int) num);
  
  return true; // TODO: Error checking
}

//static
int ITextEntryControl::InsertChars(ITextEntryControl* _this, size_t pos, const char* text, size_t num)
{
  _this->mEditString.Insert(text, (int) pos); // TODO num?
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
