/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#include "IGraphicsWeb.h"

// Helper to create WebGL context for Shadow DOM (CSS selectors don't work)
EM_JS(int, createWebGLContextForShadowDOM, (), {
  var canvas = Module.canvas;
  if (!canvas) return 0;
  var attrs = { stencil: true, depth: true, antialias: true, alpha: true };
  var ctx = canvas.getContext("webgl", attrs) || canvas.getContext("experimental-webgl", attrs);
  if (!ctx) return 0;
  return GL.registerContext(ctx, attrs);
});

EM_JS(void, iplug_popup_menu_show_js, (void* pGraphics, double viewportX, double viewportY, const char* menuJson), {
  var rootItems;
  var emitSelection = function(pG, path) {
    if (!pG || !Module._iplug_popup_menu_selected) {
      return;
    }

    if (Module.ccall) {
      Module.ccall('iplug_popup_menu_selected', null, ['number', 'string'], [pG, path || ""]);
    } else {
      Module._iplug_popup_menu_selected(pG, 0);
    }
  };

  try {
    rootItems = JSON.parse(UTF8ToString(menuJson));
  } catch (e) {
    console.error('iPlug popup menu: invalid menu payload', e);
    emitSelection(pGraphics, "");
    return;
  }

  if (!HTMLElement.prototype.showPopover || !HTMLElement.prototype.hidePopover) {
    emitSelection(pGraphics, "");
    return;
  }

  var doc = document;
  if (!doc.getElementById('__iplug_popup_menu_style')) {
    var style = doc.createElement('style');
    style.id = '__iplug_popup_menu_style';
    style.textContent = [
      '.iplug-popup-menu{box-sizing:border-box;position:fixed;inset:0;width:100vw;height:100vh;margin:0;padding:0;border:0;background:transparent;overflow:visible;pointer-events:none;color:var(--iplug-popup-menu-color,#ddd);font:var(--iplug-popup-menu-font,13px -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif);}',
      '.iplug-popup-menu::backdrop{background:transparent;}',
      '.iplug-popup-menu__panel{box-sizing:border-box;position:absolute;padding:var(--iplug-popup-menu-padding,4px 0);min-width:var(--iplug-popup-menu-min-width,160px);max-height:var(--iplug-popup-menu-max-height,70vh);overflow-y:auto;border:var(--iplug-popup-menu-border,1px solid #555);border-radius:var(--iplug-popup-menu-border-radius,6px);background:var(--iplug-popup-menu-background,#1e1e1e);color:inherit;box-shadow:var(--iplug-popup-menu-shadow,0 6px 18px rgba(0,0,0,.4));pointer-events:auto;}',
      '.iplug-popup-menu__item{display:flex;align-items:center;gap:.35em;width:100%;box-sizing:border-box;padding:var(--iplug-popup-menu-item-padding,5px 12px);border:0;background:transparent;color:inherit;text-align:left;font:inherit;white-space:nowrap;cursor:pointer;}',
      '.iplug-popup-menu__item:disabled{cursor:default;opacity:var(--iplug-popup-menu-disabled-opacity,.45);}',
      '.iplug-popup-menu__item:not(:disabled):hover,.iplug-popup-menu__item:not(:disabled):focus{background:var(--iplug-popup-menu-hover-background,#3b82f6);color:var(--iplug-popup-menu-hover-color,#fff);outline:0;}',
      '.iplug-popup-menu__check{display:inline-block;width:1.25em;flex:0 0 1.25em;}',
      '.iplug-popup-menu__submenu-indicator{margin-left:auto;padding-left:1.5em;}',
      '.iplug-popup-menu__title{padding:var(--iplug-popup-menu-title-padding,6px 12px 2px);color:var(--iplug-popup-menu-title-color,#888);font-weight:600;font-size:var(--iplug-popup-menu-title-font-size,11px);text-transform:uppercase;}',
      '.iplug-popup-menu__separator{height:1px;margin:var(--iplug-popup-menu-separator-margin,4px 8px);background:var(--iplug-popup-menu-separator-color,#444);}'
    ].join('\n');
    doc.head.appendChild(style);
  }

  var menu = doc.getElementById('__iplug_popup_menu');
  if (!menu) {
    menu = doc.createElement('div');
    menu.id = '__iplug_popup_menu';
    menu.className = 'iplug-popup-menu';
    menu.setAttribute('popover', 'manual');
    menu.setAttribute('role', 'menu');
    menu.setAttribute('tabindex', '-1');
    // Popovers are promoted to the top layer, so appending to body still
    // renders above canvases hosted inside Shadow DOM.
    doc.body.appendChild(menu);
  } else {
    if (menu._iplugCleanup) {
      menu._iplugCleanup(menu._iplugPGraphics && menu._iplugPGraphics !== pGraphics);
    }
    if (menu.matches && menu.matches(':popover-open') && menu.hidePopover) {
      menu.hidePopover();
    }
    while (menu.firstChild) {
      menu.removeChild(menu.firstChild);
    }
  }

  menu._iplugPickedPath = "";
  menu._iplugPGraphics = pGraphics;

  var clearPanelsFrom = function(depth) {
    Array.prototype.slice.call(menu.querySelectorAll('.iplug-popup-menu__panel')).forEach(function(panel) {
      if (Number(panel.dataset.depth) >= depth) {
        if (panel.dataset.path) {
          var trigger = menu.querySelector('.iplug-popup-menu__item[data-iplug-path="' + panel.dataset.path + '"]');
          if (trigger) {
            trigger.setAttribute('aria-expanded', 'false');
          }
        }
        panel.remove();
      }
    });
  };

  var getMenuItems = function(panel) {
    if (!panel) {
      return [];
    }
    return Array.prototype.slice.call(panel.querySelectorAll('.iplug-popup-menu__item:not(:disabled)'));
  };

  var focusButton = function(button) {
    if (!button) {
      return;
    }
    try {
      button.focus({ preventScroll: true });
    } catch (e) {
      button.focus();
    }
  };

  var focusMenuItem = function(panel, idx) {
    var items = getMenuItems(panel);
    if (!items.length) {
      return;
    }
    var wrappedIdx = (idx + items.length) % items.length;
    focusButton(items[wrappedIdx]);
  };

  var focusFirstMenuItem = function(panel) {
    focusMenuItem(panel, 0);
  };

  var getActivePanel = function() {
    var active = doc.activeElement;
    if (active && active.classList && active.classList.contains('iplug-popup-menu__item')) {
      return active.closest('.iplug-popup-menu__panel');
    }

    var panels = Array.prototype.slice.call(menu.querySelectorAll('.iplug-popup-menu__panel'));
    return panels.length ? panels[panels.length - 1] : null;
  };

  var getActiveButton = function() {
    var active = doc.activeElement;
    if (active && active.classList && active.classList.contains('iplug-popup-menu__item')) {
      return active;
    }
    return null;
  };

  var positionPanel = function(panel, x, y, fallbackRightEdge) {
    var margin = 4;
    panel.style.left = '0px';
    panel.style.top = '0px';
    panel.style.visibility = 'hidden';

    var rect = panel.getBoundingClientRect();
    var left = x;
    var top = y;

    if (left + rect.width + margin > window.innerWidth && fallbackRightEdge !== null) {
      left = fallbackRightEdge - rect.width;
    }

    left = Math.max(margin, Math.min(left, window.innerWidth - rect.width - margin));
    top = Math.max(margin, Math.min(top, window.innerHeight - rect.height - margin));

    panel.style.left = Math.round(left) + 'px';
    panel.style.top = Math.round(top) + 'px';
    panel.style.visibility = "";
  };

  var showPanel;
  var appendMenuItem = function(panel, item, idx, path, depth) {
    var childPath = path.concat([idx]);

    if (item.separator) {
      var separator = doc.createElement('div');
      separator.className = 'iplug-popup-menu__separator';
      separator.setAttribute('role', 'separator');
      separator.addEventListener('pointerenter', function() {
        clearPanelsFrom(depth + 1);
      });
      panel.appendChild(separator);
      return;
    }

    if (item.title) {
      var title = doc.createElement('div');
      title.className = 'iplug-popup-menu__title';
      title.setAttribute('role', 'presentation');
      title.textContent = item.text || "";
      title.addEventListener('pointerenter', function() {
        clearPanelsFrom(depth + 1);
      });
      panel.appendChild(title);
      return;
    }

    var button = doc.createElement('button');
    var hasSubmenu = Array.isArray(item.submenu) && item.submenu.length > 0;
    button.className = 'iplug-popup-menu__item';
    button.type = 'button';
    button.setAttribute('role', 'menuitem');
    button.dataset.iplugPath = childPath.join(',');
    button.disabled = !!item.disabled || (!hasSubmenu && !!item.submenu);
    if (item.checked) {
      button.setAttribute('role', 'menuitemcheckbox');
      button.setAttribute('aria-checked', 'true');
    }

    var check = doc.createElement('span');
    check.className = 'iplug-popup-menu__check';
    check.textContent = item.checked ? String.fromCharCode(0x2713) : "";
    button.appendChild(check);

    var label = doc.createElement('span');
    label.textContent = item.text || "";
    button.appendChild(label);

    if (hasSubmenu) {
      button.setAttribute('aria-haspopup', 'menu');
      button.setAttribute('aria-expanded', 'false');

      var submenuIndicator = doc.createElement('span');
      submenuIndicator.className = 'iplug-popup-menu__submenu-indicator';
      submenuIndicator.textContent = String.fromCharCode(0x203a);
      button.appendChild(submenuIndicator);

      var openSubmenu = function(focusFirst) {
        if (button.disabled) {
          return;
        }

        var rect = button.getBoundingClientRect();
        var childPanel = showPanel(item.submenu, childPath, depth + 1, rect.right - 1, rect.top, rect.left + 1);
        button.setAttribute('aria-expanded', 'true');
        if (focusFirst) {
          focusFirstMenuItem(childPanel);
        }
      };
      button._iplugOpenSubmenu = openSubmenu;

      button.addEventListener('pointerenter', function() {
        openSubmenu(false);
      });
      button.addEventListener('click', function(e) {
        e.preventDefault();
        openSubmenu(false);
      });
    } else {
      button.addEventListener('pointerenter', function() {
        clearPanelsFrom(depth + 1);
      });
      button.addEventListener('click', function() {
        if (button.disabled) {
          return;
        }
        menu._iplugPickedPath = childPath.join(',');
        menu.hidePopover();
      });
    }

    panel.appendChild(button);
  };

  showPanel = function(items, path, depth, x, y, fallbackRightEdge) {
    clearPanelsFrom(depth);

    var panel = doc.createElement('div');
    panel.className = 'iplug-popup-menu__panel';
    panel.dataset.depth = String(depth);
    panel.dataset.path = path.join(',');
    panel.setAttribute('role', 'menu');

    items.forEach(function(item, idx) {
      appendMenuItem(panel, item, idx, path, depth);
    });

    menu.appendChild(panel);
    positionPanel(panel, x, y, fallbackRightEdge);
    return panel;
  };

  var onDocPointerDown = function(e) {
    if (menu.contains(e.target)) {
      return;
    }
    menu.hidePopover();
  };

  var moveFocus = function(delta) {
    var panel = getActivePanel();
    var items = getMenuItems(panel);
    if (!items.length) {
      return;
    }

    var idx = items.indexOf(getActiveButton());
    if (idx < 0) {
      focusButton(items[delta > 0 ? 0 : items.length - 1]);
    } else {
      focusButton(items[(idx + delta + items.length) % items.length]);
    }
  };

  var closeActiveSubmenu = function() {
    var panel = getActivePanel();
    if (!panel) {
      return false;
    }

    var depth = Number(panel.dataset.depth);
    if (depth <= 0) {
      return false;
    }

    var path = panel.dataset.path || "";
    clearPanelsFrom(depth);
    focusButton(menu.querySelector('.iplug-popup-menu__item[data-iplug-path="' + path + '"]'));
    return true;
  };

  var onKeyDown = function(e) {
    var handled = true;
    var activeButton = getActiveButton();

    switch (e.key) {
      case 'Escape':
        menu.hidePopover();
        break;
      case 'ArrowDown':
        moveFocus(1);
        break;
      case 'ArrowUp':
        moveFocus(-1);
        break;
      case 'Home':
        focusMenuItem(getActivePanel(), 0);
        break;
      case 'End':
        var panel = getActivePanel();
        focusMenuItem(panel, getMenuItems(panel).length - 1);
        break;
      case 'ArrowRight':
        if (activeButton && activeButton._iplugOpenSubmenu) {
          activeButton._iplugOpenSubmenu(true);
        }
        break;
      case 'ArrowLeft':
        closeActiveSubmenu();
        break;
      case 'Enter':
      case ' ':
      case 'Spacebar':
        if (activeButton) {
          if (activeButton._iplugOpenSubmenu) {
            activeButton._iplugOpenSubmenu(true);
          } else {
            activeButton.click();
          }
        }
        break;
      default:
        handled = false;
        break;
    }

    if (handled) {
      e.preventDefault();
      e.stopPropagation();
    }
  };

  var finish = function(sendCallback) {
    menu.removeEventListener('toggle', onToggle);
    doc.removeEventListener('pointerdown', onDocPointerDown, true);
    doc.removeEventListener('keydown', onKeyDown, true);
    menu._iplugCleanup = null;

    var picked = menu._iplugPickedPath;
    var pG = menu._iplugPGraphics;
    menu._iplugPickedPath = "";
    menu._iplugPGraphics = null;

    if (sendCallback) {
      emitSelection(pG, picked);
    }
  };

  var onToggle = function(e) {
    if (e.newState === 'closed') {
      finish(true);
    }
  };

  menu._iplugCleanup = finish;
  menu.addEventListener('toggle', onToggle);

  setTimeout(function() {
    if (menu._iplugCleanup !== finish || menu._iplugPGraphics !== pGraphics) {
      return;
    }

    try {
      menu.showPopover();
    } catch (e) {
      console.warn('iPlug popup menu: showPopover failed', e);
      finish(true);
      return;
    }

    showPanel(rootItems, [], 0, viewportX, viewportY, null);

    try {
      menu.focus({ preventScroll: true });
    } catch (e) {
      menu.focus();
    }

    setTimeout(function() {
      if (menu._iplugCleanup === finish && menu._iplugPGraphics === pGraphics) {
        doc.addEventListener('pointerdown', onDocPointerDown, true);
        doc.addEventListener('keydown', onKeyDown, true);
      }
    }, 0);
  }, 0);
});

