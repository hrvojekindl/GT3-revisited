// The F1 keyboard help (input/key_help.h): the keys come from the players' bindings as they stand.

#include "test_framework.h"

#include "base/config.h"
#include "input/input_manager.h"
#include "input/key_help.h"

#include <SDL3/SDL_scancode.h>

#include <string>
#include <vector>

using namespace gt3r;

namespace {

std::string keys_for(const std::vector<KeyHelpSection>& help, const std::string& section, const std::string& action) {
    for (const KeyHelpSection& s : help)
        if (s.title == section)
            for (const KeyHelpRow& r : s.rows)
                if (r.action == action) return r.keys;
    return "";
}

} // namespace

TEST(key_help_defaults) {
    InputManager input; // no devices: the keyboard's defaults
    const std::vector<KeyHelpSection> p1 = key_help(input, 0);
    CHECK(p1.size() == 2 && p1[0].title == "Driving" && p1[1].title == "Menus");
    CHECK(keys_for(p1, "Driving", "Accelerate") == "W / Up");
    CHECK(keys_for(p1, "Driving", "Steer left") == "A / Left");
    CHECK(keys_for(p1, "Driving", "Look behind") == "Page Down");
    CHECK(keys_for(p1, "Driving", "Pause") == "Esc / P / Pause"); // Android's Back left out
    CHECK(keys_for(p1, "Menus", "Confirm (Cross)") == "Return / W");
    CHECK(keys_for(p1, "Menus", "Move") == "Up / Down / Left / Right");
    const std::vector<KeyHelpSection> p2 = key_help(input, 1);
    CHECK(keys_for(p2, "Driving", "Accelerate") == "I");
    CHECK(keys_for(p2, "Driving", "Steer right") == "L");
    CHECK(keys_for(p2, "Menus", "Confirm (Cross)") == "O");
    CHECK(keys_for(p2, "Driving", "Look left").empty()); // unbound: no row
}

// GT3 original: the race reads the menu pad, so the menu keys with their race meanings.
TEST(key_help_gt3_layout_and_rebinding) {
    InputManager input;
    input.set_race_layout(0, "gt3");
    const std::vector<KeyHelpSection> gt3 = key_help(input, 0);
    CHECK(keys_for(gt3, "Driving", "Accelerate") == "Return / W"); // Cross's keys
    CHECK(keys_for(gt3, "Driving", "Reverse") == "Esc / Backspace"); // Triangle's
    input.set_race_layout(0, "modern");
    PadTarget accel;
    InputSource t;
    CHECK(PadTarget::parse("Cross", accel) && InputSource::parse("key.T", t));
    CHECK(input.bind_race(0, accel, t, true));
    CHECK(keys_for(key_help(input, 0), "Driving", "Accelerate") == "T");
}

TEST(key_help_names) {
    CHECK(key_help_name(SDL_SCANCODE_ESCAPE) == "Esc");
    CHECK(key_help_name(SDL_SCANCODE_PAGEDOWN) == "Page Down");
    CHECK(key_help_name(SDL_SCANCODE_W) == "W");
    CHECK(key_help_name(SDL_SCANCODE_AC_BACK).empty());
}
