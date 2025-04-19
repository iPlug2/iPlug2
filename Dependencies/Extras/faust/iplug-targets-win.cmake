# this file may be used to select different target
# values are among ON or OFF 

set ( INCLUDE_EXECUTABLE  ON  CACHE STRING  "Include faust compiler" FORCE )
set ( INCLUDE_STATIC      OFF  CACHE STRING  "Include static faust library" FORCE )
set ( INCLUDE_DYNAMIC     ON  CACHE STRING  "Include dynamic faust library" FORCE )

set ( INCLUDE_OSC         OFF  CACHE STRING  "Include Faust OSC static library" FORCE )
set ( INCLUDE_HTTP        OFF  CACHE STRING  "Include Faust HTTPD library" FORCE )

set ( OSCDYNAMIC          OFF CACHE STRING  "Include Faust OSC dynamic library" FORCE )
set ( HTTPDYNAMIC         OFF CACHE STRING  "Include Faust HTTP dynamic library" FORCE )

