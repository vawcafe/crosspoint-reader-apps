#include "AppRegistry.h"

// ---------------------------------------------------------------------------
// Optional mini apps: each one below is gated by an OMIT_APP_<NAME> build
// flag, following the existing OMIT_FONTS convention (see src/main.cpp).
// Core apps (Browse Files, Recent Books, OPDS, File Transfer, Settings)
// are not gated -- they're not optional.
//
// To build without an app, add its flag to platformio.local.ini (gitignored
// personal overrides -- see CLAUDE.md "Local Development Configuration"):
//
//   [env:default]
//   build_flags =
//     ${base.build_flags}
//     -DOMIT_APP_CHESS=1
//     -DOMIT_APP_SUDOKU=1
//
// Available flags (all apps are included by default):
//   OMIT_APP_CALCULATOR   Calculator
//   OMIT_APP_WEATHER      Weather
//   OMIT_APP_DUCKDUCKGO   DuckDuckGo search
//   OMIT_APP_WIKIPEDIA    Wikipedia search
//   OMIT_APP_RSS          RSS Feed reader
//   OMIT_APP_CHESS        Chess
//   OMIT_APP_SUDOKU       Sudoku
//   OMIT_APP_DICE         Dice
//   OMIT_APP_TILTMAZE     Tilt Maze (tilt-controlled maze game)
//   OMIT_APP_TODO         To-Do List
//   OMIT_APP_TIMER        Timer (hourglass countdown + stopwatch)
//   OMIT_APP_MATHQUIZ     Math Quiz (flashcards)
//
// This only removes the app from AppRegistry's menu. Whether flash usage
// actually drops depends on the linker's --gc-sections dead-code-elimination
// picking up the now-unreferenced Activity subclass; it is not guaranteed.
// ---------------------------------------------------------------------------

#ifndef OMIT_APP_CALCULATOR
#include "activities/calculator/CalculatorActivity.h"
#endif
#ifndef OMIT_APP_CHESS
#include "activities/chess/ChessActivity.h"
#endif
#ifndef OMIT_APP_DICE
#include "activities/dice/DiceActivity.h"
#endif
#ifndef OMIT_APP_DUCKDUCKGO
#include "activities/duckduckgo/DuckDuckGoActivity.h"
#endif
#ifndef OMIT_APP_MATHQUIZ
#include "activities/mathquiz/MathQuizActivity.h"
#endif
#ifndef OMIT_APP_RSS
#include "activities/rss/RssActivity.h"
#endif
#ifndef OMIT_APP_SUDOKU
#include "activities/sudoku/SudokuActivity.h"
#endif
#ifndef OMIT_APP_TILTMAZE
#include "activities/tiltmaze/TiltMazeActivity.h"
#endif
#ifndef OMIT_APP_TIMER
#include "activities/timer/TimerActivity.h"
#endif
#ifndef OMIT_APP_TODO
#include "activities/todo/TodoActivity.h"
#endif
#ifndef OMIT_APP_WEATHER
#include "activities/weather/WeatherActivity.h"
#endif
#ifndef OMIT_APP_WIKIPEDIA
#include "activities/wikipedia/WikipediaActivity.h"
#endif

// System Activities
#include "I18n.h"
#include "OpdsServerStore.h"
#include "activities/browser/OpdsBookBrowserActivity.h"
#include "activities/home/FileBrowserActivity.h"
#include "activities/home/RecentBooksActivity.h"
#include "activities/network/CrossPointWebServerActivity.h"
#include "activities/settings/OpdsServerListActivity.h"
#include "activities/settings/SettingsActivity.h"

AppRegistry &AppRegistry::getInstance() {
  static AppRegistry instance;
  return instance;
}

AppRegistry::AppRegistry() {
  // Browse Files
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_BROWSE_FILES); }, UIIcon::Folder,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<FileBrowserActivity>(r, i);
      }));

  // Recent Books
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_MENU_RECENT_BOOKS); }, UIIcon::Recent,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<RecentBooksActivity>(r, i);
      }));

  // OPDS Browser (conditionally visible)
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_OPDS_BROWSER); }, UIIcon::Library,
      [](GfxRenderer &r, MappedInputManager &i) -> std::unique_ptr<Activity> {
        const auto &servers = OPDS_STORE.getServers();
        if (servers.size() == 1) {
          return std::make_unique<OpdsBookBrowserActivity>(r, i, servers[0]);
        } else {
          return std::make_unique<OpdsServerListActivity>(r, i, true);
        }
      },
      []() { return OPDS_STORE.hasServers(); }));

  // File Transfer
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_FILE_TRANSFER); }, UIIcon::Transfer,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<CrossPointWebServerActivity>(r, i);
      }));

  // Settings
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_SETTINGS_TITLE); }, UIIcon::Settings,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<SettingsActivity>(r, i);
      }));

#ifndef OMIT_APP_CALCULATOR
  // Calculator App
  apps.push_back(
      std::make_unique<App>("Calculator", UIIcon::Calculator,
                            [](GfxRenderer &r, MappedInputManager &i) {
                              return std::make_unique<CalculatorActivity>(r, i);
                            }));
#endif

#ifndef OMIT_APP_WEATHER
  // Weather App
  apps.push_back(std::make_unique<App>(
      "Weather", UIIcon::Weather, [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<WeatherActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_DUCKDUCKGO
  // DuckDuckGo App
  apps.push_back(
      std::make_unique<App>("DuckDuckGo", UIIcon::DuckDuckGo,
                            [](GfxRenderer &r, MappedInputManager &i) {
                              return std::make_unique<DuckDuckGoActivity>(r, i);
                            }));
#endif

#ifndef OMIT_APP_WIKIPEDIA
  // Wikipedia App
  apps.push_back(
      std::make_unique<App>("Wikipedia", UIIcon::Wikipedia,
                            [](GfxRenderer &r, MappedInputManager &i) {
                              return std::make_unique<WikipediaActivity>(r, i);
                            }));
#endif

#ifndef OMIT_APP_RSS
  // RSS Feed App
  apps.push_back(std::make_unique<App>(
      "RSS Feed", UIIcon::Rss, [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<RssActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_CHESS
  // Chess App
  apps.push_back(std::make_unique<App>(
      "Chess", UIIcon::Chess, [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<ChessActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_SUDOKU
  // Sudoku App
  apps.push_back(std::make_unique<App>(
      "Sudoku", UIIcon::Sudoku, [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<SudokuActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_DICE
  // Dice App
  apps.push_back(std::make_unique<App>(
      "Dice", UIIcon::Dice, [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<DiceActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_TILTMAZE
  // Tilt Maze App
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_TILT_MAZE_TITLE); }, UIIcon::Dice,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<TiltMazeActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_TODO
  // To-Do List App
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_TODO_LIST_TITLE); }, UIIcon::Text,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<TodoActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_TIMER
  // Timer App (hourglass countdown + stopwatch)
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_TIMER_TITLE); }, UIIcon::Clock,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<TimerActivity>(r, i);
      }));
#endif

#ifndef OMIT_APP_MATHQUIZ
  // Math Quiz App (flashcards for kids)
  apps.push_back(std::make_unique<App>(
      []() { return tr(STR_MATH_QUIZ_TITLE); }, UIIcon::Calculator,
      [](GfxRenderer &r, MappedInputManager &i) {
        return std::make_unique<MathQuizActivity>(r, i);
      }));
#endif
}
