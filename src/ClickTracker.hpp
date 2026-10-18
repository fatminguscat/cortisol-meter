#pragma once

#include <chrono>
#include <deque>

namespace cm {

// Counts jump presses in the current level (fed by the GJBaseGameLayer::handleButton hook) and
// turns them into clicks per second over the last second.
class ClickTracker {
public:
    static void press();
    static float cps();
    static void clear();

private:
    using Clock = std::chrono::steady_clock;
    static std::deque<Clock::time_point>& presses();
};

}
