#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "../wdlstring.h"
#include "../ptrlist.h"
#include "eel_pproc.h"

void NSEEL_HOSTSTUB_EnterMutex() { }
void NSEEL_HOSTSTUB_LeaveMutex() { }

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr,"Usage: %s [scriptfile | -]\n",argv[0]);
    return 1;
  }
  FILE *fp = strcmp(argv[1],"-") ? fopen(argv[1],"rb") : stdin;
  if (!fp)
  {
    fprintf(stderr,"Error: could not open %s\n",argv[1]);
    return 1;
  }
  WDL_FastString file_str, pp_str;
  for (;;)
  {
    char buf[4096];
    if (!fgets(buf,sizeof(buf),fp)) break;
    file_str.Append(buf);
  }
  if (fp != stdin) fclose(fp);

  EEL2_PreProcessor pproc;
  const char *err = pproc.preprocess(file_str.Get(),&pp_str);
  if (err)
  {
    fprintf(stderr,"Error: %s\n",err);
    return 1;
  }

  printf("%s",pp_str.Get());

  return 0;
}
