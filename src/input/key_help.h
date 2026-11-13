#pragma once

// The keyboard's controls as a player's bindings stand, for the F1 help overlay (App::draw_key_help):
// the race's (their race layout's, or the menu keys' GT3 meanings under "gt3") and the menus'.

#include <string>
#include <vector>

namespace gt3r {

class InputManager;

struct KeyHelpRow {
    std::string action; // "Accelerate", "Confirm (Cross)"
    std::string keys;   // "W / Up"
};

struct KeyHelpSection {
    std::string title; // "Driving", "Menus"
    std::vector<KeyHelpRow> rows;
};

// Driving, then Menus. Actions without a key are left out.
std::vector<KeyHelpSection> key_help(const InputManager& input, int player);

// A key as the overlay writes it ("Esc", "Page Down", "W"); empty for keys a computer keyboard
// doesn't have (Android's Back).
std::string key_help_name(int scancode);

} // namespace gt3r
