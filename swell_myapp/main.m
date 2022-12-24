//
//  main.m
//  swell_myapp
//

#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[])
{
  extern const char **g_argv;
  extern int g_argc;
  g_argc=argc;
  g_argv=argv;
  return NSApplicationMain(argc, argv);
}
