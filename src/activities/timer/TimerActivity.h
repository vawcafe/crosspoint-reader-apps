#pragma once

#include "activities/Activity.h"

// Two mini apps in one: a countdown "hourglass" timer with a handful of
// preset durations, and a stopwatch with hundredths-of-a-second precision.
// Both read elapsed time as accumulatedMs + (millis() - startMs) while
// running, so pausing/resuming never drifts.
//
// Note: the panel's fastest refresh mode still takes ~500ms per paint (see
// src/main.cpp:435, "FAST_REFRESH (~500ms)"), so the visible readout is
// throttled to once a second while a timer runs -- repainting faster would
// only queue paints and burn battery for no visible benefit. The stopwatch's
// hundredths digits are still accurate; they just don't animate live.
//
// Activities are destroyed on exit (see CLAUDE.md, Activity Lifecycle), so
// leaving this app via Back stops the countdown/stopwatch -- there is no
// background timer that keeps running while reading a book.
class TimerActivity final : public Activity {
 public:
  explicit TimerActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Timer", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override;

 private:
  enum class Tab { Hourglass, Stopwatch };
  enum class RunState { Stopped, Running, Paused, Finished };

  struct TimerState {
    RunState state = RunState::Stopped;
    unsigned long startMs = 0;
    unsigned long accumulatedMs = 0;

    [[nodiscard]] unsigned long elapsedMs() const;
  };

  static constexpr unsigned long RESET_LONG_PRESS_MS = 600;
  static constexpr int PRESET_MINUTES[] = {1, 3, 5, 10, 15, 20, 30};
  static constexpr int PRESET_COUNT = sizeof(PRESET_MINUTES) / sizeof(PRESET_MINUTES[0]);
  // Full refresh every N running-ticks to clear FAST_REFRESH ghosting, same
  // technique EpubReaderActivity uses for repeated fast refreshes (see
  // src/activities/reader/EpubReaderActivity.cpp:891).
  static constexpr int TICKS_PER_FULL_REFRESH = 10;

  Tab tab = Tab::Hourglass;
  int presetIndex = 2;  // index into PRESET_MINUTES; default 5 minutes
  TimerState hourglass;
  TimerState stopwatch;

  unsigned long lastTickMs = 0;
  unsigned long confirmPressStartMs = 0;
  bool confirmLongHandled = false;
  int ticksSinceFullRefresh = 0;

  [[nodiscard]] TimerState& activeTimer();
  [[nodiscard]] unsigned long hourglassTotalMs() const;
  void handleConfirmTap();
  void handleResetActive();

  void renderHourglass(int contentTop, int contentHeight, int pageWidth);
  void renderStopwatch(int contentTop, int contentHeight, int pageWidth);
  void drawHourglassGlass(int cx, int topY, int midY, int bottomY, int halfWidth, float remainingFraction) const;
};
