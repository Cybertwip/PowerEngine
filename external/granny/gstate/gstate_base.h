// ========================================================================
// $File: //jeffr/granny_29/gstate/gstate_base.h $
// $DateTime: 2012/01/31 14:35:28 $
// $Change: 36448 $
// $Revision: #5 $
//
// $Notice: $
// ========================================================================
#if !defined(GSTATE_BASE_H)

#include "granny.h"
#include "radbase.h"
#include "gstate_exp_interface.h"
#include <stdlib.h>
#include <new>

// These are for CRAPI
EXPGROUP(Housekeeping_Details)
#define ProductSupportAddress "granny3@radgametools.com" EXPMACRO
#define ProductSupportPage "www.radgametools.com/granny.html" EXPMACRO

EXPGROUP(BaseGroup)

#if defined(__MSC_VER)
  #define GSTATE_CALLBACK __cdecl EXPTAG(GSTATE_CALLBACK docproto)
#else
  #define GSTATE_CALLBACK
#endif

/*
   Internal calling convention for GState callback functions.

   For now, this is simply a synonym for __cdecl, but it is explicit to avoid linkage
   problems in projects that specify a different calling convention.
*/


// Namespace definitions
#ifndef GSTATE_NO_NAMESPACE
  #define BEGIN_GSTATE_NAMESPACE  namespace gstate { typedef int __require_semi
  #define END_GSTATE_NAMESPACE    }                  typedef int __require_semi
  #define USING_GSTATE_NAMESPACE  using namespace gstate
  #define GSTATE                  gstate::
#else
  #define BEGIN_GSTATE_NAMESPACE  typedef int __require_semi
  #define END_GSTATE_NAMESPACE    typedef int __require_semi
  #define USING_GSTATE_NAMESPACE  typedef int __require_semi
  #define GSTATE
#endif

#define GSTATE_DISALLOW_COPY(TypeName)                   \
    private: TypeName(TypeName const&);                  \
    private: TypeName const& operator=(TypeName const&)


BEGIN_GSTATE_NAMESPACE;

void* GStateAllocDriver(char const* File, int Line, size_t Size);
void  GStateDeallocateDriver(char const* File, int Line, void* Ptr);

void GStateLogCallback(char const* File, int Line,
                       granny_log_message_type Type,
                       char const* Message, ...);

END_GSTATE_NAMESPACE;

// handy little template...
template <class T>
inline T* MatchPointer(void* ToMatch, T*)
{
    return (T*)ToMatch;
}


#if !defined(GSTATE_LOGGING_DISABLED) || !GSTATE_LOGGING_DISABLED
   #define GStateError(...)   GStateLogCallback(__FILE__, __LINE__, GrannyErrorLogMessage,##__VA_ARGS__)
   #define GStateWarning(...) GStateLogCallback(__FILE__, __LINE__, GrannyWarningLogMessage,##__VA_ARGS__)
#else
   #define GStateError(...) 0
   #define GStateWarning(...) 0
#endif



