#include "TimerActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

unsigned long TimerActivity::TimerState::elapsedMs() const {
  if (state == RunState::Running) {
    return accumulatedMs + (millis() - startMs);
  }
  return accumulatedMs;
}

TimerActivity::TimerState& TimerActivity::activeTimer() { return tab == Tab::Hourglass ? hourglass : stopwatch; }

unsigned long TimerActivity::hourglassTotalMs() const {
  return static_cast<unsigned long>(PRESET_MINUTES[presetIndex]) * 60UL * 1000UL;
}

void TimerActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

bool TimerActivity::preventAutoSleep() {
  return hourglass.state == RunState::Running || stopwatch.state == RunState::Running;
}

void TimerActivity::handleConfirmTap() {
  TimerState& t = activeTimer();
  switch (t.state) {
    case RunState::Stopped:
      t.accumulatedMs = 0;
      t.startMs = millis();
      t.state = RunState::Running;
      lastTickMs = millis();
      break;
    case RunState::Running:
      t.accumulatedMs = t.elapsedMs();
      t.state = RunState::Paused;
      break;
    case RunState::Paused:
      t.startMs = millis();
      t.state = RunState::Running;
      lastTickMs = millis();
      break;
    case RunState::Finished:
      t.accumulatedMs = 0;
      t.state = RunState::Stopped;
      break;
  }
  requestUpdate();
}

void TimerActivity::handleResetActive() {
  TimerState& t = activeTimer();
  t.state = RunState::Stopped;
  t.accumulatedMs = 0;
  requestUpdate();
}

void TimerActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Left) ||
      mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    tab = (tab == Tab::Hourglass) ? Tab::Stopwatch : Tab::Hourglass;
    requestUpdate();
  }

  if (tab == Tab::Hourglass && hourglass.state == RunState::Stopped) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
      presetIndex = (presetIndex - 1 + PRESET_COUNT) % PRESET_COUNT;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
      presetIndex = (presetIndex + 1) % PRESET_COUNT;
      requestUpdate();
    }
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    confirmPressStartMs = millis();
    confirmLongHandled = false;
  }

  if (mappedInput.isPressed(MappedInputManager::Button::Confirm) && !confirmLongHandled &&
      (millis() - confirmPressStartMs) >= RESET_LONG_PRESS_MS) {
    confirmLongHandled = true;
    handleResetActive();
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) && !confirmLongHandled) {
    handleConfirmTap();
  }

  // Auto-complete the countdown regardless of which tab is currently shown,
  // so switching to the Stopwatch tab doesn't leave the hourglass running
  // past its preset duration.
  if (hourglass.state == RunState::Running && hourglass.elapsedMs() >= hourglassTotalMs()) {
    hourglass.accumulatedMs = hourglassTotalMs();
    hourglass.state = RunState::Finished;
    if (tab == Tab::Hourglass) requestUpdate();
  }

  // See TimerActivity.h for why this is throttled to once a second.
  const unsigned long now = millis();
  if (activeTimer().state == RunState::Running && now - lastTickMs >= 1000) {
    lastTickMs = now;
    requestUpdate();
  }
}

void TimerActivity::drawHourglassGlass(int cx, int topY, int midY, int bottomY, int halfWidth,
                                       float remainingFraction) const {
  // Outline: two triangles sharing the neck point at (cx, midY).
  renderer.drawLine(cx - halfWidth, topY, cx + halfWidth, topY, 2, true);
  renderer.drawLine(cx - halfWidth, topY, cx, midY, 2, true);
  renderer.drawLine(cx + halfWidth, topY, cx, midY, 2, true);
  renderer.drawLine(cx - halfWidth, bottomY, cx + halfWidth, bottomY, 2, true);
  renderer.drawLine(cx - halfWidth, bottomY, cx, midY, 2, true);
  renderer.drawLine(cx + halfWidth, bottomY, cx, midY, 2, true);

  // Remaining sand pools just above the neck as the top chamber empties, so
  // it renders as a triangle scaled toward the neck by remainingFraction.
  if (remainingFraction > 0.0f) {
    const int yTop = midY - static_cast<int>(remainingFraction * static_cast<float>(midY - topY));
    const int hwTop = static_cast<int>(static_cast<float>(halfWidth) * remainingFraction);
    const int px[3] = {cx - hwTop, cx + hwTop, cx};
    const int py[3] = {yTop, yTop, midY};
    renderer.fillPolygon(px, py, 3, true);
  }

  // Fallen sand piles up from the bottom; its top edge follows the chamber's
  // slanted walls, so it renders as a trapezoid clipped by a level line.
  const float fallenFraction = 1.0f - remainingFraction;
  if (fallenFraction > 0.0f) {
    const int yLevel = bottomY - static_cast<int>(fallenFraction * static_cast<float>(bottomY - midY));
    const float t = static_cast<float>(yLevel - midY) / static_cast<float>(bottomY - midY);
    const int hwLevel = static_cast<int>(static_cast<float>(halfWidth) * t);
    const int qx[4] = {cx - halfWidth, cx + halfWidth, cx + hwLevel, cx - hwLevel};
    const int qy[4] = {bottomY, bottomY, yLevel, yLevel};
    renderer.fillPolygon(qx, qy, 4, true);
  }
}

