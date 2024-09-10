// grok_client.hpp

#pragma once

#include <grpcpp/grpcpp.h>
#include <string>
#include <memory>
#include "Message.hpp"

namespace xai {

class GrokClient {
public:
    GrokClient(const std::string& api_key, const std::string& api_host = "api.x.ai:443");

    grpc::Status GrokQuery(const std::string& question, std::string& response);

private:
    std::shared_ptr<grpc::Channel> channel_;
    class GrokStub;
    std::unique_ptr<GrokStub> stub_;

    static std::shared_ptr<grpc::Channel> CreateChannel(const std::string& target_str, const std::string& api_key);
};

} // namespace xai
