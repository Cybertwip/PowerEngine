#if !defined(GRANNY_SYSTEM_UTILS_H)
#include "header_preamble.h"
// ========================================================================
// Platform-agnostic system utilities header
// ========================================================================

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif


#include <system_error>
#include <fstream>

BEGIN_GRANNY_NAMESPACE;

// Log error using std::error_code for platform-agnostic error handling
void LogLastError(bool IsError,
                  char const* SourceFile, int32x SourceLineNumber,
                  char const* FailedFunction,
                  const std::error_code& ErrorCode);

// Macros for logging errors and warnings
#define LogErrorAsWarning(FailedFunction) \
    LogLastError(false, __FILE__, __LINE__, #FailedFunction, std::error_code(errno, std::system_category()))
#define LogErrorAsError(FailedFunction) \
    LogLastError(true, __FILE__, __LINE__, #FailedFunction, std::error_code(errno, std::system_category()))

// Portable file seeking function
int32x Seek(std::fstream& FileStream, int32x Offset, file_writer_seek_type SeekType);

// Constant for invalid seek result
constexpr int32x InvalidSeekPosition = -1;

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SYSTEM_UTILS_H
#endif