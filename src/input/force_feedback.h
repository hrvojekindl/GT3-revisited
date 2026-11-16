#pragma once

// Native controller feedback beyond DS2 rumble: DualSense adaptive triggers,
// Xbox impulse triggers and wheel force feedback (SDL haptics).
//
// The race feeds a ForceFeedbackState every frame through
// InputManager::set_force_feedback(); InputManager routes it to the player's
// device. Effects stop by themselves when no state arrives for a moment
// (pause, menus, screen change). Configuration, [ffb] in config.ini:
//
//   enabled  = on      ; master switch
//   triggers = on      ; DualSense brake resistance and throttle buzz / Xbox impulse triggers
//   wheel    = on      ; wheel force feedback (spring + steering force)
//   strength = 1.0     ; overall wheel force (0..2)
//   spring   = 0.6     ; self-centring spring at speed (0..1)
//   trigger_force = 0.6 ; DualSense brake resistance (0..1)
//   invert   = off     ; flip the wheel's steering force direction
//   lightbar = on      ; DualSense / DualShock 4 light bar: the player's colour, a shift light while
//                      ; driving, an electric car's power meter (not under `enabled`: it is no force)

#include "base/bytes.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_haptic.h>
#include <SDL3/SDL_joystick.h>

#include <array>
#include <string>
#include <vector>

namespace gt3r {

class IniDocument;

struct ForceFeedbackState {
    float speed_kmh = 0;    // vehicle speed
    float brake = 0;        // brake pedal as applied, 0..1
    float abs_level = 0;    // 0..1 the ABS at work, or a wheel locking without it (VehicleTelemetry::abs_level)
    float steer_force = 0;  // -1..1 force the wheel should apply; positive turns it clockwise (right)
    float jolt = 0;         // 0..1 impact this frame (walls, cars)
    float traction = 0;     // 0..1 the driven wheels at the edge of their grip under throttle (VehicleTelemetry::traction_level)
    float understeer = 0;   // GT3's steering indicators: 0..1 the yaw rate short of what the steering asks
    float oversteer = 0;    // 0..1 the rear tyres past their slip threshold
    bool triggers_on = true; // the Vibration option: the triggers
    bool wheel_on = true;    // the GT FORCE Force Feedback option: the wheel
};

// The revs as the light bar shows them (InputManager::set_shift_light), sent every frame while a player drives.
struct ShiftLightState {
    float rpm = 0;           // engine speed
    float shift_rpm = 0;     // the HUD's shift point (HudData::shift_rpm); 0: no shift light
    bool rev_limiter = false; // on the fuel cut
    // A single-speed electric car shows its motor instead (motor = true): its output over its full power (0..1; below
    // 0 while it brakes the car) and its lift-off braking over its full braking (0..1).
    bool motor = false;
    float motor_power = 0, motor_braking = 0;
};

struct ForceFeedbackConfig {
    bool enabled = true, triggers = true, wheel = true, invert = false;
    bool lightbar = true;
    float strength = 1.0f, spring = 0.6f, trigger_force = 0.6f;
    float steering = 1.0f;   // the steering force's share of `strength`, 0..1 (0: the spring alone)
    float understeer = 0.5f; // Modern: how much of the wheel's force goes at full understeer, 0..1
    std::string wheel_mode = "auto"; // auto (modern) | modern | gt3

    bool gt3_wheel() const { return wheel_mode == "gt3"; }