EM_JS(void, iplug_popup_menu_close_js, (void* pGraphics), {
  var menu = document.getElementById('__iplug_popup_menu');
  if (!menu || menu._iplugPGraphics !== pGraphics) {
    return;
  }

  menu._iplugPGraphics = null;
  menu._iplugPickedPath = "";
  if (menu.matches && menu.matches(':popover-open') && menu.hidePopover) {
    menu.hidePopover();
  } else if (menu._iplugCleanup) {
    menu._iplugCleanup(false);
  }
});


BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

void GetScreenDimensions(int& width, int& height)
{
  width = val::global("window")["innerWidth"].as<int>();
  height = val::global("window")["innerHeight"].as<int>();
}

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE

using namespace iplug;
using namespace igraphics;
using namespace emscripten;

extern std::vector<IGraphicsWeb*> gGraphicsInstances;
extern void UnregisterGraphicsInstance(IGraphicsWeb* pGraphics);
double gPrevMouseDownTime = 0.;
bool gFirstClick = false;

#pragma mark - Private Classes and Structs

// Fonts

class IGraphicsWeb::Font : public PlatformFont
{
public:
  Font(const char* fontName, const char* fontStyle)
  : PlatformFont(true), mDescriptor{fontName, fontStyle}
  {}
  
  FontDescriptor GetDescriptor() override { return &mDescriptor; }
  
private:
  std::pair<WDL_String, WDL_String> mDescriptor;
};