#define GStateCheckPtrNotNULL(Ptr, ret)                         \
    do {                                                        \
        if ((Ptr) == 0)                                         \
        {                                                       \
            GStateError(#Ptr " is NULL in %s\n", __FUNCTION__); \
            ret;                                                \
        }                                                       \
    } while ((Ptr) == 0)

#define GStateCheckIndex(Idx, ArraySize, ret)                           \
    do {                                                                \
        if ((Idx) < 0 || (Idx) >= (ArraySize))                          \
        {                                                               \
            GStateError(#Idx " out of bounds in %s\n", __FUNCTION__);   \
            ret;                                                        \
        }                                                               \
    } while ((Idx) < 0)

#define GStateCheck(Cond, ret)                                      \
    do {                                                            \
        if (!(Cond)) {                                              \
            GStateError(#Cond " was false in %s\n", __FUNCTION__);  \
            ret;                                                    \
        }                                                           \
    } while (!(Cond))

#define GStateArrayLen(Arr) (sizeof(Arr)/sizeof((Arr)[0]))

#define GStateAlloc(Size)             ((GSTATE GStateAllocDriver)(__FILE__, __LINE__, Size))
#define GStateDeallocate(Ptr)                                           \
    do {                                                                \
        ((GSTATE GStateDeallocateDriver)(__FILE__, __LINE__, (Ptr)));   \
        (Ptr) = 0;                                                      \
    } while ((Ptr) != 0)

#define GStateAllocStruct(Type)       (Type*)GStateAlloc(sizeof(Type))
#define GStateAllocArray(Type, Count) (Type*)GStateAlloc((Count) * sizeof(Type))

#define GStateAllocZeroedStruct(PtrVar)                                         \
    do {                                                                        \
        (PtrVar) = 0; /* unconfuses RTC, since we're passing to MatchPointer */ \
        PtrVar = MatchPointer(GStateAlloc(sizeof(*PtrVar)), PtrVar);            \
        if (PtrVar) memset(PtrVar, 0, sizeof(*PtrVar));                         \
    } while (false)

#define GStateAllocZeroedArray(PtrVar, Count)                                   \
    do {                                                                        \
        PtrVar = MatchPointer(GStateAlloc((Count) * sizeof(*PtrVar)), PtrVar);  \
        if (PtrVar) memset(PtrVar, 0, (Count) * sizeof(*PtrVar));               \
    } while (false)

template <class T>
T* GStateNew()
{
    T* RetPtr = GStateAllocStruct(T);
    if (RetPtr)
    {
        new ((void*)RetPtr) T;
    }

    return RetPtr;
}

template <class T, class A0>
T* GStateNew(A0 a0)
{
    T* RetPtr = GStateAllocStruct(T);
    if (RetPtr)
    {
        new ((void*)RetPtr) T(a0);
    }

    return RetPtr;
}

template <class T>
void GStateDelete(T* Trash)
{
    if (Trash != 0)
        Trash->~T();

    GStateDeallocate(Trash);
}

#define GStateCloneArray(ToArray, FromArray, Count)                 \
    if (FromArray)                                                  \
    {                                                               \
        void* mem = GStateAlloc(sizeof((FromArray)[0]) * Count);    \
        memcpy(mem, FromArray, sizeof((FromArray)[0]) * Count);     \
        ToArray = MatchPointer(mem, FromArray);                     \
    }                                                               \
    else                                                            \
    {                                                               \
        ToArray = 0;                                                \
    } typedef int __require_semi

#define GStateCloneStruct(To, From)                 \
    if (From)                                       \
    {                                               \
        void* mem = GStateAlloc(sizeof(*(From)));   \
        memcpy(mem, (From), sizeof(*(From)));       \
        (To) = MatchPointer(mem, From);             \
    }                                               \
    else                                            \
    {                                               \
        To = 0;                                     \
    } typedef int __require_semi

// Note that the dodge with NewBuf is for the case that ToStr == Str
#define GStateCloneString(ToStr, Str)                   \
    {                                                   \
        char const* StoreStr = (Str);                   \
        if (StoreStr != 0)                              \
        {                                               \
            size_t BufSize = strlen(StoreStr) + 1;      \
            char* NewBuf = (char*)GStateAlloc(BufSize); \
            if (NewBuf) {                               \
                strcpy(NewBuf, StoreStr);               \
                (ToStr) = NewBuf;                       \
            }                                           \
        } else {                                        \
            ToStr = 0;                                  \
        }                                               \
    } typedef int __require_semi

#define GStateReplaceString(ToStr, Str)             \
    /* leave it alone if it's the same pointer */   \
    if ((ToStr) != (Str))                           \
    {                                               \
        GStateDeallocate(ToStr);                    \
        ToStr = 0;                                  \
        GStateCloneString((ToStr), (Str));          \
    } typedef int __require_semi

// Turn off stupid do { } while (false) warning
#ifdef _MSC_VER
  #pragma warning(disable:4127)
#else
  #define UNSAFE_VSNPRINTF 1

  #if defined(__SNC__)
    #pragma diag_suppress=237
    #define _stricmp strcasecmp
    #define _strnicmp strncasecmp
  #elif (defined(__GNUG__) || defined(__GNUC__))
    #define _stricmp strcasecmp
    #define _strnicmp strncasecmp
  #elif defined(NN_COMPILER_RVCT)
    #define _stricmp strcasecmp
    #define _strnicmp strncasecmp
  #elif defined(HOLLYWOOD_REV) || defined(REVOLUTION)
    #pragma warn_illtokenpasting off
    #define _stricmp stricmp
    #define _strnicmp strnicmp
  #endif
#endif


// Enough forward declaration to cover most application headers
struct gstate_character_info;
struct gstate_character_instance;

// Application 


EXPAPI typedef granny_real32 GSTATE_CALLBACK GStateRandomGenerator(void* UserData);
/*
Random generation function

$:UserData Context variable (See $gstate_random_callback.)
$:return A randomly generated floating point number between 0 and 1, inclusive.

See $GStateOverrideRandomCallback.
*/

EXPTYPE struct gstate_random_callback
{
    GStateRandomGenerator* Generator;
    void*                  UserData;
};
/*
  Specifies a Generator/Context pair.

  $:Generator The generator function.  Note that a NULL value indicates that the default GState handler should be called.
  $:UserData Arbitrary context variable passed to the generator for each call.
*/

EXPAPI void GStateGetRandomCallback(EXPOUT gstate_random_callback* CurrentCallback);
/*
  Retrieves the installed random generator.

  $:CurrentCallback The currently installed generator.
*/

EXPAPI void GStateOverrideRandomCallback(EXPIN gstate_random_callback* NewCallback);
/*
Overrides the installed random generator.

$:NewCallback The random generator to install. Passing NULL will reset the callback to the library default.

This allows the application to override the GState default random generator.  If you have
a deterministic playback system, it's especially important to wire this to your random
generation system.  All random numbers required by GState playback will route through this
system.
*/


EXPAPI typedef bool GSTATE_CALLBACK GStateAssertionHandler(EXPIN char const* Condition,
                                                           EXPIN char const* File,
                                                           EXPIN int Line);
/*
  Function type for handling library assertions.

  $:Condition String representation of the condition that failed
  $:File Filename of the assertion location
  $:Line line number of the assertion location
  $:return true if the assertion should be ignored, false to cause a breakpoint

  See $GStateOverrideAssertionHandler for more information.
*/

EXPAPI GStateAssertionHandler* GStateGetAssertionHandler();
/*
  Returns the currently installed assertion handler.
*/

EXPAPI void GStateOverrideAssertionHandler(GStateAssertionHandler* Handler);
/*
  Overrides the installed assertion handler

  $:Hander New function to install

  This allows the application to catch and handle asserts through its own debug system, if
desired.  By default, GState will always issue a breakpoint at the location of the
assertion in DEBUG mode.  If a handler is installed, and returns true, execution will be
allowed to continue without the breakpoint.  This is handy if you have already notified
the user of the problem, and want to continue trying to finish the frame.
*/

BEGIN_GSTATE_NAMESPACE;

bool HandleAssertion(char const* Condition,
                     char const* File,
                     int Line);

granny_real32 RandomUnit();

END_GSTATE_NAMESPACE;

// GState Assertions

#define GS_InRange(idx, cnt) ((idx) >= 0 && (idx) < (cnt))

#ifndef NDEBUG

#define GStateAssert(cond)                                      \
    do                                                          \
    {                                                           \
        if (!(cond))                                            \
        {                                                       \
            if (!HandleAssertion(#cond, __FILE__, __LINE__)) {  \
                RR_BREAK();                                     \
            }                                                   \
        }                                                       \
    } while (false)

#define GS_NotYetImplemented    GStateAssert(!"NYI")
#define GS_InvalidCodePath(msg)                     \
    do {                                            \
        GStateError("InvalidCodePath: %s", msg);    \
        GStateAssert(!msg);                         \
    } while (false)

#define GS_PreconditionFailed                                   \
    do {                                                        \
        GStateError("PreconditionFailed: %s", __FUNCTION__);    \
        GStateAssert(false);                                    \
    } while (false)

#else

#ifndef CAFE
#define GStateAssert(cond) ((void)sizeof(cond))
#else
#define GStateAssert(cond)
#endif

#define GS_NotYetImplemented    (void)0
#define GS_InvalidCodePath(msg) ((void)sizeof(msg))
#define GS_PreconditionFailed   (void)0

#endif


#define GSTATE_BASE_H
#endif /* GSTATE_BASE_H */
