#include "ClickTracker.hpp"

namespace cm {

namespace {
constexpr auto kWindow = std::chrono::seconds(1);
constexpr size_t kMaxKept = 256;
}

std::deque<ClickTracker::Clock::time_point>& ClickTracker::presses() {
    static std::deque<Clock::time_point> list;
    return list;
}

void ClickTracker::press() {
    auto& list = presses();
    list.push_back(Clock::now());
    if (list.size() > kMaxKept) list.pop_front();
}

float ClickTracker::cps() {
    auto& list = presses();
    auto const cutoff = Clock::now() - kWindow;
    while (!list.empty() && list.front() < cutoff) list.pop_front();
    return static_cast<float>(list.size());
}

void ClickTracker::clear() {
    presses().clear();
}

}
