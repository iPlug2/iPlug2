// used by plug-ins to access imported localization
// usage: 
// #define LOCALIZE_IMPORT_PREFIX "midi_" (should be the directory name with _)
// #include "../localize-import.h"
// ...
// IMPORT_LOCALIZE_VST(hostcb) 
// ...or
// IMPORT_LOCALIZE_RPLUG(rec)
//
#ifdef _REAPER_LOCALIZE_H_
#error you must include localize-import.h before localize.h, sorry
#endif

#include "../WDL/localize/localize-import.h"

#ifndef _REAPER_LOCALIZE_IMPORT_H_
#define _REAPER_LOCALIZE_IMPORT_H_

void (*vac_createGroupsFromTab)(const unsigned short *);

#define IMPORT_LOCALIZE_VST(hostcb) \
    import_vst_reaper_api((void **)&vac_createGroupsFromTab, "vac_createGroupsFromTab", hostcb); \
    import_vst_reaper_api((void **)&importedLocalizeFunc, "__localizeFunc", hostcb); \
    import_vst_reaper_api((void **)&importedLocalizeMenu, "__localizeMenu", hostcb); \
    import_vst_reaper_api((void **)&importedLocalizeInitializeDialog, "__localizeInitializeDialog", hostcb); \
    import_vst_reaper_api((void **)&importedLocalizePrepareDialog, "__localizePrepareDialog", hostcb);

#define IMPORT_LOCALIZE_RPLUG(rec) \
  *(void **)&vac_createGroupsFromTab = rec->GetFunc("vac_createGroupsFromTab"); \
  *(void **)&importedLocalizeFunc = rec->GetFunc("__localizeFunc"); \
  *(void **)&importedLocalizeMenu = rec->GetFunc("__localizeMenu"); \
  *(void **)&importedLocalizeInitializeDialog = rec->GetFunc("__localizeInitializeDialog"); \
  *(void **)&importedLocalizePrepareDialog = rec->GetFunc("__localizePrepareDialog");

#endif