class IGraphicsWeb::FileFont : public Font
{
public:
  FileFont(const char* fontName, const char* fontStyle, const char* fontPath)
  : Font(fontName, fontStyle), mPath(fontPath)
  {
    mSystem = false;
  }
  
  IFontDataPtr GetFontData() override;
  
private:
  WDL_String mPath;
};

IFontDataPtr IGraphicsWeb::FileFont::GetFontData()
{
  IFontDataPtr fontData(new IFontData());
  FILE* fp = fopen(mPath.Get(), "rb");
  
  // Read in the font data.
  if (!fp)
    return fontData;
  
  fseek(fp,0,SEEK_END);
  fontData = std::make_unique<IFontData>((int) ftell(fp));
  
  if (!fontData->GetSize())
    return fontData;
  
  fseek(fp,0,SEEK_SET);
  size_t readSize = fread(fontData->Get(), 1, fontData->GetSize(), fp);
  fclose(fp);
  
  if (readSize && readSize == fontData->GetSize())
    fontData->SetFaceIdx(0);
  
  return fontData;
}

class IGraphicsWeb::MemoryFont : public Font
{
public:
  MemoryFont(const char* fontName, const char* fontStyle, const void* pData, int dataSize)
  : Font(fontName, fontStyle)
  {
    mSystem = false;
    mData.Set((const uint8_t*)pData, dataSize);
  }

  IFontDataPtr GetFontData() override
  {
    return IFontDataPtr(new IFontData(mData.Get(), mData.GetSize(), 0));
  }

private:
  WDL_TypedBuf<uint8_t> mData;
};

#pragma mark - Utilities and Callbacks

static EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphicsWeb = (IGraphicsWeb*) pUserData;

  int VK = DOMKeyToVirtualKey(pEvent->keyCode);
  WDL_String keyUTF8;

  // filter utf8 for non ascii keys
  if ((VK >= kVK_0 && VK <= kVK_Z) || VK == kVK_NONE)
    keyUTF8.Set(pEvent->key);
  else
    keyUTF8.Set("");

  IKeyPress keyPress {keyUTF8.Get(),
                      DOMKeyToVirtualKey(pEvent->keyCode),
                      static_cast<bool>(pEvent->shiftKey),
                      static_cast<bool>(pEvent->ctrlKey || pEvent->metaKey),
                      static_cast<bool>(pEvent->altKey)};
  
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_KEYDOWN:
    {
      return pGraphicsWeb->OnKeyDown(pGraphicsWeb->mPrevX, pGraphicsWeb->mPrevY, keyPress);
    }
    case EMSCRIPTEN_EVENT_KEYUP:
    {
      return pGraphicsWeb->OnKeyUp(pGraphicsWeb->mPrevX, pGraphicsWeb->mPrevY, keyPress);
    }
    default:
      break;
  }
  
  return 0;
}

