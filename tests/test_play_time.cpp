// The daily play-time limit (game/play_time.h).

#include "test_framework.h"

#include "base/config.h"
#include "game/play_time.h"

#include <ctime>

using namespace gt3r;
using game::PlayTime;

TEST(play_time_counts_per_day) {
    PlayTime p;
    CHECK(!p.limited() && !p.over("2026-11-13") && p.left("2026-11-13") > 1e9); // no limit
    p.daily_minutes = 30;
    p.add(600, "2026-11-13");
    p.add(599, "2026-11-13");
    CHECK(p.played_on("2026-11-13") == 1199);
    CHECK(p.left("2026-11-13") == 30 * 60 - 1199 && !p.over("2026-11-13"));
    p.add(601, "2026-11-13");
    CHECK(p.over("2026-11-13") && p.left("2026-11-13") == 0);
    CHECK(p.played_on("2026-11-14") == 0 && !p.over("2026-11-14")); // a new day: time again
    p.add(10, "2026-11-14");
    CHECK(p.date == "2026-11-14" && p.played == 10);
    p.add(-5, "2026-11-14"); // never backwards
    CHECK(p.played == 10);
}

// A race being driven may go on to its finish, up to the grace past the limit.
TEST(play_time_race_grace) {
    PlayTime p;
    p.daily_minutes = 20;
    p.add(20 * 60 + PlayTime::kRaceGraceSeconds - 1, "2026-11-13");
    CHECK(p.over("2026-11-13") && p.race_may_finish("2026-11-13"));
    p.add(2, "2026-11-13");
    CHECK(!p.race_may_finish("2026-11-13"));
}

// The limit in config.ini (only when set), the day's count in playtime.ini.
TEST(play_time_config_roundtrip) {
    PlayTime p = PlayTime::load(IniDocument::parse("[playtime]\ndaily_minutes = 45\nlocked = ON\n"),
                                IniDocument::parse("[playtime]\ndate = 2026-11-13\nplayed = 754.6\n"));
    CHECK(p.daily_minutes == 45 && p.locked && p.date == "2026-11-13" && p.played > 754 && p.played < 755);
    IniDocument cfg, count;
    p.save_limit(cfg);
    p.save_count(count);
    CHECK(!cfg.get("playtime", "date") && !count.get("playtime", "daily_minutes")); // each in its own file
    PlayTime q = PlayTime::load(cfg, count);
    CHECK(q.daily_minutes == 45 && q.locked && q.date == "2026-11-13" && q.played == 754);
    // Back to no limit and unlocked: the keys leave config.ini.
    q.daily_minutes = 0, q.locked = false;
    q.save_limit(cfg);
    CHECK(!cfg.get("playtime", "daily_minutes") && !cfg.get("playtime", "locked"));
    const IniDocument none;
    CHECK(PlayTime::load(IniDocument::parse("[playtime]\ndaily_minutes = 99999\n"), none).daily_minutes ==
          PlayTime::kMaxMinutes);
    CHECK(PlayTime::load(IniDocument::parse("[playtime]\ndaily_minutes = -3\n"), none).daily_minutes == 0);
    CHECK(!PlayTime::load(none, none).limited());
}

TEST(play_time_text_and_date) {
    CHECK(game::play_time_text(0) == "0 min");
    CHECK(game::play_time_text(59) == "0 min");
    CHECK(game::play_time_text(12 * 60 + 30) == "12 min");
    CHECK(game::play_time_text(65 * 60) == "1 h 05 min");
    const std::string d = game::local_date(std::time(nullptr));
    CHECK(d.size() == 10 && d[4] == '-' && d[7] == '-');
}
