// Platform-agnostic utility functions for C++17
#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif


#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#include <system_error>
#include <fstream>
#include <string>

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#define SubsystemCode SystemLogMessage

USING_GRANNY_NAMESPACE;

namespace granny {

void LogLastError(bool IsError,
                  char const* SourceFile, int32x SourceLineNumber,
                  char const* FailedFunction,
                  const std::error_code& ErrorCode) {
    if (ErrorCode) {
        std::string error_message = ErrorCode.message();

        Log4(IsError ? ErrorLogMessage : WarningLogMessage,
             SystemLogMessage,
             "%s failed with error \"%s\" at %s(%d)",
             FailedFunction, error_message.c_str(),
             SourceFile, SourceLineNumber);
    }
}

int32x Seek(std::fstream& FileStream, int32x Offset, file_writer_seek_type SeekType) {
    std::ios::seekdir dir;
    switch (SeekType) {
        case SeekStart:
            dir = std::ios::beg;
            break;
        case SeekEnd:
            dir = std::ios::end;
            break;
        case SeekCurrent:
            dir = std::ios::cur;
            break;
        default:
            InvalidCodePath("Invalid seek type");
            return -1;
    }

    try {
        FileStream.seekg(Offset, dir);
        FileStream.seekp(Offset, dir); // Ensure both input and output pointers are set
        if (!FileStream.fail()) {
            return static_cast<int32x>(FileStream.tellg());
        } else {
            std::error_code ec(errno, std::system_category());
            LogLastError(true, __FILE__, __LINE__, "Seek", ec);
            return -1;
        }
    } catch (const std::exception& e) {
        std::error_code ec(errno, std::system_category());
        LogLastError(true, __FILE__, __LINE__, "Seek", ec);
        return -1;
    }
}

} // namespace granny