static EM_BOOL outside_mouse_callback(int eventType, const EmscriptenMouseEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;

  IMouseInfo info;
  val rect = pGraphics->GetCanvas().call<val>("getBoundingClientRect");
  info.x = (pEvent->targetX - rect["left"].as<double>()) / pGraphics->GetDrawScale();
  info.y = (pEvent->targetY - rect["top"].as<double>()) / pGraphics->GetDrawScale();
  info.dX = pEvent->movementX;
  info.dY = pEvent->movementY;
  info.ms = {(pEvent->buttons & 1) != 0, (pEvent->buttons & 2) != 0, static_cast<bool>(pEvent->shiftKey), static_cast<bool>(pEvent->ctrlKey), static_cast<bool>(pEvent->altKey)};
  std::vector<IMouseInfo> list {info};
  
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
      // Get button states based on what caused the mouse up (nothing in buttons)
      list[0].ms.L = pEvent->button == 0;
      list[0].ms.R = pEvent->button == 2;
      pGraphics->OnMouseUp(list);
      emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, nullptr);
      emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
      if(pEvent->buttons != 0 && !pGraphics->IsInPlatformTextEntry())
        pGraphics->OnMouseDrag(list);
      break;
    }
    default:
      break;
  }
  
  pGraphics->mPrevX = info.x;
  pGraphics->mPrevY = info.y;
    
  return true;
}

static EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;
  
  IMouseInfo info;
  info.x = pEvent->targetX / pGraphics->GetDrawScale();
  info.y = pEvent->targetY / pGraphics->GetDrawScale();
  info.dX = pEvent->movementX;
  info.dY = pEvent->movementY;
  info.ms = {(pEvent->buttons & 1) != 0,
             (pEvent->buttons & 2) != 0,
             static_cast<bool>(pEvent->shiftKey),
             static_cast<bool>(pEvent->ctrlKey),
             static_cast<bool>(pEvent->altKey)};
  
  std::vector<IMouseInfo> list {info};
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
      const double timestamp = GetTimestamp();
      const double timeDiff = timestamp - gPrevMouseDownTime;
      
      if (gFirstClick && timeDiff < 0.3)
      {
        gFirstClick = false;
        pGraphics->OnMouseDblClick(info.x, info.y, info.ms);
      }
      else
      {
        gFirstClick = true;
        pGraphics->OnMouseDown(list);
      }
        
      gPrevMouseDownTime = timestamp;
      
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
      // Get button states based on what caused the mouse up (nothing in buttons)
      list[0].ms.L = pEvent->button == 0;
      list[0].ms.R = pEvent->button == 2;
      pGraphics->OnMouseUp(list);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
      gFirstClick = false;
      
      if(pEvent->buttons == 0)
        pGraphics->OnMouseOver(info.x, info.y, info.ms);
      else
      {
        if(!pGraphics->IsInPlatformTextEntry())
          pGraphics->OnMouseDrag(list);
      }
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEENTER:
      pGraphics->OnSetCursor();
      pGraphics->OnMouseOver(info.x, info.y, info.ms);
      emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, nullptr);
      break;
    case EMSCRIPTEN_EVENT_MOUSELEAVE:
      if(pEvent->buttons != 0)
      {
        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, outside_mouse_callback);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, outside_mouse_callback);
      }
      pGraphics->OnMouseOut(); break;
    default:
      break;
  }
  
  pGraphics->mPrevX = info.x;
  pGraphics->mPrevY = info.y;

  return true;
}

static EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent* pEvent, void* pUserData)
{
  IGraphics* pGraphics = (IGraphics*) pUserData;
  
  IMouseMod modifiers(false, false, pEvent->mouse.shiftKey, pEvent->mouse.ctrlKey, pEvent->mouse.altKey);
  
  double x = pEvent->mouse.targetX;
  double y = pEvent->mouse.targetY;
  
  x /= pGraphics->GetDrawScale();
  y /= pGraphics->GetDrawScale();
  
  switch (eventType) {
    case EMSCRIPTEN_EVENT_WHEEL: pGraphics->OnMouseWheel(x, y, modifiers, pEvent->deltaY);
    default:
      break;
  }
  
  return true;
}

EM_BOOL touch_callback(int eventType, const EmscriptenTouchEvent* pEvent, void* pUserData)
{
  IGraphics* pGraphics = (IGraphics*) pUserData;
  const float drawScale = pGraphics->GetDrawScale();

  std::vector<IMouseInfo> points;

  static EmscriptenTouchPoint previousTouches[32];
  
  for (auto i = 0; i < pEvent->numTouches; i++)
  {
    IMouseInfo info;
    info.x = pEvent->touches[i].targetX / drawScale;
    info.y = pEvent->touches[i].targetY / drawScale;
    info.dX = info.x - (previousTouches[i].targetX / drawScale);
    info.dY = info.y - (previousTouches[i].targetY / drawScale);
    info.ms = {true,
              false,
              static_cast<bool>(pEvent->shiftKey),
              static_cast<bool>(pEvent->ctrlKey),
              static_cast<bool>(pEvent->altKey),
              static_cast<ITouchID>(pEvent->touches[i].identifier)
    };
    
    if(pEvent->touches[i].isChanged)
      points.push_back(info);
  }

  memcpy(previousTouches, pEvent->touches, sizeof(previousTouches));
  
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_TOUCHSTART:
      pGraphics->OnMouseDown(points);
      return true;
    case EMSCRIPTEN_EVENT_TOUCHEND:
      pGraphics->OnMouseUp(points);
      return true;
    case EMSCRIPTEN_EVENT_TOUCHMOVE:
      pGraphics->OnMouseDrag(points);
      return true;
   case EMSCRIPTEN_EVENT_TOUCHCANCEL:
      pGraphics->OnTouchCancelled(points);
      return true;
    default:
      return false;
  }
}

static EM_BOOL complete_text_entry(int eventType, const EmscriptenFocusEvent* focusEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;
  
  val input = val::global("document").call<val>("getElementById", std::string("textEntry"));
  std::string str = input["value"].as<std::string>();
  val::global("document")["body"].call<void>("removeChild", input);
  pGraphics->SetControlValueAfterTextEdit(str.c_str());
  
  return true;
}

static EM_BOOL text_entry_keydown(int eventType, const EmscriptenKeyboardEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphicsWeb = (IGraphicsWeb*) pUserData;
  
  IKeyPress keyPress {pEvent->key, DOMKeyToVirtualKey(pEvent->keyCode),
    static_cast<bool>(pEvent->shiftKey),
    static_cast<bool>(pEvent->ctrlKey),
    static_cast<bool>(pEvent->altKey)};
  
  if (keyPress.VK == kVK_RETURN || keyPress.VK ==  kVK_TAB)
    return complete_text_entry(0, nullptr, pUserData);
  
  return false;
}

static EM_BOOL uievent_callback(int eventType, const EmscriptenUiEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;

  if (eventType == EMSCRIPTEN_EVENT_RESIZE)
  {
    pGraphics->GetDelegate()->OnParentWindowResize(pEvent->windowInnerWidth, pEvent->windowInnerHeight);

    return true;
  }
  
  return false;
}

