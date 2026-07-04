#ifndef CONTROL_STATE_H
#define CONTROL_STATE_H

#include <atomic>
#include <optional>
#include <string_view>

enum class State {
    Active,
    Inactive,
    DisableTouchpad,
    ControlOnly,
};

using AtomicState = std::atomic<State>;

inline const char* stateToString(State s) {
    switch (s) {
        case State::Active: return "active";
        case State::Inactive: return "inactive";
        case State::DisableTouchpad: return "disable-touchpad";
        case State::ControlOnly: return "control-only";
    }
    return "unknown";
}

inline std::optional<State> stateFromString(std::string_view s) {
    if (s == "active") return State::Active;
    if (s == "inactive") return State::Inactive;
    if (s == "disable-touchpad") return State::DisableTouchpad;
    if (s == "control-only") return State::ControlOnly;
    return std::nullopt;
}

#endif
