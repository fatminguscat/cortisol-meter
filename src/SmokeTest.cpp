// Unattended in-game smoke test. Only compiled with -DCM_SMOKE_TEST=ON; the shipped build never
// contains any of this. It opens a main level, reads the meter back from its own nodes while
// tapping jump, flips its settings live, saves PNG snapshots, then kills the process without
// saving so the player's stats are untouched.
#ifdef CM_SMOKE_TEST

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/GameLevelManager.hpp>

#include <Windows.h>

#include "ClickTracker.hpp"
#include "CortisolMeter.hpp"

using namespace geode::prelude;

namespace {

struct MeterState {
    bool present = false;
    float shown = -1.f;   // the meter's own eased value
    float needle = -1.f;  // read back from the needle: -90deg = 0, +90deg = 100
    float scale = 0.f;
    CCPoint pos;
    CCSize size;
    int z = -999;
};

MeterState readMeter() {
    MeterState s;
    auto playLayer = PlayLayer::get();
    if (!playLayer) return s;
    auto node = playLayer->getChildByID("meter"_spr);
    if (!node) return s;
    s.present = true;
    s.scale = node->getScale();
    s.pos = node->getPosition();
    s.size = node->getContentSize();
    s.z = node->getZOrder();
    if (auto meter = typeinfo_cast<cm::CortisolMeter*>(node)) s.shown = meter->shown();
    if (auto needle = node->getChildByIDRecursive("needle"_spr)) s.needle = (needle->getRotation() + 90.f) / 1.8f;
    return s;
}

void logMeter(std::string const& when) {
    auto s = readMeter();
    auto playLayer = PlayLayer::get();
    log::info(
        "SMOKE: [{}] meter present {}, shown {:.1f}, needle {:.1f}, scale {:.2f}, at ({:.1f},{:.1f}) size {:.1f}x{:.1f}, z {}, cps {:.1f}, percent {:.1f}",
        when, s.present, s.shown, s.needle, s.scale, s.pos.x, s.pos.y, s.size.width, s.size.height, s.z,
        cm::ClickTracker::cps(), playLayer ? playLayer->getCurrentPercent() : -1.f
    );
}

// Renders the running scene into a PNG in this mod's save folder - works even while the desktop
// is locked, unlike a screen capture.
void snapshot(std::string const& name) {
    auto scene = CCDirector::get()->getRunningScene();
    if (!scene) return;
    auto win = CCDirector::get()->getWinSize();
    auto rt = CCRenderTexture::create(static_cast<int>(win.width), static_cast<int>(win.height));
    if (!rt) return;
    rt->begin();
    scene->visit();
    rt->end();
    auto image = rt->newCCImage(true);
    if (!image) return;
    auto path = (Mod::get()->getSaveDir() / fmt::format("smoke_{}.png", name)).string();
    bool const ok = image->saveToFile(path.c_str(), false);
    image->release();
    log::info("SMOKE: snapshot {} saved: {}", name, ok);
}

void openMainLevel(int id) {
    auto level = GameLevelManager::sharedState()->getMainLevel(id, false);
    log::info("SMOKE: opening main level {}", id);
    CCDirector::get()->replaceScene(CCTransitionFade::create(0.3f, PlayLayer::scene(level, false, false)));
}

class SmokeDriver : public CCObject {
public:
    enum Phase { Menu, Start, Clicking, Scale, Toggle, Done };

    Phase phase = Menu;
    float clock = 0.f;      // seconds since the driver started
    float phaseClock = 0.f; // seconds in the current phase
    int step = 0;
    int taps = 0;
    double originalScale = 1.0;
    bool originalEnabled = true;

    void enter(Phase next) {
        phase = next;
        phaseClock = 0.f;
        step = 0;
        taps = 0;
    }

    void tick(float dt) {
        clock += dt;
        phaseClock += dt;
        if (clock > 120.f) finish("global timeout");
        auto playLayer = PlayLayer::get();
        auto mod = Mod::get();

        switch (phase) {
            case Menu:
                if (phaseClock > 3.f) {
                    originalScale = mod->getSettingValue<double>("scale");
                    originalEnabled = mod->getSettingValue<bool>("enabled");
                    std::string keys;
                    for (auto const& key : mod->getSettingKeys()) keys += key + " ";
                    log::info("SMOKE: settings: {}| scale {:.2f}, enabled {}", keys, originalScale, originalEnabled);
                    // The meter only shows while enabled; the test needs to see it.
                    if (!originalEnabled) mod->setSettingValue<bool>("enabled", true);
                    openMainLevel(1);
                    enter(Start);
                }
                break;
            case Start:
                if (phaseClock > 1.5f && playLayer) {
                    logMeter("level start");
                    snapshot("start");
                    enter(Clicking);
                }
                if (phaseClock > 20.f) finish("level did not open");
                break;
            case Clicking:
                // tap jump ~7 times a second for 3 seconds: the click-speed part should push the needle up
                if (playLayer && phaseClock < 3.f) {
                    int const tap = static_cast<int>(phaseClock / 0.14f);
                    if (tap >= taps) {
                        playLayer->handleButton(true, 1, true);
                        playLayer->handleButton(false, 1, true);
                        taps = tap + 1;
                    }
                }
                if (phaseClock > 1.5f && step == 0) { logMeter("while clicking"); step = 1; }
                if (phaseClock > 3.0f && step == 1) { logMeter("clicking done"); snapshot("clicking"); step = 2; }
                if (phaseClock > 5.0f && step == 2) { logMeter("2s after clicking"); enter(Scale); }
                break;
            case Scale:
                if (step == 0) { mod->setSettingValue<double>("scale", 2.0); step = 1; }
                else if (step == 1 && phaseClock > 0.5f) { logMeter("scale 2"); snapshot("scale2"); mod->setSettingValue<double>("scale", 0.5); step = 2; }
                else if (step == 2 && phaseClock > 1.0f) { logMeter("scale 0.5"); mod->setSettingValue<double>("scale", originalScale); step = 3; }
                else if (step == 3 && phaseClock > 1.5f) { logMeter("scale restored"); enter(Toggle); }
                break;
            case Toggle:
                if (step == 0) { mod->setSettingValue<bool>("enabled", false); step = 1; }
                else if (step == 1 && phaseClock > 0.5f) { logMeter("disabled"); mod->setSettingValue<bool>("enabled", true); step = 2; }
                else if (step == 2 && phaseClock > 1.0f) {
                    logMeter("re-enabled");
                    mod->setSettingValue<bool>("enabled", originalEnabled);
                    enter(Done);
                }
                break;
            case Done:
                if (phaseClock > 1.f) finish("done", 0);
                break;
        }
    }

    // Exit code 0 only for a full run, so the script driving GD can tell a pass from a bailout.
    void finish(char const* why, UINT code = 1) {
        log::info("SMOKE: finished ({}), terminating without saving", why);
        TerminateProcess(GetCurrentProcess(), code);
    }
};

SmokeDriver* s_driver = nullptr;

}

class $modify(SmokeMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!s_driver) {
            s_driver = new SmokeDriver();
            CCDirector::get()->getScheduler()->scheduleSelector(
                schedule_selector(SmokeDriver::tick), s_driver, 0.f, false
            );
            log::info("SMOKE: driver started");
        }
        return true;
    }
};

class $modify(SmokePlayLayer, PlayLayer) {
    // Nobody is at the keyboard: don't let losing window focus pause the test.
    void pauseGame(bool unfocused) {
        if (unfocused) return;
        PlayLayer::pauseGame(unfocused);
    }
};

#endif