IColorPickerHandlerFunc gColorPickerHandlerFunc = nullptr;

static void color_picker_callback(val e)
{
  if(gColorPickerHandlerFunc)
  {
    std::string colorStrHex = e["target"]["value"].as<std::string>();
    
    if (colorStrHex[0] == '#')
      colorStrHex = colorStrHex.erase(0, 1);
    
    IColor result;
    result.A = 255;
    sscanf(colorStrHex.c_str(), "%02x%02x%02x", &result.R, &result.G, &result.B);
    
    gColorPickerHandlerFunc(result);
  }
}

static void file_dialog_callback(val e)
{
  // DBGMSG(e["files"].as<std::string>().c_str());
}

EMSCRIPTEN_BINDINGS(events) {
  function("color_picker_callback", color_picker_callback);
  function("file_dialog_callback", file_dialog_callback);
}

#pragma mark -

IGraphicsWeb::IGraphicsWeb(IGEditorDelegate& dlg, int w, int h, int fps, float scale, val canvas)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  val keys = val::global("Object").call<val>("keys", GetPreloadedImages());

  DBGMSG("Preloaded %i images\n", keys["length"].as<int>());

  // Initialize canvas - use provided element, Module.canvas, or fall back to getElementById
  if (canvas.isUndefined() || canvas.isNull())
  {
    // Try Module.canvas first (set by web component or HTML template)
    val moduleCanvas = val::global("Module")["canvas"];
    if (!moduleCanvas.isUndefined() && !moduleCanvas.isNull())
    {
      mCanvas = moduleCanvas;
    }
    else
    {
      // Fall back to getElementById for legacy templates
      mCanvas = val::global("document").call<val>("getElementById", std::string("canvas"));
    }
  }
  else
  {
    mCanvas = canvas;
  }

  // Detect Shadow DOM by checking the canvas's root node
  mRootNode = mCanvas.call<val>("getRootNode");
  std::string rootNodeType = mRootNode["constructor"]["name"].as<std::string>();
  mInShadowDOM = (rootNodeType == "ShadowRoot");

  // Use the canvas's existing id if it has one (legacy templates rely on
  // querying their <canvas id="canvas"> from JS); otherwise synthesize a
  // per-instance id so multiple instances on a page don't collide.
  std::string existingId = mCanvas["id"].as<std::string>();
  if (existingId.empty())
  {
    char idBuf[64];
    snprintf(idBuf, sizeof(idBuf), "iplug-canvas-%p", static_cast<void*>(this));
    existingId = idBuf;
    mCanvas.set("id", existingId);
  }
  mCanvasSelector = std::string("#") + existingId;

  DBGMSG("IGraphicsWeb: Shadow DOM = %s, selector = %s\n", mInShadowDOM ? "true" : "false", mCanvasSelector.c_str());

  RegisterCanvasEvents();
}

void IGraphicsWeb::RegisterCanvasEvents()
{
  // Window-level events always work
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, key_callback);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, key_callback);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, uievent_callback);

  if (mInShadowDOM)
  {
    // Shadow DOM: emscripten's CSS selector-based callbacks don't work
    // Set up events via JavaScript on the canvas element directly
    EM_ASM({
      var pGraphics = $0;
      var canvas = Module.canvas;
      if (!canvas) return;

      // Store reference for cleanup
      canvas._iplugGraphics = pGraphics;

      canvas.addEventListener('mousedown', function(e) {
        Module._iGraphicsMouseCallback(pGraphics, 0, e.offsetX, e.offsetY, e.movementX, e.movementY, e.buttons, e.button, e.shiftKey, e.ctrlKey, e.altKey);
      });
      canvas.addEventListener('mouseup', function(e) {
        Module._iGraphicsMouseCallback(pGraphics, 1, e.offsetX, e.offsetY, e.movementX, e.movementY, e.buttons, e.button, e.shiftKey, e.ctrlKey, e.altKey);
      });
      canvas.addEventListener('mousemove', function(e) {
        Module._iGraphicsMouseCallback(pGraphics, 2, e.offsetX, e.offsetY, e.movementX, e.movementY, e.buttons, e.button, e.shiftKey, e.ctrlKey, e.altKey);
      });
      canvas.addEventListener('mouseenter', function(e) {
        Module._iGraphicsMouseCallback(pGraphics, 3, e.offsetX, e.offsetY, e.movementX, e.movementY, e.buttons, e.button, e.shiftKey, e.ctrlKey, e.altKey);
      });
      canvas.addEventListener('mouseleave', function(e) {
        Module._iGraphicsMouseCallback(pGraphics, 4, e.offsetX, e.offsetY, e.movementX, e.movementY, e.buttons, e.button, e.shiftKey, e.ctrlKey, e.altKey);
      });
      canvas.addEventListener('wheel', function(e) {
        Module._iGraphicsWheelCallback(pGraphics, e.offsetX, e.offsetY, e.deltaY, e.shiftKey, e.ctrlKey, e.altKey);
        e.preventDefault();
      }, { passive: false });
      // TODO: Touch events for Shadow DOM
    }, this);
  }
  else
  {
    // Regular DOM: use emscripten's callback system
    const char* target = mCanvasSelector.c_str();
    emscripten_set_mousedown_callback(target, this, 1, mouse_callback);
    emscripten_set_mouseup_callback(target, this, 1, mouse_callback);
    emscripten_set_mousemove_callback(target, this, 1, mouse_callback);
    emscripten_set_mouseenter_callback(target, this, 1, mouse_callback);
    emscripten_set_mouseleave_callback(target, this, 1, mouse_callback);
    emscripten_set_wheel_callback(target, this, 1, wheel_callback);
    emscripten_set_touchstart_callback(target, this, 1, touch_callback);
    emscripten_set_touchend_callback(target, this, 1, touch_callback);
    emscripten_set_touchmove_callback(target, this, 1, touch_callback);
    emscripten_set_touchcancel_callback(target, this, 1, touch_callback);
  }
}

