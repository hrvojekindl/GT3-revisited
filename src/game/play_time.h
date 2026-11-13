#pragma once

// A daily play-time limit (ours, for families): the minutes the game may be played each day. When
// today's time is up, a race being driven can still be finished; then the game says so until the
// next day (App, app/screen_time_up.cpp). The limit is in config.ini, written only when set:
//
//   [playtime]
//   daily_minutes = 30   ; none: no limit
//   locked = on          ; Settings shows the limit but can't change it (only this file can)
//
// and the day's count in playtime.ini beside it, which the game saves every 30 seconds while
// there is a limit (and when it quits):
//
//   [playtime]
//   date = 2026-11-13    ; the day `played` counts (local time)
//   played = 754         ; seconds played that day

#include <ctime>
#include <string>

namespace gt3r {

class IniDocument;

namespace game {

struct PlayTime {
    static constexpr int kMaxMinutes = 600;
    static constexpr double kWarnSeconds = 5 * 60; // the "minutes left" notice from here
    // How long past the limit a race being driven may go on to its finish: a free run or a time
    // trial has none, and an endurance race takes long.
    static constexpr double kRaceGraceSeconds = 10 * 60;

    int daily_minutes = 0;
    bool locked = false;
    std::string date;
    double played = 0;

    // `cfg`: config.ini's limit; `count`: playtime.ini's day and seconds.
    static PlayTime load(const IniDocument& cfg, const IniDocument& count);
    void save_limit(IniDocument& cfg) const; // removes the keys left at their defaults
    void save_count(IniDocument& count) const;

    bool limited() const { return daily_minutes > 0; }
    // `seconds` more played on `today` (YYYY-MM-DD): a new day starts again from zero.
    void add(double seconds, const std::string& today);
    // Seconds played on `today`: zero when the count is another day's.
    double played_on(const std::string& today) const { return date == today ? played : 0.0; }
    // Seconds left on `today` (a very large number without a limit; never below zero).
    double left(const std::string& today) const;
    bool over(const std::string& today) const { return limited() && left(today) <= 0; }
    // Whether a race being driven still gets to finish: over the limit by less than the grace.
    bool race_may_finish(const std::string& today) const {
        return played_on(today) - daily_minutes * 60.0 < kRaceGraceSeconds;
    }
};

// The local calendar day of `t`, as YYYY-MM-DD.
std::string local_date(std::time_t t);

// "12 min", "1 h 05 min" (rounded down to the minute).
std::string play_time_text(double seconds);

} // namespace game
} // namespace gt3r
