#pragma once

#include <functional>

class UiComponent {
public:
    UiComponent(std::function<void()> onSelected) : mOnSelected(onSelected) {
    }
    
    void select(){
        mOnSelected();
    }
    
private:
    std::function<void()> mOnSelected;
};
