#ifndef __RADBASEH__
  #define __RADBASEH__

  #define RADCOPYRIGHT "Copyright (C) 1994-2007, RAD Game Tools, Inc."

  #ifndef __RADRES__

    #define RADSTRUCT struct

    // Define platform and architecture macros
    #if defined(_WIN64)
      #define __RADWIN__
      #define __RADNT__
      #define __RAD32__      // At least 32-bit code
      #define __RAD64__      // 64-bit code
      #define __RADX64__
      #define __RADMMX__
      #define __RADX86__
      #define __RADLITTLEENDIAN__
      #define RADINLINE __inline
      #define RADRESTRICT __restrict

    #elif defined(HOLLYWOOD_REV) || defined(REVOLUTION)
      #define __RADWII__
      #define __RAD32__
      #define __RADPPC__
      #define __RADBIGENDIAN__
      #define RADINLINE inline
      #define RADRESTRICT __restrict

    #elif defined(GEKKO)
      #define __RADNGC__
      #define __RAD32__
      #define __RADPPC__
      #define __RADBIGENDIAN__
      #define RADINLINE inline
      #define RADRESTRICT // __restrict not supported on cw

    #elif defined(__arm) && defined(__MWERKS__)
      #define __RADNDS__
      #define __RAD32__
      #define __RADARM__
      #define __RADLITTLEENDIAN__
      #define __RADFIXEDPOINT__
      #define RADINLINE inline
      #define RADRESTRICT // __restrict not supported on cw

    #elif defined(R5900)
      #define __RADPS2__
      #define __RAD32__
      #define __RADMIPS__
      #define __RADLITTLEENDIAN__
      #define RADINLINE inline
      #define RADRESTRICT __restrict
      #if !defined(__MWERKS__)
        #undef RADSTRUCT
        #define RADSTRUCT struct __attribute__((__packed__))
      #endif

    #elif defined(__psp__)
      #define __RADPSP__
      #define __RAD32__
      #define __RADMIPS__
      #define __RADLITTLEENDIAN__
      #define RADINLINE inline
      #define RADRESTRICT __restrict
      #if !defined(__MWERKS__)
        #undef RADSTRUCT
        #define RADSTRUCT struct __attribute__((__packed__))
      #endif

    #elif defined(__CELLOS_LV2__)
      #ifdef __SPU__
        #define __RADSPU__
        #define __RAD32__
        #define __RADCELL__
        #define __RADBIGENDIAN__
        #define RADINLINE inline
        #define RADRESTRICT __restrict
      #else
        #define __RADPS3__
        #define __RADPPC__
        #define __RAD32__
        #define __RADCELL__
        #define __RADBIGENDIAN__
        #define RADINLINE inline
        #define RADRESTRICT __restrict
      #endif
      #ifndef __LP32__
        #error "PS3 32bit ABI support only"
      #endif

    #elif (defined(__MWERKS__) && !defined(__INTEL__)) || defined(__MRC__) || defined(THINK_C) || defined(powerc) || defined(macintosh) || defined(__powerc) || defined(__APPLE__) || defined(__MACH__)
      #define __RADMAC__
      #if defined(powerc) || defined(__powerc) || defined(__ppc__)
        #define __RADPPC__
        #define __RADBIGENDIAN__
        #define __RADALTIVEC__
        #define RADRESTRICT
        #define __RAD32__
      #elif defined(__i386__)
        #define __RADX86__
        #define __RADMMX__
        #define __RADLITTLEENDIAN__
        #define RADRESTRICT __restrict
        #define __RAD32__
      #elif defined(__x86_64__)
        #define __RADX64__
        #define __RADMMX__
        #define __RADLITTLEENDIAN__
        #define RADRESTRICT __restrict
        #define __RAD32__  // Ensure __RAD32__ for 64-bit
        #define __RAD64__
      #else
        #define __RAD68K__
        #define __RADBIGENDIAN__
        #define __RADALTIVEC__
        #define RADRESTRICT
        #define __RAD32__
      #endif

      #if defined(__MWERKS__)
        #if (defined(__cplusplus) || !__option(only_std_keywords))
          #define RADINLINE inline
        #endif
        #ifdef __MACH__
          #define __RADMACH__
        #endif
      #elif defined(__MRC__)
        #if defined(__cplusplus)
          #define RADINLINE inline
        #endif
      #elif defined(__GNUC__) || defined(__GNUG__) || defined(__MACH__)
        #define RADINLINE inline
        #define __RADMACH__
        #undef RADRESTRICT
        #define RADRESTRICT __restrict
      #endif

      #ifdef __RADX86__
        #ifndef __RADCARBON__
          #define __RADCARBON__
        #endif
      #endif

      #ifdef TARGET_API_MAC_CARBON
        #if TARGET_API_MAC_CARBON
          #ifndef __RADCARBON__
            #define __RADCARBON__
          #endif
        #endif
      #endif

    #elif defined(linux)
      #define __RADLINUX__
      #define __RADX86__
      #define __RADMMX__
      #define __RAD32__
      #define __RADLITTLEENDIAN__
      #define RADINLINE inline
      #define RADRESTRICT __restrict

    #else
      #if _MSC_VER >= 1400
        #undef RADRESTRICT
        #define RADRESTRICT __restrict
      #else
        #define RADRESTRICT
      #endif

      #if defined(_XENON) || (_XBOX_VER == 200)
        #define __RADPPC__
        #define __RADBIGENDIAN__
        #define __RADALTIVEC__
        #define __RAD32__
      #else
        #define __RADX86__
        #define __RADMMX__
        #define __RADLITTLEENDIAN__
      #endif

      #ifdef __MWERKS__
        #define _WIN32
      #endif

      #ifdef __DOS__
        #define __RADDOS__
      #endif

      #ifdef __386__
        #define __RAD32__
      #endif

      #ifdef _Windows // For Borland
        #ifdef __WIN32__
          #define WIN32
        #else
          #define __WINDOWS__
        #endif
      #endif

      #ifdef _WINDOWS // For MS
        #ifndef _WIN32
          #define __WINDOWS__
        #endif
      #endif

      #ifdef _WIN32
        #if defined(_XENON) || (_XBOX_VER == 200)
          #define __RADXENON__
        #elif defined(_XBOX)
          #define __RADXBOX__
        #else
          #define __RADNT__
        #endif
        #define __RADWIN__
        #define __RAD32__
      #else
        #ifdef __NT__
          #if defined(_XENON) || (_XBOX_VER == 200)
            #define __RADXENON__
          #elif defined(_XBOX)
            #define __RADXBOX__
          #else
            #define __RADNT__
          #endif
          #define __RADWIN__
          #define __RAD32__
        #else
          #ifdef __WINDOWS_386__
            #define __RADWIN__
            #define __RADWINEXT__
            #define __RAD32__
          #else
            #ifdef __WINDOWS__
              #define __RADWIN__
              #define __RAD16__
            #else
              #ifdef WIN32
                #if defined(_XENON) || (_XBOX_VER == 200)
                  #define __RADXENON__
                #elif defined(_XBOX)
                  #define __RADXBOX__
                #else
                  #define __RADNT__
                #endif
                #define __RADWIN__
                #define __RAD32__
              #endif
            #endif
          #endif
        #endif
      #endif

      #ifdef __WATCOMC__
        #define RADINLINE
      #else
        #define RADINLINE __inline
      #endif
    #endif

    // Ensure all platforms are detected
    #if (!defined(__RADDOS__) && !defined(__RADWIN__) && !defined(__RADMAC__) && \
         !defined(__RADNGC__) && !defined(__RADNDS__) && !defined(__RADXBOX__) && \
         !defined(__RADXENON__) && !defined(__RADLINUX__) && !defined(__RADPS2__) && \
         !defined(__RADPSP__) && !defined(__RADPS3__) && !defined(__RADSPU__) && !defined(__RADWII__))
      #error "RAD.H did not detect your platform. Define DOS, WINDOWS, WIN32, macintosh, powerpc, or appropriate console."
    #endif

    // Ensure __RAD32__ is defined if __RAD64__ is
    #ifdef __RAD64__
      #ifndef __RAD32__
        #define __RAD32__
      #endif
    #endif

    #ifdef __RADFINAL__
      #define RADTODO(str) { char __str[0]=str; }
    #else
      #define RADTODO(str)
    #endif

    // Function linkage definitions
    #if (defined(__RADNGC__) || defined(__RADWII__) || defined(__RADPS2__) || \
         !defined(__RADPSP__) || defined(__RADPS3__) || defined(__RADSPU__) || defined(__RADNDS__))
      #define RADLINK
      #define RADEXPLINK
      #define RADEXPFUNC RADDEFFUNC
      #define RADASMLINK
      #define PTR4
    #elif defined(__RADLINUX__)
      #define RADLINK __attribute__((cdecl))
      #define RADEXPLINK __attribute__((cdecl))
      #define RADEXPFUNC RADDEFFUNC
      #define RADASMLINK
      #define PTR4
    #elif defined(__RADMAC__)
      #define __MSL_LONGLONG_SUPPORT__
      #define RADLINK
      #define RADEXPLINK
      #if defined(__CFM68K__) || defined(__MWERKS__)
        #ifdef __RADINDLL__
          #define RADEXPFUNC RADDEFFUNC __declspec(export)
        #else
          #define RADEXPFUNC RADDEFFUNC __declspec(import)
        #endif
      #else
        #if defined(__RADMACH__) && !defined(__MWERKS__)
          #ifdef __RADINDLL__
            #define RADEXPFUNC RADDEFFUNC __attribute__((visibility("default")))
          #else
            #define RADEXPFUNC RADDEFFUNC
          #endif
        #else
          #define RADEXPFUNC RADDEFFUNC
        #endif
      #endif
      #define RADASMLINK
    #else
      #ifdef __RADNT__
        #ifndef _WIN32
          #define _WIN32
        #endif
        #ifndef WIN32
          #define WIN32
        #endif
      #endif

      #ifdef __RADWIN__
        #ifdef __RAD32__
          #ifdef __RADXBOX__
            #define RADLINK __stdcall
            #define RADEXPLINK __stdcall
            #define RADEXPFUNC RADDEFFUNC
          #elif defined(__RADXENON__)
            #define RADLINK __stdcall
            #define RADEXPLINK __stdcall
            #define RADEXPFUNC RADDEFFUNC
          #elif defined(__RADNTBUILDLINUX__)
            #define RADLINK __cdecl
            #define RADEXPLINK __cdecl
            #define RADEXPFUNC RADDEFFUNC
          #else
            #ifdef __RADNT__
              #define RADLINK __stdcall
              #define RADEXPLINK __stdcall
              #ifdef __RADINEXE__
                #define RADEXPFUNC RADDEFFUNC
              #else
                #ifndef __RADINDLL__
                  #define RADEXPFUNC RADDEFFUNC __declspec(dllimport)
                  #ifdef __BORLANDC__
                    #if __BORLANDC__<=0x460
                      #undef RADEXPFUNC
                      #define RADEXPFUNC RADDEFFUNC
                    #endif
                  #endif
                #else
                  #define RADEXPFUNC RADDEFFUNC __declspec(dllexport)
                #endif
              #endif
            #else
              #define RADLINK __pascal
              #define RADEXPLINK __far __pascal
              #define RADEXPFUNC RADDEFFUNC
            #endif
          #endif
        #else
          #define RADLINK __pascal
          #define RADEXPLINK __far __pascal __export
          #define RADEXPFUNC RADDEFFUNC
        #endif
      #else
        #define RADLINK __pascal
        #define RADEXPLINK __pascal
        #define RADEXPFUNC RADDEFFUNC
      #endif
      #define RADASMLINK __cdecl
    #endif

    #if !defined(__RADXBOX__) && !defined(__RADXENON__)
      #ifdef __RADWIN__
        #ifndef _WINDOWS
          #define _WINDOWS
        #endif
      #endif
    #endif

    #ifndef RADDEFFUNC
      #ifdef __cplusplus
        #define RADDEFFUNC extern "C"
        #define RADDEFSTART extern "C" {
        #define RADDEFEND }
        #define RADDEFINEDATA extern "C"
        #define RADDECLAREDATA extern "C"
        #define RADDEFAULT(val) =val
      #else
        #define RADDEFFUNC
        #define RADDEFSTART
        #define RADDEFEND
        #define RADDEFINEDATA
        #define RADDECLAREDATA extern
        #define RADDEFAULT(val)
      #endif
    #endif

    // Alignment macro
    #if (defined(__RADWII__) || defined(__RADPSP__) || defined(__RADPS3__) || defined(__RADSPU__) || defined(__RADLINUX__) || defined(__RADMAC__) || defined(__RADNDS__))
      #define RAD_ALIGN(type, var, num) type __attribute__((aligned(num))) var
    #elif (defined(__RADNGC__) || defined(__RADPS2__))
      #define RAD_ALIGN(type, var, num) __attribute__((aligned(num))) type var
    #elif (_MSC_VER >= 1300)
      #define RAD_ALIGN(type, var, num) type __declspec(align(num)) var
    #else
      #define RAD_ALIGN(type, var, num) type var
    #endif

    // Type definitions
    #define S8 signed char
    #define U8 unsigned char
    #if defined(__RAD64__)
      #if defined(__RADX64__)
        #define U32 unsigned int
        #define S32 signed int
        #define UINTADDR unsigned long long
        #define INTADDR long long
        #define UINTa unsigned long long
        #define SINTa signed long long
      #else
        #error Unknown 64-bit processor (see radbase.h)
      #endif
    #elif defined(__RAD32__)
      #define U32 unsigned int
      #define S32 signed int
      #define UINTADDR unsigned int
      #define INTADDR int
      #define UINTa unsigned int
      #define SINTa signed int
    #else
      #define U32 unsigned long
      #define S32 signed long
      #define UINTADDR unsigned long
      #define INTADDR signed long
      #define UINTa unsigned long
      #define SINTa signed long
    #endif
    #define F32 float
    #if defined(__RADPS2__) || defined(__RADPSP__)
      typedef RADSTRUCT F64 {
        U32 vals[2];
      } F64;
    #else
      #define F64 double
    #endif

    #if defined(__RADMAC__) || defined(__MRC__) || defined(__RADNGC__) || defined(__RADWII__) || defined(__RADNDS__) || defined(__RADPSP__) || defined(__RADPS3__) || defined(__RADSPU__)
      #define U64 unsigned long long
      #define S64 signed long long
    #elif defined(__RADPS2__)
      #define U64 unsigned long
      #define S64 signed long
    #elif defined(__RADX64__) || defined(__RAD32__)
      #define U64 unsigned long long
      #define S64 signed long long
    #else
      typedef RADSTRUCT U64 {
        U32 vals[2];
      } U64;
      typedef RADSTRUCT S64 {
        S32 vals[2];
      } S64;
    #endif

    #if defined(__RAD32__)
      #define PTR4
      #define U16 unsigned short
      #define S16 signed short
    #else
      #define PTR4 __far
      #define U16 unsigned int
      #define S16 signed int
    #endif

    #if defined(__RADXENON__) || defined(__RADPS3__) || defined(__RADSPU__) || defined(__RADWII__)
      #undef RAD_NO_LOWERCASE_TYPES
      #define RAD_NO_LOWERCASE_TYPES
    #endif

    #ifndef RAD_NO_LOWERCASE_TYPES
      #ifdef __RADNGC__
        #include <dolphin/types.h>
      #elif defined(__RADWII__)
        #include <revolution/types.h>
      #elif defined(__RADNDS__)
        #include <nitro/types.h>
      #else
        #define u8  U8
        #define s8  S8
        #define u16 U16
        #define s16 S16
        #define u32 U32
        #define s32 S32
        #define u64 U64
        #define s64 S64
        #define f32 F32
        #define f64 F64
      #endif
    #endif

    // Disable unreferenced inline function warning
    #ifdef _MSC_VER
      #pragma warning(disable: 4514)
    #endif

  #endif

  #define RAD_UNUSED_VARIABLE(x) (void)(sizeof(x))

  #define RR_BREAK() do { void; } while(0)

#endif