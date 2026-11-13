// Time's up: today's play time (game/play_time.h) has run out. The game stays here until there is
// time again (the next day, or a higher limit), and the player can quit; while the limit isn't
// locked, Settings can change it.

#include "app/app.h"
#include "app/screen_registry.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gt3r {

namespace {

class TimeUpScreen : public Screen {
public:
    void update(App& app, double) override {
        if (!app.play_time().over(App::today())) {
            app.pop(); // time again: back to the game
            return;
        }
        const std::vector<Item> items = shown(app);
        sel_ = std::clamp(sel_, 0, int(items.size()) - 1);
        const MenuInput& m = app.menu();
        sel_ = std::clamp(sel_ + m.vertical(), 0, int(items.size()) - 1);
        int chosen = m.pressed(PadButton::Cross) ? sel_ : -1;
        for (const PointerInput::Action& a : pointer_.take()) {
            const HitArea* h = hits_.at(a.x, a.y);
            if (!h) continue;
            if (a.kind == PointerInput::Action::Kind::Hover) sel_ = h->id;
            if (a.kind == PointerInput::Action::Kind::Click) chosen = h->id;
        }
        if (chosen < 0 || chosen >= int(items.size())) return;
        if (items[size_t(chosen)] == Item::Quit) app.quit();
        else app.push(make_settings_screen(kSettingsTabPlayTime));
    }

    void render(App& app, Ui& ui) override {
        const game::PlayTime& pt = app.play_time();
        const float w = ui.width(), h = ui.height();
        ui.fill(0, 0, w, h, colors::kBackground);
        const float cx = w / 2;
        float y = std::round(h * 0.22f);
        const char* title = "Time's up!";
        ui.text_big(std::round(cx - Ui::text_width(title, 3) / 2), y, title, 3, colors::kAccent);
        y += Ui::kChar * 3 + 22;
        const std::string played = "You've played " + game::play_time_text(pt.played_on(App::today())) +
                                   " today, and the limit is " + game::play_time_text(pt.daily_minutes * 60.0) + ".";
        ui.text_center(y, played);
        y += Ui::kChar + 8;
        ui.text_center(y, "See you tomorrow!", colors::kDim);
        y += Ui::kChar + 30;
        hits_.clear();
        const std::vector<Item> items = shown(app);
        const float bw = 170, bh = ui.touch() ? 30 : 22;
        for (size_t i = 0; i < items.size(); ++i) {
            const UiRect r{std::round(cx - bw / 2), y, bw, bh};
            ui.button(r, items[i] == Item::Quit ? "Quit the game" : "Change the limit (Settings)", int(i) == sel_,
                      false, items[i] == Item::Quit);
            hits_.add(r, int(i));
            y += bh + 8;
        }
        if (pt.locked)
            ui.text_center(y + 6, "The limit is locked: it can only be changed in config.ini.", colors::kDim);
    }

    void on_event(App&, const SDL_Event& e) override { pointer_.event(e); }
    bool adaptive_ui() const override { return true; }
    bool time_up() const override { return true; }
    bool clickable(float x, float y) const override { return hits_.at(x, y) != nullptr; }

private:
    enum class Item { Quit, Settings };
    static std::vector<Item> shown(const App& app) {
        if (app.play_time().locked) return {Item::Quit};
        return {Item::Quit, Item::Settings};
    }

    int sel_ = 0;
    PointerInput pointer_;
    HitList hits_;
};

} // namespace

std::unique_ptr<Screen> make_time_up_screen() { return std::make_unique<TimeUpScreen>(); }

GT3R_REGISTER_SCREEN_NO_DISC(time_up, "timeup", "Time's up",
                             "The daily play-time limit's screen (Settings > Play time).", 95,
                             [] { return make_time_up_screen(); });

} // namespace gt3r
