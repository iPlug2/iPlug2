/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * extend EEL_Editor with JSFX-specific stuff
 */
#ifndef __EEL_EDIT_H_
#define __EEL_EDIT_H_

#include "../WDL/win32_curses/eel_edit.h"

class SX_Editor : public EEL_Editor
{
  public:
    SX_Editor(SX_Instance *parentinst, void *cursesCtx) :
      EEL_Editor(cursesCtx), m_parent(parentinst)
    {
    }

    virtual int updateFile();  // overrides for recompile

    virtual int overrideSyntaxDrawingForLine(int *skipcnt, const char **p, int *c_comment_state, int *last_attr);
    virtual int namedTokenHighlight(const char *tokStart, int len, int state);

    virtual int GetTabCount();
    virtual WDL_CursesEditor *GetTab(int idx);
    virtual bool AddTab(const char *fn);
    virtual void SwitchTab(int idx, bool rel);
    virtual void CloseCurrentTab();

    virtual int is_code_start_line(const char *p);

    virtual void *peek_get_VM();
    virtual int peek_get_named_string_value(const char *name, char *sstr, size_t sstr_sz);
    virtual bool peek_get_numbered_string_value(double idx, char *sstr, size_t sstr_sz);
    virtual void peek_lock();
    virtual void peek_unlock();
    virtual void on_help(const char *str, int curChar); // curChar is current character if str is NULL
    virtual bool line_has_openable_file(const char *line, int cursor_pos, char *fnout, size_t fnout_sz);

    FILE *tryToFindOrCreateFile(const char *fnp, WDL_FastString *s); // called by line_has_openable_file

    virtual bool LineCanAffectOtherLines(const char *txt, int spos, int slen) // if multiline comment etc
    {
      return *txt == '@' || EEL_Editor::LineCanAffectOtherLines(txt,spos,slen);
    }

    virtual void get_extra_filepos_names(WDL_LogicalSortStringKeyedArray<int> * list, int pass);

    virtual int peek_get_token_info(const char *name, char *sstr, size_t sstr_sz, int chkmask, int ignoreline);
    virtual void get_suggested_token_names(const char *fname, int chkmask, suggested_matchlist *list);
    SX_Instance *m_parent;
};

#endif
