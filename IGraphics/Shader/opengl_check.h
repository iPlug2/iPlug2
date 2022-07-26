#if defined(NDEBUG)
#  define CHK(cmd) cmd
#else
#  define CHK(cmd)                          \
    do {                                    \
        cmd;                                \
        (void) nanogui_check_glerror(#cmd); \
    } while (0)
#endif

