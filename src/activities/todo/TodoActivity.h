#pragma once

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// Simple checklist / to-do list app. Items persist as JSON on the SD card
// via TodoStore (src/TodoStore.h), following the same store pattern as
// OpdsServerStore. No due dates: the device has no persisted calendar date
// (HalClock only exposes hour:minute, see lib/hal/HalClock.h), so this is a
// plain checklist rather than a scheduled reminders/calendar app.
class TodoActivity final : public Activity {
 private:
  ButtonNavigator buttonNavigator;
  int selectedIndex = 0;

  unsigned long confirmPressStartMs = 0;
  bool confirmLongHandled = false;
  static constexpr unsigned long DELETE_LONG_PRESS_MS = 600;

  int getItemCount() const;
  void handleConfirmTap();
  void handleDeleteSelected();

 public:
  explicit TodoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Todo", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
