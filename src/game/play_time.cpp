#include "game/play_time.h"

#include "base/config.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace gt3r::game {

PlayTime PlayTime::load(const IniDocument& cfg, const IniDocument& count) {
    PlayTime p;
    if (auto v = cfg.get("playtime", "daily_minutes"))
        p.daily_minutes = std::clamp(std::atoi(v->c_str()), 0, kMaxMinutes);
    if (auto v = cfg.get("playtime", "locked")) p.locked = to_lower(*v) == "on";
    p.date = count.get("playtime", "date").value_or("");
    if (auto v = count.get("playtime", "played")) p.played = std::max(0.0, std::atof(v->c_str()));
    return p;
}

void PlayTime::save_limit(IniDocument& cfg) const {
    if (daily_minutes > 0) cfg.set("playtime", "daily_minutes", std::to_string(daily_minutes));
    else cfg.remove("playtime", "daily_minutes");
    if (locked) cfg.set("playtime", "locked", "on");
    else cfg.remove("playtime", "locked");
}

void PlayTime::save_count(IniDocument& count) const {
    count.set("playtime", "date", date);
    count.set("playtime", "played", std::to_string(int(played)));
}

void PlayTime::add(double seconds, const std::string& today) {
    if (date != today) date = today, played = 0;
    played += std::max(0.0, seconds);
}

double PlayTime::left(const std::string& today) const {
    if (!limited()) return 1e12;
    return std::max(0.0, daily_minutes * 60.0 - played_on(today));
}

std::string local_date(std::time_t t) {
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char b[16];
    std::snprintf(b, sizeof(b), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return b;
}

std::string play_time_text(double seconds) {
    const int minutes = int(std::max(0.0, seconds) / 60);
    char b[32];
    if (minutes < 60) std::snprintf(b, sizeof(b), "%d min", minutes);
    else std::snprintf(b, sizeof(b), "%d h %02d min", minutes / 60, minutes % 60);
    return b;
}

} // namespace gt3r::game
