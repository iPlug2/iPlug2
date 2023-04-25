SWELL_MyApp
Copyright (C) 2022 and onwards, Cockos Incorporated
LICENSE:

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

Minimal SWELL win32/linux/macOS GUI application starting point, feel free to base your applications on this.

NOTE: this project is not configured to be compiled in-place, you should move it to be alongside WDL, e.g.

cp -a ./WDL/swell/swell_myapp ./my_new_app


------

Previous sample project xcode instructions:

  How we created the xcode project that compiles this, too:

  New Project -> Mac OS X -> Application -> Cocoa Application

  Save as...

  Add to "Other sources/SWELL": (from swell path) swell-dlg.mm swell-gdi.mm swell-ini.cpp swell-kb.mm swell-menu.mm swell-misc.mm swell-miscdlg.mm swell-wnd.mm swell.cpp swell-appstub.mm swellappmain.mm

  Add app_main.cpp main_dialog.cpp to "Other sources"

  Go to Frameworks -> Linked Frameworks, add existing framework, Carbon.Framework

  go to terminal, to project dir, and run <pathtoswell>/swell_resgen.pl sample_project.rc

  Open mainmenu.xib in Interface Builder (doubleclick it in XCode)

    + Delete the default "Window"
    + File->Read class files, find and select "swellappmain.h"
    + Go to Library, Objects, scroll to "Object" , drag to "MainMenu.xib", rename to "Controller", then open its
      properties (Cmd+Shift+I, go to class identity tab), choose for Class "SWELLAppController".
    + Select "Application" in MainMenu.xib, go to (Cmd+Shift+I) application identity tab, select "SWELLApplication" for the class.

    + Customize the "NewApplication" menu.
       + Properties on "About NewApplication":
         + set the tag to 40002 (matching resource.h for about)
         + on the connection tab, "Sent Actions", remove the default connection, then drag a new connection to controller (onSysMenuCommand).
       + Properties on "Quit NewApplication":
         + set the tag to 40001 (matching resource.h for quit)
         + on the connection tab, "Sent Actions", remove the default connection, then drag a new connection to controller (onSysMenuCommand).
    + Delete the file/edit/format/view/window/help menus, if you like (or keep some of them if you want)

    + Save and quit IB

  Go to Targets->sample_project, hit cmd+I, go to "Properties", and change Principal class to "SWELLApplication"