void IGraphicsWeb::UnregisterCanvasEvents()
{
  const char* target = mCanvasSelector.c_str();

  emscripten_set_mousedown_callback(target, this, 1, nullptr);
  emscripten_set_mouseup_callback(target, this, 1, nullptr);
  emscripten_set_mousemove_callback(target, this, 1, nullptr);
  emscripten_set_mouseenter_callback(target, this, 1, nullptr);
  emscripten_set_mouseleave_callback(target, this, 1, nullptr);
  emscripten_set_wheel_callback(target, this, 1, nullptr);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, nullptr);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, nullptr);
  emscripten_set_touchstart_callback(target, this, 1, nullptr);
  emscripten_set_touchend_callback(target, this, 1, nullptr);
  emscripten_set_touchmove_callback(target, this, 1, nullptr);
  emscripten_set_touchcancel_callback(target, this, 1, nullptr);
}

IGraphicsWeb::~IGraphicsWeb()
{
  iplug_popup_menu_close_js(this);
  UnregisterCanvasEvents();
  UnregisterGraphicsInstance(this);
}

void* IGraphicsWeb::OpenWindow(void* pHandle)
{
#ifdef IGRAPHICS_GL
  EmscriptenWebGLContextAttributes attr;
  emscripten_webgl_init_context_attributes(&attr);
  attr.stencil = true;
  attr.depth = true;
//  attr.explicitSwapControl = 1;

  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

  if (mInShadowDOM)
  {
    // Shadow DOM: create context via JS since CSS selectors don't work
    ctx = createWebGLContextForShadowDOM();
  }
  else
  {
    // Regular DOM: use standard emscripten API
    ctx = emscripten_webgl_create_context(mCanvasSelector.c_str(), &attr);
  }

  emscripten_webgl_make_context_current(ctx);
#endif
  
  OnViewInitialized(nullptr /* not used */);

  SetScreenScale(std::ceil(std::max(emscripten_get_device_pixel_ratio(), 1.)));

  GetDelegate()->LayoutUI(this);
  GetDelegate()->OnUIOpen();
  
  return nullptr;
}

void IGraphicsWeb::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden == hide)
    return;

  if (hide)
  {
#ifdef IGRAPHICS_WEB_POINTERLOCK
    if (lock)
      emscripten_request_pointerlock(mCanvasSelector.c_str(), EM_FALSE);
    else
#endif
      mCanvas["style"].set("cursor", "none");

    mCursorHidden = true;
    mCursorLock = lock;
  }
  else
  {
#ifdef IGRAPHICS_WEB_POINTERLOCK
    if (mCursorLock)
      emscripten_exit_pointerlock();
    else
#endif
    OnSetCursor();

    mCursorHidden = false;
    mCursorLock = false;
  }
}

ECursor IGraphicsWeb::SetMouseCursor(ECursor cursorType)
{
  std::string cursor("pointer");

  switch (cursorType)
  {
    case ECursor::ARROW:            cursor = "default";         break;
    case ECursor::IBEAM:            cursor = "text";            break;
    case ECursor::WAIT:             cursor = "wait";            break;
    case ECursor::CROSS:            cursor = "crosshair";       break;
    case ECursor::UPARROW:          cursor = "n-resize";        break;
    case ECursor::SIZENWSE:         cursor = "nwse-resize";     break;
    case ECursor::SIZENESW:         cursor = "nesw-resize";     break;
    case ECursor::SIZEWE:           cursor = "ew-resize";       break;
    case ECursor::SIZENS:           cursor = "ns-resize";       break;
    case ECursor::SIZEALL:          cursor = "move";            break;
    case ECursor::INO:              cursor = "not-allowed";     break;
    case ECursor::HAND:             cursor = "pointer";         break;
    case ECursor::APPSTARTING:      cursor = "progress";        break;
    case ECursor::HELP:             cursor = "help";            break;
  }

  mCanvas["style"].set("cursor", cursor);
  return IGraphics::SetMouseCursor(cursorType);
}

void IGraphicsWeb::GetMouseLocation(float& x, float&y) const
{
  x = mPrevX;
  y = mPrevY;
}

//static
void IGraphicsWeb::OnMainLoopTimer()
{
  int screenScale = (int) std::ceil(std::max(emscripten_get_device_pixel_ratio(), 1.));

  // Iterate over all registered graphics instances
  for (IGraphicsWeb* pGraphics : gGraphicsInstances)
  {
    if (pGraphics == nullptr)
      continue;

    if (screenScale != pGraphics->GetScreenScale())
    {
      pGraphics->SetScreenScale(screenScale);
    }

    IRECTList rects;
    if (pGraphics->IsDirty(rects))
    {
      pGraphics->SetAllControlsClean();
      pGraphics->Draw(rects);
    }
  }
}

EMsgBoxResult IGraphicsWeb::ShowMessageBox(const char* str, const char* /*title*/, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
  ReleaseMouseCapture();
  
  EMsgBoxResult result = kNoResult;
  
  switch (type)
  {
    case kMB_OK:
    {
      val::global("window").call<val>("alert", std::string(str));
      result = EMsgBoxResult::kOK;
      break;
    }
    case kMB_YESNO:
    case kMB_OKCANCEL:
    {
      result = static_cast<EMsgBoxResult>(val::global("window").call<val>("confirm", std::string(str)).as<int>());
    }
    // case MB_CANCEL:
    //   break;
    default:
      return result = kNoResult;
  }
  
  if(completionHandler)
    completionHandler(result);
  
  return result;
}

void IGraphicsWeb::PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
{
  //TODO
  // val inputEl = val::global("document").call<val>("createElement", std::string("input"));
  
  // inputEl.call<void>("setAttribute", std::string("type"), std::string("file"));
  // inputEl.call<void>("setAttribute", std::string("accept"), std::string(ext));
  // inputEl.call<void>("click");
  // inputEl.call<void>("addEventListener", std::string("input"), val::module_property("file_dialog_callback"), false);
  // inputEl.call<void>("addEventListener", std::string("onChange"), val::module_property("file_dialog_callback"), false);
}

void IGraphicsWeb::PromptForDirectory(WDL_String& path, IFileDialogCompletionHandlerFunc completionHandler)
{
  //TODO
  // val inputEl = val::global("document").call<val>("createElement", std::string("input"));

  // inputEl.call<void>("setAttribute", std::string("type"), std::string("file"));
  // inputEl.call<void>("setAttribute", std::string("directory"), true);
  // inputEl.call<void>("setAttribute", std::string("webkitdirectory"), true);
  // inputEl.call<void>("click");
  // inputEl.call<void>("addEventListener", std::string("input"), val::module_property("file_dialog_callback"), false);
  // inputEl.call<void>("addEventListener", std::string("onChange"), val::module_property("file_dialog_callback"), false);
}

