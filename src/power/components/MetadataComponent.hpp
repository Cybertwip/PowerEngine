#pragma once

#include <string>
#include <string_view>

class MetadataComponent {
public:
    MetadataComponent(const std::string& name) : mName(name) {
    }

    void set_name(std::string_view name) {
        mName = name;
    }
    std::string_view get_name() const {
        return mName;
    }
    
private:
    std::string mName;
};
