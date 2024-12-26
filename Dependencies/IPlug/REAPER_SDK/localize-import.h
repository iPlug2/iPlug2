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
    *(VstIntPtr *)&vac_createGroupsFromTab = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"vac_createGroupsFromTab",0.0); \
    *(VstIntPtr *)&importedLocalizeFunc = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizeFunc",0.0); \
    *(VstIntPtr *)&importedLocalizeMenu = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizeMenu",0.0); \
    *(VstIntPtr *)&importedLocalizeInitializeDialog = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizeInitializeDialog",0.0); \
    *(VstIntPtr *)&importedLocalizePrepareDialog = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizePrepareDialog",0.0);

#define IMPORT_LOCALIZE_RPLUG(rec) \
  *(void **)&vac_createGroupsFromTab = rec->GetFunc("vac_createGroupsFromTab"); \
  *(void **)&importedLocalizeFunc = rec->GetFunc("__localizeFunc"); \
  *(void **)&importedLocalizeMenu = rec->GetFunc("__localizeMenu"); \
  *(void **)&importedLocalizeInitializeDialog = rec->GetFunc("__localizeInitializeDialog"); \
  *(void **)&importedLocalizePrepareDialog = rec->GetFunc("__localizePrepareDialog");

#endif
