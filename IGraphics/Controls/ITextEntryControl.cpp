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
#include <string>
#include <codecvt>
#include <locale>

#ifdef _MSC_VER
#if (_MSC_VER >= 1900 /* VS 2015*/) && (_MSC_VER < 1920 /* pre VS 2019 */)
std::locale::id std::codecvt<char16_t, char, _Mbstatet>::id;
#endif
#endif

//TODO: use either wdlutf8, iplug2 UTF8/UTF16 or cpp11 wstring_convert
using StringConvert = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>;

using namespace iplug;
using namespace igraphics;

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
#define STB_TEXTEDIT_K_PGUP (VIRTUAL_KEY_BIT | kVK_PRIOR)
#define STB_TEXTEDIT_K_PGDOWN (VIRTUAL_KEY_BIT | kVK_NEXT)
// functions
#define STB_TEXTEDIT_STRINGLEN(tc) ITextEntryControl::GetLength (tc)
#define STB_TEXTEDIT_LAYOUTROW ITextEntryControl::Layout
#define STB_TEXTEDIT_GETWIDTH(tc, n, i) ITextEntryControl::GetCharWidth (tc, n, i)
#define STB_TEXTEDIT_KEYTOTEXT(key)                                                                \
((key & VIRTUAL_KEY_BIT) ? 0 : ((key & STB_TEXTEDIT_K_CONTROL) ? 0 : (key & (~0xF0000000))));
#define STB_TEXTEDIT_GETCHAR(tc, i) ITextEntryControl::GetChar (tc, i)
#define STB_TEXTEDIT_NEWLINE '\n'
#define STB_TEXTEDIT_IS_SPACE(ch) isspace(ch)
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

  StbTexteditRow row;
  Layout(&row, this, 0);

  const bool hasSelection = mEditState.select_start != mEditState.select_end;
  if (hasSelection)
  {
    float selectionStart = row.x0, selectionEnd = row.x0;
    const int start = std::min(mEditState.select_start, mEditState.select_end);
    const int end = std::max(mEditState.select_start, mEditState.select_end);
    for (int i = 0; i < mCharWidths.GetSize() && i < end; ++i)
    {
      if (i < start)
        selectionStart += mCharWidths.Get()[i];

      selectionEnd += mCharWidths.Get()[i];
    }
    IRECT selectionRect(selectionStart, mRECT.T + row.ymin, selectionEnd, mRECT.T + row.ymax);
    selectionRect = selectionRect.GetVPadded(-mText.mSize*0.1f);
    IBlend blend(EBlend::Default, 0.2f);
    g.FillRect(mText.mTextEntryFGColor, selectionRect, &blend);
  }

  g.DrawText(mText, StringConvert{}.to_bytes(mEditString).c_str(), mRECT);
  
  if (mDrawCursor && !hasSelection)
  {
    float cursorPos = row.x0;
    for (int i = 0; i < mCharWidths.GetSize() && i < mEditState.cursor; ++i)
    {
      cursorPos += mCharWidths.Get()[i];
    }
    IRECT cursorRect(roundf(cursorPos-1), mRECT.T + row.ymin, roundf(cursorPos), mRECT.T + row.ymax);
    cursorRect = cursorRect.GetVPadded(-mText.mSize*0.1f);
    g.FillRect(mText.mTextEntryFGColor, cursorRect);
  }
}

template<typename Proc>
bool ITextEntryControl::CallSTB(Proc proc)
{
  auto oldState = mEditState;
  proc();
  
  if(memcmp(&oldState, &mEditState, sizeof (STB_TexteditState)) != 0)
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
    
  if(mod.L)
  {
    CallSTB ([&]() {
      stb_textedit_click(this, &mEditState, x, y);
    });
  }
  
  if(mod.R)
  {
    static IPopupMenu menu {"", {"Cut", "Copy", "Paste"}, [&](IPopupMenu* pMenu) {
      switch (pMenu->GetChosenItemIdx()) {
        case 0: Cut(); break;
        case 1: CopySelection(); break;
        case 2: Paste(); break;
        default:
          break;
      }
    }
    };
    
    GetUI()->CreatePopupMenu(*this, menu, x, y);
  }
}

void ITextEntryControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  if (mod.L)
  {
    CallSTB([&]() {
      stb_textedit_drag(this, &mEditState, x, y);
    });
  }
}

void ITextEntryControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  SelectAll();
}

void ITextEntryControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if (mod.L)
  {
    CallSTB([&]() {
      stb_textedit_drag(this, &mEditState, x, y);
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
        SelectAll();
        return true;
      }
      case 'X':
      {
        Cut();
        return true;
      }
      case 'C':
      {
        CopySelection();
        return true;
      }
      case 'V':
      {
        Paste();
        return true;
      }
      case 'Z':
      {
        if (key.S)
          CallSTB([&]() { stb_textedit_key(this, &mEditState, STB_TEXTEDIT_K_REDO); });
        else
          CallSTB([&]() { stb_textedit_key(this, &mEditState, STB_TEXTEDIT_K_UNDO); });
        return true;
      }
    
      default:
        break;
    }
  }
  
  int stbKey;
  
  wdl_utf8_parsechar(key.utf8, &stbKey);
  
  switch (key.VK)
  {
    case kVK_SPACE: stbKey = ' '; break;
    case kVK_TAB: return false;
    case kVK_DELETE: stbKey = STB_TEXTEDIT_K_DELETE; break;
    case kVK_BACK: stbKey = STB_TEXTEDIT_K_BACKSPACE; break;
    case kVK_LEFT: stbKey = STB_TEXTEDIT_K_LEFT; break;
    case kVK_RIGHT: stbKey = STB_TEXTEDIT_K_RIGHT; break;
    case kVK_UP: stbKey = STB_TEXTEDIT_K_UP; break;
    case kVK_DOWN: stbKey = STB_TEXTEDIT_K_DOWN; break;
    case kVK_PRIOR: stbKey = STB_TEXTEDIT_K_PGUP; break;
    case kVK_NEXT: stbKey = STB_TEXTEDIT_K_PGDOWN; break;
    case kVK_HOME: stbKey = STB_TEXTEDIT_K_LINESTART; break;
    case kVK_END: stbKey = STB_TEXTEDIT_K_LINEEND; break;
    case kVK_RETURN: CommitEdit(); break;
    case kVK_ESCAPE: DismissEdit(); break;
    default:
    {
      // validate input based on param type
      IControl* pControlInTextEntry = GetUI()->GetControlInTextEntry();
      
      if(!pControlInTextEntry)
        return false;
      
      const IParam* pParam = pControlInTextEntry->GetParam();

      if(pParam)
      {
        switch (pParam->Type())
        {
          case IParam::kTypeEnum:
          case IParam::kTypeInt:
          case IParam::kTypeBool:
          {
            if (key.VK >= '0' && key.VK <= '9' && !key.S)
              break;
            if (key.VK >= kVK_NUMPAD0 && key.VK <= kVK_NUMPAD9)
              break;
            if (stbKey == '+' || stbKey == '-')
              break;
            stbKey = 0;
            break;
          }
          case IParam::kTypeDouble:
          {
            if (key.VK >= '0' && key.VK <= '9' && !key.S)
              break;
            if (key.VK >= kVK_NUMPAD0 && key.VK <= kVK_NUMPAD9)
              break;
            if (stbKey == '+' || stbKey == '-' || stbKey == '.')
              break;
            stbKey = 0;
            break;
          }
          default:
            break;
        }
      }

      if (stbKey == 0)
      {
        stbKey = (key.VK) | VIRTUAL_KEY_BIT;
      }
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

void ITextEntryControl::CopySelection()
{
  if (mEditState.select_start != mEditState.select_end)
  {
    const int start = std::min(mEditState.select_start, mEditState.select_end);
    const int end = std::max(mEditState.select_start, mEditState.select_end);
    GetUI()->SetTextInClipboard(StringConvert{}.to_bytes(mEditString.data() + start, mEditString.data() + end).c_str());
  }
}

void ITextEntryControl::Paste()
{
  WDL_String fromClipboard;
  if (GetUI()->GetTextFromClipboard(fromClipboard))
  {
    CallSTB([&] {
      auto uText = StringConvert{}.from_bytes (fromClipboard.Get(), fromClipboard.Get() + fromClipboard.GetLength());
      stb_textedit_paste (this, &mEditState, uText.data(), (int) uText.size());
    });
  }
}

void ITextEntryControl::Cut()
{
  CopySelection();
  CallSTB([&] {
    stb_textedit_cut(this, &mEditState);
  });
}

void ITextEntryControl::SelectAll()
{
  CallSTB([&]() {
    mEditState.select_start = 0;
    mEditState.select_end = static_cast<int>(mEditString.length());
  });
}

//static
int ITextEntryControl::DeleteChars(ITextEntryControl* _this, size_t pos, size_t num)
{
  _this->mEditString.erase(pos, num);
  _this->SetStr(StringConvert{}.to_bytes(_this->mEditString).c_str());
  _this->OnTextChange();
  return true; // TODO: Error checking
}

//static
int ITextEntryControl::InsertChars(ITextEntryControl* _this, size_t pos, const char16_t* text, size_t num)
{
  _this->mEditString.insert(pos, text, num);
  _this->SetStr(StringConvert{}.to_bytes(_this->mEditString).c_str());
  _this->OnTextChange();
  return true;
}

//static
char16_t ITextEntryControl::GetChar(ITextEntryControl* _this, int pos)
{
  return _this->mEditString[pos];
}

//static
int ITextEntryControl::GetLength(ITextEntryControl* _this)
{
  return static_cast<int>(_this->mEditString.size());
}

//static
void ITextEntryControl::Layout(StbTexteditRow* row, ITextEntryControl* _this, int start_i)
{
  assert (start_i == 0);

  _this->FillCharWidthCache();
  float textWidth = 0.;
  
  for (int i = 0; i < _this->mCharWidths.GetSize(); i++)
  {
    textWidth += _this->mCharWidths.Get()[i];
  }
  
  row->num_chars = GetLength(_this);
  row->baseline_y_delta = 1.25;

  switch (_this->GetText().mAlign)
  {
    case EAlign::Near:
    {
      row->x0 = _this->GetRECT().L;
      row->x1 = row->x0 + textWidth;
      break;
    }
    case EAlign::Center:
    {
      row->x0 = _this->GetRECT().MW() - (textWidth * 0.5f);
      row->x1 = row->x0 + textWidth;
      break;
    }
    case EAlign::Far:
    {
      row->x0 = _this->GetRECT().R - textWidth;
      row->x1 = row->x0 + textWidth;
    }
  }

  switch (_this->GetText().mVAlign)
  {
    case EVAlign::Top:
    {
      row->ymin = 0;
      break;
    }
    case EVAlign::Middle:
    {
      row->ymin = _this->GetRECT().H()*0.5f - _this->GetText().mSize * 0.5f;
      break;
    }
    case EVAlign::Bottom:
    {
      row->ymin = _this->GetRECT().H() - _this->GetText().mSize;
      break;
    }
  }

  row->ymax = row->ymin + static_cast<float> (_this->GetText().mSize);
}

//static
float ITextEntryControl::GetCharWidth(ITextEntryControl* _this, int n, int i)
{
  _this->FillCharWidthCache();
  return _this->mCharWidths.Get()[i]; // TODO: n not used?
}

void ITextEntryControl::OnStateChanged()
{
  SetDirty(false);
}

void ITextEntryControl::OnTextChange()
{
  mCharWidths.Resize(0, false);
  FillCharWidthCache();
}

void ITextEntryControl::FillCharWidthCache()
{
  // only calculate when empty
  if (mCharWidths.GetSize())
    return;

  const int len = static_cast<int>(mEditString.size());
  mCharWidths.Resize(len, false);
  for (int i = 0; i < len; ++i)
  {
    mCharWidths.Get()[i] = MeasureCharWidth(mEditString[i], i == 0 ? 0 : mEditString[i - 1]);
  }
}

void ITextEntryControl::CalcCursorSizes()
{
  //TODO: cache cursor size and location?
}

// the width of character 'c' should include the kerning between it and the next character,
// so that when adding up character widths in the cache we can get to the beginning of the visible glyph,
// which is important for cursor placement to look correct.
// stb_textedit behavior for clicking in the text field is to place the cursor in front of a character
// when the xpos is less than halfway into the width of the character and in front of the following character otherwise.
// see: https://github.com/nothings/stb/issues/6
float ITextEntryControl::MeasureCharWidth(char16_t c, char16_t nc)
{
  IRECT bounds;

  if (nc)
  {
    std::string str (StringConvert{}.to_bytes (nc));
    float ncWidth = GetUI()->MeasureText(mText, str.c_str(), bounds);
    str += StringConvert{}.to_bytes (c);
    float tcWidth = GetUI()->MeasureText(mText, str.c_str(), bounds);
    return tcWidth - ncWidth;
  }
  
  std::string str (StringConvert{}.to_bytes (c));
  return GetUI()->MeasureText(mText, str.c_str(), bounds);
}

void ITextEntryControl::CreateTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  SetTargetAndDrawRECTs(bounds);
  SetText(text);
  mText.mFGColor = mText.mTextEntryFGColor;
  SetStr(str);
  SelectAll();
  mEditState.cursor = 0;
  OnTextChange();
  SetDirty(true);
  mEditing = true;
}

void ITextEntryControl::DismissEdit()
{
  mEditing = false;
  SetTargetAndDrawRECTs(IRECT());
  GetUI()->mInTextEntry = nullptr;
  GetUI()->SetAllControlsDirty();
}

void ITextEntryControl::CommitEdit()
{
  mEditing = false;
  GetUI()->SetControlValueAfterTextEdit(StringConvert{}.to_bytes(mEditString).c_str());
  SetTargetAndDrawRECTs(IRECT());
  GetUI()->SetAllControlsDirty();
}

void ITextEntryControl::SetStr(const char* str)
{
  mCharWidths.Resize(0, false);
  mEditString = StringConvert{}.from_bytes(std::string(str));

  if (mEditState.select_start != mEditState.select_end)
  {
    SelectAll();
  }
}
