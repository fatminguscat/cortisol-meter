#include "CortisolMeter.hpp"

#include "Art.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace cm {

namespace {

// Panel size in screen units (the gauge art is 372x300).
constexpr float kGaugeW = 80.f, kGaugeH = 64.5f;
constexpr float kBarW = 80.f, kBarH = 8.f;

// Dial geometry on resources/cortisol_gauge.png (pixels, y down). The original art's white card,
// its baked needle and hub were removed; the hub lives in resources/cortisol_hub.png and sits on
// top of the code-drawn needle. Text, needle and hub get a thin white outline so they read on
// dark levels too.
constexpr float kArtW = 372.f, kArtH = 300.f;
constexpr float kPivotX = 185.5f, kPivotY = 270.1f;
constexpr float kNeedleLen = 137.f, kNeedleHalfWidth = 13.6f;
constexpr float kOutline = 2.5f;
constexpr float kHubCropSize = 56.f, kHubCenterX = 27.5f, kHubCenterY = 28.1f;
ccColor4F const kNeedleColor { 53.f / 255.f, 53.f / 255.f, 53.f / 255.f, 1.f };
ccColor4F const kOutlineColor { 1.f, 1.f, 1.f, 1.f };

// Clicks per second that reads as "maxed out".
constexpr float kFullCps = 10.f;

}

CortisolMeter* CortisolMeter::create() {
    auto ret = new CortisolMeter();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool CortisolMeter::init() {
    if (!CCNode::init()) return false;
    this->setAnchorPoint({ 0.f, 0.f });
    if (!this->initGauge()) this->initBar();
    this->apply();
    return true;
}

bool CortisolMeter::initGauge() {
    auto gauge = loadArt("cortisol_gauge.png"_spr, false);
    auto hub = loadArt("cortisol_hub.png"_spr, false);
    if (!gauge || !hub) return false;

    this->setContentSize({ kGaugeW, kGaugeH });
    placeArt(this, gauge, { 0.f, 0.f, kGaugeW, kGaugeH }, { 0.f, 0.f });

    // Needle and hub live in the gauge sprite's own space, so they scale with it exactly.
    auto size = gauge->getContentSize();
    float const px = size.width / kArtW;  // sprite units per source pixel
    CCPoint const pivot { kPivotX * px, (kArtH - kPivotY) * px };

    // Drawn in source pixels pointing straight up (value 50), then scaled and rotated.
    // White outline first: round-capped strokes along the needle's edges.
    auto needle = CCDrawNode::create();
    CCPoint const edge[] = { { -kNeedleHalfWidth, 0.f }, { kNeedleHalfWidth, 0.f }, { 0.f, kNeedleLen } };
    for (int i = 0; i < 3; i++) needle->drawSegment(edge[i], edge[(i + 1) % 3], kOutline, kOutlineColor);
    // GD's drawPolygon (no border) pushes every corner out by half a local unit along its
    // miter, which on a tip this sharp adds ~5px; pull the tip in so it ends where the
    // original needle did.
    float const halfWidth = kNeedleHalfWidth - 0.5f;
    float const tip = kNeedleLen - 0.5f / std::sin(std::atan(halfWidth / kNeedleLen));
    CCPoint verts[] = { { -halfWidth, 0.f }, { halfWidth, 0.f }, { 0.f, tip } };
    needle->drawPolygon(verts, 3, kNeedleColor, 0.f, kNeedleColor);
    needle->setScale(px);
    needle->setPosition(pivot);
    needle->setID("needle"_spr);
    gauge->addChild(needle, 1);
    m_needle = needle;

    auto hubSize = hub->getContentSize();
    if (hubSize.width > 0.f) hub->setScale(kHubCropSize * px / hubSize.width);
    hub->setAnchorPoint({ kHubCenterX / kHubCropSize, 1.f - kHubCenterY / kHubCropSize });
    hub->setPosition(pivot);
    gauge->addChild(hub, 2);
    return true;
}

void CortisolMeter::initBar() {
    this->setContentSize({ kBarW, kBarH });
    auto back = CCLayerColor::create({ 20, 20, 20, 200 }, kBarW, kBarH);
    back->ignoreAnchorPointForPosition(false);
    back->setAnchorPoint({ 0.f, 0.f });
    this->addChild(back);
    m_fill = CCLayerColor::create({ 0, 200, 60, 255 }, 0.f, kBarH);
    m_fill->ignoreAnchorPointForPosition(false);
    m_fill->setAnchorPoint({ 0.f, 0.f });
    this->addChild(m_fill, 1);
    m_barWidth = kBarW;
}

void CortisolMeter::setTarget(float value) {
    m_target = std::clamp(value, 0.f, 100.f);
}

void CortisolMeter::setShown(float value) {
    m_shown = std::clamp(value, 0.f, 100.f);
    this->apply();
}

void CortisolMeter::step(float dt) {
    float const k = 1.f - std::exp(-std::max(dt, 0.f) * 5.f);
    m_shown += (m_target - m_shown) * k;
    this->apply();
}

void CortisolMeter::apply() {
    if (m_needle) m_needle->setRotation(-90.f + 180.f * m_shown / 100.f);
    if (m_fill) {
        m_fill->setContentSize({ m_barWidth * m_shown / 100.f, kBarH });
        // green -> yellow -> red
        float const t = m_shown / 100.f;
        GLubyte const r = static_cast<GLubyte>(std::min(1.f, t * 2.f) * 230.f);
        GLubyte const g = static_cast<GLubyte>(std::min(1.f, (1.f - t) * 2.f) * 200.f);
        m_fill->setColor({ r, g, 40 });
    }
}

float CortisolMeter::compute(std::optional<float> percent, float cps) {
    auto weight = [](char const* key) {
        return static_cast<float>(std::max(0.0, Mod::get()->getSettingValue<double>(key)));
    };
    float sum = 0.f, total = 0.f;
    auto add = [&](float w, float value) {
        sum += w * value;
        total += w;
    };
    if (percent) add(weight("weight-progress"), std::clamp(*percent, 0.f, 100.f));
    add(weight("weight-cps"), std::clamp(cps / kFullCps, 0.f, 1.f) * 100.f);
    return total > 0.f ? sum / total : 0.f;
}

}
