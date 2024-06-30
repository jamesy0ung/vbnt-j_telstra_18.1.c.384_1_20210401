
#if __GNUC__ >= 4
  #define DLL_LOCAL __attribute__ ((visibility ("hidden")))
#else
  #define DLL_LOCAL
#endif

extern DLL_LOCAL int tch_ld_debug;

#define debug(...)  { if (tch_ld_debug) { __debug(__VA_ARGS__); } }
void DLL_LOCAL __debug(const char *format, ...);

