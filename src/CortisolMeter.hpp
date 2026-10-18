#pragma once

#include <Geode/Geode.hpp>

#include <optional>

namespace cm {

// The stress meter: a 0-100 value on the supplied gauge art (no background card) with a
// code-drawn needle (-90deg at 0, +90deg at 100). Falls back to a plain colored bar only if the
// gauge art is missing.
class CortisolMeter : public cocos2d::CCNode {
public:
    static CortisolMeter* create();

    void setTarget(float value);  // 0-100
    // Eases the shown value toward the target; called from the game's postUpdate so the needle
    // only moves while the level is running.
    void step(float dt);
    float shown() const { return m_shown; }
    void setShown(float value);

    // Combines the inputs into the 0-100 value, using the weights from settings. The level
    // percent is left out on platformer levels (there is none) instead of counting as 0, so the
    // needle can still reach the top.
    static float compute(std::optional<float> percent, float cps);

private:
    bool init() override;
    bool initGauge();
    void initBar();
    void apply();

    cocos2d::CCNode* m_needle = nullptr;
    cocos2d::CCLayerColor* m_fill = nullptr;
    float m_barWidth = 0.f;
    float m_target = 0.f;
    float m_shown = 0.f;
};

}
