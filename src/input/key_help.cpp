#include "input/key_help.h"

#include "input/input_manager.h"

#include <SDL3/SDL_keyboard.h>

#include <algorithm>
#include <string_view>

namespace gt3r {

namespace {

struct Entry {
    std::string_view label;
    std::vector<std::string_view> targets; // PadTarget tokens, keys joined in this order
};

// The race pad's buttons keep GT3's meaning in every layout (race_action_name).
const std::vector<Entry>& driving_entries() {
    static const std::vector<Entry> k = {
        {"Steer left", {"LX-", "Left"}},  {"Steer right", {"LX+", "Right"}}, {"Accelerate", {"Cross"}},
        {"Brake", {"Square"}},            {"Handbrake", {"Circle"}},          {"Shift up", {"R2"}},
        {"Shift down", {"L2"}},           {"Reverse", {"Triangle"}},          {"Change view", {"R1"}},
        {"Look back", {"L1"}},            {"Look left", {"RX-"}},             {"Look right", {"RX+"}},
        {"Look behind", {"RY+"}},         {"Pause", {"Start"}},
    };
    return k;
}

const std::vector<Entry>& menu_entries() {
    static const std::vector<Entry> k = {
        {"Move", {"Up", "Down", "Left", "Right"}},
        {"Confirm (Cross)", {"Cross"}},
        {"Back (Triangle)", {"Triangle"}},
        {"Circle", {"Circle"}},
        {"Square", {"Square"}},
        {"L1 / R1", {"L1", "R1"}},
        {"L2 / R2", {"L2", "R2"}},
        {"Start", {"Start"}},
        {"Select", {"Select"}},
    };
    return k;
}

KeyHelpSection section(std::string title, const Profile& keys, const std::vector<Entry>& entries) {
    KeyHelpSection s{std::move(title), {}};
    for (const Entry& e : entries) {
        std::vector<std::string> names;
        for (std::string_view token : e.targets) {
            PadTarget t;
            if (!PadTarget::parse(token, t)) continue;
            for (const Binding& b : keys.bindings) {
                if (!(b.target == t) || b.source.kind != InputSource::Kind::Key) continue;
                std::string n = key_help_name(b.source.code);
                if (!n.empty() && std::find(names.begin(), names.end(), n) == names.end()) names.push_back(n);
            }
        }
        if (names.empty()) continue;
        std::string joined;
        for (const std::string& n : names) joined += (joined.empty() ? "" : " / ") + n;
        s.rows.push_back({std::string(e.label), joined});
    }
    return s;
}

} // namespace

std::string key_help_name(int scancode) {
    const SDL_Scancode sc = SDL_Scancode(scancode);
    if (sc == SDL_SCANCODE_AC_BACK) return "";
    if (sc == SDL_SCANCODE_ESCAPE) return "Esc";
    if (sc == SDL_SCANCODE_PAGEUP) return "Page Up";
    if (sc == SDL_SCANCODE_PAGEDOWN) return "Page Down";
    const char* n = SDL_GetScancodeName(sc);
    return n ? n : "";
}

std::vector<KeyHelpSection> key_help(const InputManager& input, int player) {
    const Profile& menu = input.bindings(player, false, BindingSet::Keyboard);
    // Under "gt3" the race reads the menu pad: the menu keys, with the race's meanings.
    const Profile& race = input.race_uses_menu_pad(player) ? menu : input.bindings(player, true, BindingSet::Keyboard);
    return {section("Driving", race, driving_entries()), section("Menus", menu, menu_entries())};
}

} // namespace gt3r