bool IGraphicsWeb::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  ReleaseMouseCapture();

  gColorPickerHandlerFunc = func;

  val inputEl = val::global("document").call<val>("createElement", std::string("input"));
  inputEl.call<void>("setAttribute", std::string("type"), std::string("color"));
  WDL_String colorStr;
  colorStr.SetFormatted(64, "#%02x%02x%02x", color.R, color.G, color.B);
  inputEl.call<void>("setAttribute", std::string("value"), std::string(colorStr.Get()));
  inputEl.call<void>("click");
  inputEl.call<void>("addEventListener", std::string("input"), val::module_property("color_picker_callback"), false);
  inputEl.call<void>("addEventListener", std::string("onChange"), val::module_property("color_picker_callback"), false);

  return false;
}

void IGraphicsWeb::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  val input = val::global("document").call<val>("createElement", std::string("input"));
  const val rect = mCanvas.call<val>("getBoundingClientRect");

  auto setDim = [&input](const char *dimName, double pixels)
  {
    WDL_String dimstr;
    dimstr.SetFormatted(32, "%fpx",  pixels);
    input["style"].set(dimName, std::string(dimstr.Get()));
  };
  
  auto setColor = [&input](const char *colorName, IColor color)
  {
    WDL_String str;
    str.SetFormatted(64, "rgba(%d, %d, %d, %d)", color.R, color.G, color.B, color.A);
    input["style"].set(colorName, std::string(str.Get()));
  };

  input.set("id", std::string("textEntry"));
  input["style"].set("position", val("fixed"));
  setDim("left", rect["left"].as<double>() + bounds.L);
  setDim("top", rect["top"].as<double>() + bounds.T);
  setDim("width", bounds.W());
  setDim("height", bounds.H());
  
  setColor("color", text.mTextEntryFGColor);
  setColor("background-color", text.mTextEntryBGColor);
  if (paramIdx > kNoParameter)
  {
    const IParam* pParam = GetDelegate()->GetParam(paramIdx);

    switch (pParam->Type())
    {
      case IParam::kTypeEnum:
      case IParam::kTypeInt:
      case IParam::kTypeBool:
        input.set("type", val("number")); // TODO
        break;
      case IParam::kTypeDouble:
        input.set("type", val("number"));
        break;
      default:
        break;
    }
  }
  else
  {
    input.set("type", val("text"));
  }

  // Append to shadow root or document.body based on mode
  if (mInShadowDOM)
  {
    mRootNode.call<void>("appendChild", input);
  }
  else
  {
    val::global("document")["body"].call<void>("appendChild", input);
  }

  input.call<void>("focus");
  emscripten_set_focusout_callback("textEntry", this, 1, complete_text_entry);
  emscripten_set_keydown_callback("textEntry", this, 1, text_entry_keydown);
}

namespace
{
  void AppendJsonString(std::string& out, const char* str)
  {
    out.push_back('"');

    if (!str)
    {
      out.push_back('"');
      return;
    }

    for (const char* p = str; *p; ++p)
    {
      const unsigned char c = static_cast<unsigned char>(*p);

      switch (c)
      {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
          if (c < 0x20)
          {
            char esc[8];
            std::snprintf(esc, sizeof(esc), "\\u%04x", c);
            out += esc;
          }
          else
          {
            out.push_back(static_cast<char>(c));
          }
          break;
      }
    }

    out.push_back('"');
  }

  std::string GetPopupMenuItemText(IPopupMenu& menu, int itemIdx, IPopupMenu::Item& item)
  {
    const char* itemText = item.GetText();

    if (!menu.GetPrefix() || item.GetIsSeparator())
      return itemText ? itemText : "";

    char prefix[16];
    switch (menu.GetPrefix())
    {
      case 1: std::snprintf(prefix, sizeof(prefix), "%1d: ", itemIdx + 1); break;
      case 2: std::snprintf(prefix, sizeof(prefix), "%02d: ", itemIdx + 1); break;
      case 3: std::snprintf(prefix, sizeof(prefix), "%03d: ", itemIdx + 1); break;
      default: prefix[0] = '\0'; break;
    }

    return std::string(prefix) + (itemText ? itemText : "");
  }

  void AppendPopupMenuJson(std::string& json, IPopupMenu& menu)
  {
    json.push_back('[');

    for (int i = 0; i < menu.NItems(); ++i)
    {
      IPopupMenu::Item* pItem = menu.GetItem(i);
      if (i > 0)
        json.push_back(',');

      json.push_back('{');
      json += "\"text\":";
      std::string itemText;

      if (pItem)
        itemText = GetPopupMenuItemText(menu, i, *pItem);

      AppendJsonString(json, itemText.c_str());

      if (pItem)
      {
        if (pItem->GetIsSeparator()) json += ",\"separator\":true";
        if (pItem->GetIsTitle()) json += ",\"title\":true";
        if (pItem->GetChecked()) json += ",\"checked\":true";
        if (!pItem->GetEnabled()) json += ",\"disabled\":true";

        if (IPopupMenu* pSubmenu = pItem->GetSubmenu())
        {
          json += ",\"submenu\":";
          AppendPopupMenuJson(json, *pSubmenu);
        }
      }

      json.push_back('}');
    }

    json.push_back(']');
  }

  bool ReadPopupMenuPathIndex(const char*& path, int& idx)
  {
    if (!path || *path < '0' || *path > '9')
      return false;

    idx = 0;
    while (*path >= '0' && *path <= '9')
    {
      idx = (idx * 10) + (*path - '0');
      ++path;
    }

    return true;
  }

  bool ResolvePopupMenuPath(IPopupMenu& rootMenu, const char* path, IPopupMenu*& pSelectedMenu, int& selectedIdx)
  {
    if (!path || !*path)
      return false;

    IPopupMenu* pMenu = &rootMenu;
    const char* p = path;

    while (*p)
    {
      int idx = -1;
      if (!ReadPopupMenuPathIndex(p, idx))
        return false;

      IPopupMenu::Item* pItem = pMenu->GetItem(idx);
      if (!pItem)
        return false;

      if (*p == ',')
      {
        ++p;
        pMenu = pItem->GetSubmenu();
        if (!pMenu || !*p)
          return false;

        continue;
      }

      if (*p != '\0')
        return false;

      pSelectedMenu = pMenu;
      selectedIdx = idx;
      return true;
    }

    return false;
  }
}

