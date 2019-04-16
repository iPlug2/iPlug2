/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

/**
 * @file
 * @brief ITextEntryControl implementation
 * @ingroup SpecialControls
*/

#include "ITextEntryControl.h"
#include "IPlugPlatform.h"
#include "wdlutf8.h"

#define VIRTUAL_KEY_BIT 0x80000000
#define STB_TEXTEDIT_K_SHIFT 0x40000000
#define STB_TEXTEDIT_K_CONTROL 0x20000000
#define STB_TEXTEDIT_K_ALT 0x10000000
// key-bindings
#define STB_TEXTEDIT_K_LEFT (VIRTUAL_KEY_BIT | kVK_LEFT)
#define STB_TEXTEDIT_K_RIGHT (VIRTUAL_KEY_BIT | kVK_RIGHT)
#define STB_TEXTEDIT_K_UP (VIRTUAL_KEY_BIT | kVK_UP)
#define STB_TEXTEDIT_K_DOWN (VIRTUAL_KEY_BIT | kVK_DOWN)
#define STB_TEXTEDIT_K_LINESTART (VIRTUAL_KEY_BIT | kVK_HOME)
#define STB_TEXTEDIT_K_LINEEND (VIRTUAL_KEY_BIT | kVK_END)
#define STB_TEXTEDIT_K_WORDLEFT (STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_WORDRIGHT (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTSTART (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTEND (STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_DELETE (VIRTUAL_KEY_BIT | kVK_DELETE)
#define STB_TEXTEDIT_K_BACKSPACE (VIRTUAL_KEY_BIT | kVK_BACK)
#define STB_TEXTEDIT_K_UNDO (STB_TEXTEDIT_K_CONTROL | 'z')
#define STB_TEXTEDIT_K_REDO (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_SHIFT | 'z')
#define STB_TEXTEDIT_K_INSERT (VIRTUAL_KEY_BIT | kVK_INSERT)
#define STB_TEXTEDIT_K_PGUP (VIRTUAL_KEY_BIT | kVK_PAGEUP)
#define STB_TEXTEDIT_K_PGDOWN (VIRTUAL_KEY_BIT | kVK_PAGEDOWN)
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

ITextEntryControl::ITextEntryControl()
: IControl(IRECT())
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
    1000);
  });
}

void ITextEntryControl::Draw(IGraphics& g)
{
  g.FillRect(mText.mTextEntryBGColor, mRECT);
  g.DrawText(mText, mEditString.Get(), mRECT);
  
  //TODO: draw selection rect
  
  if(mDrawCursor)
    g.DrawVerticalLine(mText.mTextEntryFGColor, mRECT.GetVPadded(-2.f), 0.4f);
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
  if(!mRECT.Contains(x, y))
  {
    CommitEdit();
    return;
  }
    
  if (mod.L)
  {
    CallSTB ([&]() {
      stb_textedit_click(this, &mEditState, x - mRECT.L, y - mRECT.T);
    });
    
    SetDirty(true);
  }
}

bool ITextEntryControl::OnKeyDown(float x, float y, const IKeyPress& key)
{
  if (key.C)
  {
    switch (key.VK)
    {
      case 'A':
      {
        //TODO: Select All
        return true;
      }
      case 'X':
      {
        //TODO: Cut
        return false;
      }
      case 'C':
      {
        //TODO: Copy
        return false;
      }
      case 'V':
      {
        //TODO: Paste
        return false;
      }
    
      default:
        break;
    }
  }
  
  int stbKey;
  
  wdl_utf8_parsechar(key.utf8, &stbKey);
  
  switch (key.VK)
  {
    case kVK_SPACE: stbKey = kVK_SPACE; break;
    case kVK_TAB: return false;
    case kVK_DELETE: stbKey = kVK_DELETE; break;
    case kVK_BACK: stbKey = kVK_BACK; break;
    case kVK_RETURN: CommitEdit(); break;
    case kVK_ESCAPE: DismissEdit(); break;
    default:
    {
      if(key.VK >= '0' && key.VK <= '9')
        break;
      if(key.VK >= kVK_NUMPAD0 && key.VK <= kVK_NUMPAD9)
        break;
      if(key.VK >= 'A' && key.VK <= 'Z')
        break;
      else
      // TODO: need to shift correct bits for VK
//        stbKey = (key.VK) | VIRTUAL_KEY_BIT;
      break;
    }
  }

  if (key.C)
    stbKey |= STB_TEXTEDIT_K_CONTROL;
  if (key.A)
    stbKey |= STB_TEXTEDIT_K_ALT;
  if (key.S)
    stbKey |= STB_TEXTEDIT_K_SHIFT;
  
  return CallSTB([&]() { stb_textedit_key(this, &mEditState, stbKey); }) ? true : false;
}

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
      row->x0 = static_cast<float> ((_this->GetRECT().W() / 2.) - (textWidth / 2.));
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
  //TODO:
}

void ITextEntryControl::FillCharWidthCache()
{
  //TODO:
}

void ITextEntryControl::CalcCursorSizes()
{
  //TODO:
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

void ITextEntryControl::DismissEdit()
{
  mEditing = false;
  SetTargetAndDrawRECTs(IRECT());
  GetUI()->SetAllControlsDirty();
}

void ITextEntryControl::CommitEdit()
{
  mEditing = false;
  SetTargetAndDrawRECTs(IRECT());
  GetUI()->SetAllControlsDirty();
}
