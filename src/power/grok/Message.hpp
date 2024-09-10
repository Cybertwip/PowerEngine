// grok_message.hpp

#pragma once

#include <string>

namespace xai {

class ChatRequest {
public:
    void set_message(const std::string& msg) { message_ = msg; }
    const std::string& message() const { return message_; }

private:
    std::string message_;
};

class ChatResponse {
public:
    void set_message(const std::string& msg) { message_ = msg; }
    const std::string& message() const { return message_; }

private:
    std::string message_;
};

} // namespace xai