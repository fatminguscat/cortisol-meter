#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <optional>

#include "ClickTracker.hpp"
#include "CortisolMeter.hpp"

using namespace geode::prelude;
using namespace cm;

namespace {

constexpr float kEdge = 5.f;   // bottom-left corner, like before
constexpr int kZOrder = 21;     // above GD's in-level UI (progress bar z 10, percentage z 15)

float meterScale() {
    return std::clamp(static_cast<float>(Mod::get()->getSettingValue<double>("scale")), 0.25f, 2.f);
}

}

class $modify(CMPlayLayer, PlayLayer) {
    struct Fields {
        CortisolMeter* meter = nullptr;
        float shown = 0.f;  // survives the meter being rebuilt or switched off and on
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        ClickTracker::clear();
        this->buildMeter();
        return true;
    }

    // (Re)creates the meter; also used when a setting changes mid-level.
    void buildMeter() {
        auto fields = m_fields.self();
        if (fields->meter) {
            fields->shown = fields->meter->shown();
            fields->meter->removeFromParent();
            fields->meter = nullptr;
        }
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;

        auto meter = CortisolMeter::create();
        if (!meter) return;
        meter->setID("meter"_spr);
        meter->setScale(meterScale());
        meter->setPosition({ kEdge, kEdge });
        meter->setShown(fields->shown);
        this->addChild(meter, kZOrder);
        fields->meter = meter;
        log::info("meter built: scale {:.2f}", meterScale());
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto meter = m_fields->meter;
        if (!meter) return;

        // Platformer levels have no completion percent, so that part sits out.
        std::optional<float> const percent = m_isPlatformer ? std::nullopt : std::optional(this->getCurrentPercent());
        meter->setTarget(CortisolMeter::compute(percent, ClickTracker::cps()));
        meter->step(dt);
    }
};

class $modify(CMGameLayer, GJBaseGameLayer) {
    // Declared on GJBaseGameLayer only (not virtual, not redeclared on PlayLayer in 2.2081).
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        // Jump presses in a real level only (this also runs in the editor).
        if (down && button == 1 && typeinfo_cast<PlayLayer*>(this)) ClickTracker::press();
    }
};

$on_mod(Loaded) {
    auto rebuild = [] {
        if (auto playLayer = PlayLayer::get()) static_cast<CMPlayLayer*>(playLayer)->buildMeter();
    };
    listenForSettingChanges<bool>("enabled", [rebuild](bool) { rebuild(); });
    listenForSettingChanges<double>("scale", [rebuild](double) { rebuild(); });
}