    static ForceFeedbackConfig load(const IniDocument& ini);
    void save(IniDocument& ini) const;
};

namespace ffb {

// DualSense trigger effect blocks (11 bytes, SDL's DS5 effects packet layout).
using TriggerEffect = std::array<u8, 11>;
TriggerEffect trigger_off();
// Constant resistance from `start` (0..1 of the travel) with force 0..1.
TriggerEffect trigger_resistance(float start, float force);
// Vibration at `hz` with amplitude 0..1, from `start`.
TriggerEffect trigger_vibration(float start, int hz, float amplitude);
// What the brake and throttle triggers should do for a state (pure; unit-tested).
TriggerEffect brake_trigger_effect(const ForceFeedbackState& s, const ForceFeedbackConfig& c);
TriggerEffect throttle_trigger_effect(const ForceFeedbackState& s, const ForceFeedbackConfig& c);
// The DualSense effects packet for both triggers (SDL_SendGamepadEffect's payload) with the motors SDL
// plays in it, 0..255 each (0, 0: none), the way SDL's own packets carry them: `improved` for the
// rumble emulation of firmware 2.24 and newer, else the older one at half strength.
using Ds5Packet = std::array<u8, 47>;
Ds5Packet ds5_trigger_packet(const TriggerEffect& left, const TriggerEffect& right, u8 low, u8 high, bool improved);

// Wheel: spring coefficient and constant-force level, both 0..32767 / +-32767. GT3 original sends GT3's
// force alone, over a spring that doesn't change with speed; Modern firms the spring up with speed, adds
// a kick on impacts and lightens the wheel in understeer, and fades the spring in a slide.
int wheel_spring_coefficient(const ForceFeedbackState& s, const ForceFeedbackConfig& c);
int wheel_constant_level(const ForceFeedbackState& s, const ForceFeedbackConfig& c);
// GT3's steering indicators as the wheel plays them, a step of `dt` seconds on from `felt`: they jump
// tick to tick (0 to 0.65 and back at a turn-in, where the yaw rate lags the steering).
float eased_indicator(float felt, float indicator, float dt);
// Whether the wheel's effects, started at `started` seconds, play them again at `now`. SDL's DirectInput
// (Windows) and macOS backends turn an infinite length into 4294966296 microseconds, not the API's
// INFINITE; each wheel's driver then fits that into its own duration field, where it can come out
// ~30 s (issue 276). Linux and SDL's Logitech driver play it as infinite: never due there.
constexpr double kWheelRerunSeconds = 5.0;
bool wheel_rerun_due(double started, double now);

// Xbox impulse trigger strengths 0..1, from a level the motors can play: the brake's at time `t` seconds (the ABS
// level, throbbing at 10 Hz), the throttle's (the traction level).
float impulse_brake(const ForceFeedbackState& s, double t);
float impulse_throttle(const ForceFeedbackState& s);

struct LedColor {
    u8 r = 0, g = 0, b = 0;
    bool operator==(const LedColor&) const = default;
};
// A player's own light bar colour, dim as SDL's: player 1 blue, player 2 pink (a PlayStation's second
// pad is red, which the shift light flashes).
LedColor player_led(int player);
// The light bar as a shift light: the player's colour up to 60 % of the shift point, then green, yellow and
// red as the revs climb (in 12 steps, so the pad isn't sent a colour every frame), flashing red at 8 Hz
// from the shift point and on the rev limiter. `time` in seconds. With `motor` set, motor_light_led instead.
LedColor shift_light_led(const ShiftLightState& s, int player, double time);
// A single-speed electric car's light bar: a power meter in the player's own colour, which brightens with the
// motor's output from the player's dim colour at none to four times as bright at full power; while the motor
// brakes the car, green is added to the player's colour with the braking (in 12 steps each, as the shift
// light). It never flashes: there is no shift to call, and the top speed needs nothing done.
LedColor motor_light_led(const ShiftLightState& s, int player);

} // namespace ffb

// Device side: owns haptic handles and avoids resending unchanged effects.
class ForceFeedback {
public:
    ~ForceFeedback();
    void configure(const ForceFeedbackConfig& c) { cfg_ = c; }
    const ForceFeedbackConfig& config() const { return cfg_; }

    // `brake_trigger`, `throttle_trigger`: SDL_GAMEPAD_AXIS_LEFT/RIGHT_TRIGGER bound to the brake
    // and the throttle, or -1. `joystick` is used for haptics when `gamepad` is null (wheels).
    void apply(SDL_JoystickID id, SDL_Gamepad* gamepad, SDL_Joystick* joystick, int brake_trigger,
               int throttle_trigger, const ForceFeedbackState& s, double time);
    void stop(SDL_JoystickID id);
    void forget(SDL_JoystickID id); // device removed (handles closed by SDL)
    // The motors SDL plays on a gamepad from now on (InputManager::set_rumble, 0..255 each): the
    // DualSense's trigger packets carry them.
    void rumbling(SDL_JoystickID id, u8 low, u8 high);
    // Whether the wheel's force effects are ours (a spring or constant force made). Joystick rumble
    // must not reach it then: SDL's DirectInput and macOS rumble resets the device's force
    // feedback the first time it runs, which removes those effects.
    bool drives_wheel(SDL_JoystickID id) const;
    // Whether a joystick plays a wheel's forces, a spring and a constant force. A rumble motor
    // doesn't, whatever a driver says of force feedback: this tells a wheel from a pad SDL has no
    // mapping for (is_wheel). Opens the haptic device and closes it again.
    static bool has_wheel_forces(SDL_Joystick* joystick);
    // The light bar of `player`'s gamepad (pads without one are left alone): the shift light for `s`,
    // or the player's colour when `s` is null (not driving). Only sent when the colour changes.
    void light(SDL_JoystickID id, SDL_Gamepad* gamepad, int player, const ShiftLightState* s, double time);

private:
    struct Dev {
        SDL_JoystickID id = 0;
        SDL_Gamepad* gamepad = nullptr;
        bool active = false;
        bool sent_triggers = false;  // a DS5 trigger packet is in effect
        ffb::TriggerEffect sent_left{}, sent_right{}; // its trigger blocks
        u8 rumble_low = 0, rumble_high = 0; // the motors SDL plays (rumbling), since rumble_ticks
        Uint64 rumble_ticks = 0;            // SDL_GetTicks()
        bool impulse_on = false;
        SDL_Haptic* haptic = nullptr;
        bool haptic_tried = false;
        float understeer = 0, oversteer = 0; // ffb::eased_indicator
        double eased_at = 0;                 // their time
        double effects_at = 0;               // when the wheel's effects last (re)started (ffb::wheel_rerun_due)
        int spring_id = -1, constant_id = -1;
        int spring_coef = -1, constant_level = 0x7fffffff;
        bool has_led = false; // the pad has an RGB light bar, as of led_asked
        double led_asked = -1e9;
        bool led_sent = false;
        ffb::LedColor led; // the colour sent
    };
    Dev& dev(SDL_JoystickID id);
    void stop(Dev& d);
    void send_triggers(Dev& d, const ffb::TriggerEffect& left, const ffb::TriggerEffect& right);
    void apply_wheel(Dev& d, SDL_Joystick* joystick, const ForceFeedbackState& s, double time, bool restart = false);

    ForceFeedbackConfig cfg_;
    std::vector<Dev> devs_;
};

} // namespace gt3r
