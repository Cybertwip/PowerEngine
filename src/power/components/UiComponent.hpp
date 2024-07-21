#pragma once

#include <functional>

class UiComponent {
public:
    UiComponent(std::function<void()> onSelected) {
    }
    
    void select(){
        mOnSelected();
    }
    
private:
    std::function<void()> mOnSelected;
};
