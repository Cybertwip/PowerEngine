#pragma once

#include <glm/glm.hpp>

class ColorComponent {
public:
    ColorComponent(int actorId)
        : mActorId(actorId), mColor(1.0f, 1.0f, 1.0f, 1.0f), mVisible(true) {}

    int identifier() {
        return mActorId;
    }

    void set_color(const glm::vec4& color) {
        mColor = color;
    }

    glm::vec4 get_color() const {
        return mColor;
    }

    void set_visible(bool visible) {
        mVisible = visible;
    }

    bool get_visible() const {
        return mVisible;
    }

private:
    int mActorId;
    glm::vec4 mColor;
    bool mVisible;
};