IPopupMenu* IGraphicsWeb::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync)
{
  isAsync = true;
  mCurrentPopupMenu = &menu;
  menu.SetChosenItemIdx(-1);

  std::string json;
  json.reserve(64 + (menu.NItems() * 48));
  AppendPopupMenuJson(json, menu);

  const val rect = mCanvas.call<val>("getBoundingClientRect");
  const double scale = static_cast<double>(GetDrawScale());
  const double viewportX = rect["left"].as<double>() + (bounds.L * scale);
  const double viewportY = rect["top"].as<double>() + (bounds.B * scale);
  iplug_popup_menu_show_js(this, viewportX, viewportY, json.c_str());

  return nullptr;
}

void IGraphicsWeb::OnPopupMenuSelectedAsync(const char* path)
{
  IPopupMenu* pRootMenu = mCurrentPopupMenu;
  mCurrentPopupMenu = nullptr;

  if (!pRootMenu)
    return;

  IPopupMenu* pMenu = nullptr;
  int idx = -1;
  if (!ResolvePopupMenuPath(*pRootMenu, path, pMenu, idx))
  {
    SetControlValueAfterPopupMenu(nullptr);
    return;
  }

  IPopupMenu::Item* pItem = pMenu->GetItem(idx);
  if (pItem && pItem->GetIsChoosable())
  {
    pMenu->SetChosenItemIdx(idx);

    if (pMenu->GetFunction())
      pMenu->ExecFunction();

    SetControlValueAfterPopupMenu(pMenu);
  }
  else
  {
    SetControlValueAfterPopupMenu(nullptr);
  }
}

extern "C" EMSCRIPTEN_KEEPALIVE
void iplug_popup_menu_selected(void* pGraphics, const char* path)
{
  if (pGraphics)
    static_cast<IGraphicsWeb*>(pGraphics)->OnPopupMenuSelectedAsync(path);
}

bool IGraphicsWeb::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  val::global("window").call<val>("open", std::string(url), std::string("_blank"));
  
  return true;
}

void IGraphicsWeb::DrawResize()
{
  // CSS style.width/height need "px" suffix
  std::string widthPx = std::to_string(static_cast<int>(Width() * GetDrawScale())) + "px";
  std::string heightPx = std::to_string(static_cast<int>(Height() * GetDrawScale())) + "px";
  mCanvas["style"].set("width", val(widthPx));
  mCanvas["style"].set("height", val(heightPx));

  // Canvas element width/height attributes are integers (no px).
  // Assigning these clears the WebGL drawing buffer even when the value
  // is unchanged, so guard against redundant writes — otherwise every
  // ResizeObserver tick during a drag causes a visible flash.
  const int newBufW = Width() * GetBackingPixelScale();
  const int newBufH = Height() * GetBackingPixelScale();
  const int curBufW = mCanvas["width"].as<int>();
  const int curBufH = mCanvas["height"].as<int>();
  if (newBufW != curBufW || newBufH != curBufH)
  {
    mCanvas.set("width", newBufW);
    mCanvas.set("height", newBufH);
  }

  IGRAPHICS_DRAW_CLASS::DrawResize();
}

void IGraphicsWeb::PostResize()
{
  // Called at the end of IGraphics::Resize(), after OnResize +
  // SetAllControlsDirty + DrawResize + (optional) LayoutUI have all
  // run. At this point control layout is final, so it's safe to
  // repaint synchronously. Without this, the canvas is stuck in a
  // cleared state (canvas.width/height assignment wiped the default
  // framebuffer; NanoVG's DrawResize rebuilt an empty FBO) until the
  // next main-loop RAF tick, which the browser may composite past —
  // producing a one-frame blank flash on every size change.
  IRECTList rects;
  rects.Add(GetBounds());
  SetAllControlsClean();
  Draw(rects);
}

PlatformFontPtr IGraphicsWeb::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), nullptr, nullptr);
  
  if (fontLocation == kNotFound)
    return nullptr;

  return PlatformFontPtr(new FileFont(fontID, "", fullPath.Get()));
}

PlatformFontPtr IGraphicsWeb::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  const char* styles[] = { "normal", "bold", "italic" };
  
  return PlatformFontPtr(new Font(fontName, styles[static_cast<int>(style)]));
}

PlatformFontPtr IGraphicsWeb::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return PlatformFontPtr(new MemoryFont(fontID, "", pData, dataSize));
}

// Shadow DOM event callback implementations (called from JavaScript)
extern "C" {

EMSCRIPTEN_KEEPALIVE
void iGraphicsMouseCallback(void* pGraphics, int eventType, double x, double y, double dx, double dy, int buttons, int button, int shift, int ctrl, int alt)
{
  IGraphicsWeb* pG = static_cast<IGraphicsWeb*>(pGraphics);
  float scale = pG->GetDrawScale();

  IMouseInfo info;
  info.x = x / scale;
  info.y = y / scale;
  info.dX = dx;
  info.dY = dy;
  info.ms = {(buttons & 1) != 0, (buttons & 2) != 0, static_cast<bool>(shift), static_cast<bool>(ctrl), static_cast<bool>(alt)};
  std::vector<IMouseInfo> list{info};

  switch (eventType)
  {
    case 0: // mousedown
      pG->OnMouseDown(list);
      break;
    case 1: // mouseup
      list[0].ms.L = button == 0;
      list[0].ms.R = button == 2;
      pG->OnMouseUp(list);
      break;
    case 2: // mousemove
      if (buttons == 0)
        pG->OnMouseOver(info.x, info.y, info.ms);
      else if (!pG->IsInPlatformTextEntry())
        pG->OnMouseDrag(list);
      break;
    case 3: // mouseenter
      pG->OnSetCursor();
      pG->OnMouseOver(info.x, info.y, info.ms);
      break;
    case 4: // mouseleave
      pG->OnMouseOut();
      break;
  }

  pG->mPrevX = info.x;
  pG->mPrevY = info.y;
}

EMSCRIPTEN_KEEPALIVE
void iGraphicsWheelCallback(void* pGraphics, double x, double y, double deltaY, int shift, int ctrl, int alt)
{
  IGraphicsWeb* pG = static_cast<IGraphicsWeb*>(pGraphics);
  float scale = pG->GetDrawScale();
  IMouseMod mod(false, false, static_cast<bool>(shift), static_cast<bool>(ctrl), static_cast<bool>(alt));
  pG->OnMouseWheel(x / scale, y / scale, mod, deltaY);
}

} // extern "C"

#if defined IGRAPHICS_NANOVG
#include "IGraphicsNanoVG.cpp"

#ifdef IGRAPHICS_FREETYPE
#define FONS_USE_FREETYPE
#endif

#include "nanovg.c"
#endif