void TimerActivity::renderHourglass(int contentTop, int contentHeight, int pageWidth) {
  const int glassHeight = contentHeight * 6 / 10;
  const int glassHalfWidth = pageWidth / 6;
  const int topY = contentTop + (contentHeight - glassHeight) / 2;
  const int bottomY = topY + glassHeight;
  const int midY = (topY + bottomY) / 2;
  const int cx = pageWidth / 2;

  const unsigned long total = hourglassTotalMs();
  const unsigned long elapsed = std::min(hourglass.elapsedMs(), total);
  const float remainingFraction =
      total > 0 ? 1.0f - static_cast<float>(elapsed) / static_cast<float>(total) : 0.0f;

  drawHourglassGlass(cx, topY, midY, bottomY, glassHalfWidth, remainingFraction);

  const unsigned long remainingSec = (total - elapsed) / 1000;
  char timeBuf[16];
  snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", remainingSec / 60, remainingSec % 60);
  renderer.drawCenteredText(NOTOSANS_18_FONT_ID, bottomY + 24, timeBuf, true, EpdFontFamily::BOLD);

  const char* statusText = nullptr;
  if (hourglass.state == RunState::Paused) {
    statusText = tr(STR_TIMER_PAUSED);
  } else if (hourglass.state == RunState::Finished) {
    statusText = tr(STR_TIMER_TIMES_UP);
  }

  if (statusText) {
    renderer.drawCenteredText(NOTOSANS_14_FONT_ID, bottomY + 58, statusText);
  } else if (hourglass.state == RunState::Stopped) {
    char durBuf[32];
    snprintf(durBuf, sizeof(durBuf), "%d min", PRESET_MINUTES[presetIndex]);
    renderer.drawCenteredText(NOTOSANS_14_FONT_ID, bottomY + 58, durBuf);
  }
}

void TimerActivity::renderStopwatch(int contentTop, int contentHeight, int pageWidth) {
  (void)pageWidth;
  const unsigned long elapsed = stopwatch.elapsedMs();
  const unsigned long totalCentis = elapsed / 10;
  const unsigned long minutes = totalCentis / 6000;
  const unsigned long seconds = (totalCentis / 100) % 60;
  const unsigned long centis = totalCentis % 100;

  char buf[16];
  snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", minutes, seconds, centis);

  const int y = contentTop + contentHeight / 2 - 20;
  renderer.drawCenteredText(NOTOSANS_18_FONT_ID, y, buf, true, EpdFontFamily::BOLD);

  if (stopwatch.state == RunState::Paused) {
    renderer.drawCenteredText(NOTOSANS_14_FONT_ID, y + 40, tr(STR_TIMER_PAUSED));
  }
}

void TimerActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_TIMER_TITLE));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentBottom = pageHeight - metrics.buttonHintsHeight - metrics.verticalSpacing;
  const int contentHeight = contentBottom - contentTop;

  renderer.drawCenteredText(NOTOSANS_14_FONT_ID, contentTop,
                            tab == Tab::Hourglass ? tr(STR_TIMER_HOURGLASS_TAB) : tr(STR_TIMER_STOPWATCH_TAB));

  const int bodyTop = contentTop + 30;
  const int bodyHeight = contentHeight - 30 - 24;

  if (tab == Tab::Hourglass) {
    renderHourglass(bodyTop, bodyHeight, pageWidth);
  } else {
    renderStopwatch(bodyTop, bodyHeight, pageWidth);
  }

  const RunState activeState = activeTimer().state;
  if (tab == Tab::Hourglass && hourglass.state == RunState::Stopped) {
    GUI.drawHelpText(renderer, Rect{0, pageHeight - metrics.buttonHintsHeight - 24, pageWidth, 20},
                     tr(STR_TIMER_ADJUST_DURATION));
  } else if (activeState == RunState::Running || activeState == RunState::Paused) {
    GUI.drawHelpText(renderer, Rect{0, pageHeight - metrics.buttonHintsHeight - 24, pageWidth, 20},
                     tr(STR_TIMER_HOLD_TO_RESET));
  }

  const char* confirmLabel = tr(STR_TIMER_START);
  switch (activeState) {
    case RunState::Running:
      confirmLabel = tr(STR_TIMER_PAUSE);
      break;
    case RunState::Paused:
      confirmLabel = tr(STR_TIMER_RESUME);
      break;
    case RunState::Finished:
      confirmLabel = tr(STR_TIMER_RESET);
      break;
    case RunState::Stopped:
      confirmLabel = tr(STR_TIMER_START);
      break;
  }
  const char* otherTabName = tab == Tab::Hourglass ? tr(STR_TIMER_STOPWATCH_TAB) : tr(STR_TIMER_HOURGLASS_TAB);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), confirmLabel, otherTabName, otherTabName);
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  bool useFullRefresh = false;
  if (activeState == RunState::Running) {
    ticksSinceFullRefresh++;
    if (ticksSinceFullRefresh >= TICKS_PER_FULL_REFRESH) {
      ticksSinceFullRefresh = 0;
      useFullRefresh = true;
    }
  }
  renderer.displayBuffer(useFullRefresh ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
}
