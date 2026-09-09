#include "TiltMazeActivity.h"

#include <Arduino.h>
#include <CrossPointSettings.h>
#include <GfxRenderer.h>
#include <HalTiltSensor.h>
#include <I18n.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr uint8_t WALL_N = 1 << 0;
constexpr uint8_t WALL_E = 1 << 1;
constexpr uint8_t WALL_S = 1 << 2;
constexpr uint8_t WALL_W = 1 << 3;
}  // namespace

void TiltMazeActivity::onEnter() {
  Activity::onEnter();
  srand(millis());
  generateMaze();
  ballRow = 0;
  ballCol = 0;
  halTiltSensor.resetRelativeTiltAngle();
  lastMoveMs = millis();
  requestUpdate();
}

void TiltMazeActivity::onExit() {
  Activity::onExit();
}

void TiltMazeActivity::generateMaze() {
  for (uint8_t& cell : wallGrid) {
    cell = WALL_N | WALL_E | WALL_S | WALL_W;
  }

  // Iterative randomized depth-first search (recursive backtracker). Kept
  // iterative with an explicit stack, rather than recursive, to avoid
  // recursion depth on the ESP32-C3's limited task stack.
  bool visited[CELL_COUNT] = {};
  uint8_t stackIdx[CELL_COUNT];  // CELL_COUNT (60) fits uint8_t
  int stackSize = 0;

  visited[0] = true;
  stackIdx[stackSize++] = 0;

  while (stackSize > 0) {
    const int current = stackIdx[stackSize - 1];
    const int cr = current / COLS;
    const int cc = current % COLS;

    int neighborDir[4];
    int neighborCount = 0;
    if (cr > 0 && !visited[current - COLS]) neighborDir[neighborCount++] = 0;         // N
    if (cc < COLS - 1 && !visited[current + 1]) neighborDir[neighborCount++] = 1;     // E
    if (cr < ROWS - 1 && !visited[current + COLS]) neighborDir[neighborCount++] = 2;  // S
    if (cc > 0 && !visited[current - 1]) neighborDir[neighborCount++] = 3;            // W

    if (neighborCount == 0) {
      stackSize--;
      continue;
    }

    const int dir = neighborDir[rand() % neighborCount];
    int next;
    switch (dir) {
      case 0:
        next = current - COLS;
        wallGrid[current] &= ~WALL_N;
        wallGrid[next] &= ~WALL_S;
        break;
      case 1:
        next = current + 1;
        wallGrid[current] &= ~WALL_E;
        wallGrid[next] &= ~WALL_W;
        break;
      case 2:
        next = current + COLS;
        wallGrid[current] &= ~WALL_S;
        wallGrid[next] &= ~WALL_N;
        break;
      default:
        next = current - 1;
        wallGrid[current] &= ~WALL_W;
        wallGrid[next] &= ~WALL_E;
        break;
    }

    visited[next] = true;
    stackIdx[stackSize++] = static_cast<uint8_t>(next);
  }
}

bool TiltMazeActivity::canMove(int row, int col, int dir) const {
  const int idx = row * COLS + col;
  return !(wallGrid[idx] & (1 << dir));
}

void TiltMazeActivity::handleTilt() {
  if (!halTiltSensor.isAvailable()) {
    return;
  }

  const unsigned long now = millis();
  if ((now - lastMoveMs) < MOVE_COOLDOWN_MS) {
    return;
  }

  float gx, gy;
  halTiltSensor.getRelativeTiltAngleDeg(gx, gy);

  // Map the raw PCB-frame gyro axes to screen-relative right/up axes using
  // the same per-orientation convention as HalTiltSensor's own reader
  // tilt-page-turn gesture mapping (lib/hal/HalTiltSensor.cpp:188-207): the X
  // axis reads left/right in portrait, the Y axis reads left/right in
  // landscape. The sign of the "up" axis (the other raw axis) is a
  // best-effort mirror of that convention and has NOT been verified on
  // hardware - see verification steps in chat.
  float rightVal;
  float upVal;
  switch (SETTINGS.orientation) {
    case CrossPointOrientation::PORTRAIT:
      rightVal = gx;
      upVal = gy;
      break;
    case CrossPointOrientation::INVERTED:
      rightVal = -gx;
      upVal = -gy;
      break;
    case CrossPointOrientation::LANDSCAPE_CW:
      rightVal = -gy;
      upVal = gx;
      break;
    case CrossPointOrientation::LANDSCAPE_CCW:
      rightVal = gy;
      upVal = -gx;
      break;
    default:
      rightVal = gx;
      upVal = gy;
      break;
  }

  int dir = -1;
  if (fabsf(rightVal) >= fabsf(upVal)) {
    if (fabsf(rightVal) > TILT_MOVE_THRESHOLD_DEG) {
      dir = rightVal > 0 ? 1 : 3;  // E : W
    }
  } else if (fabsf(upVal) > TILT_MOVE_THRESHOLD_DEG) {
    dir = upVal > 0 ? 0 : 2;  // N : S
  }

  if (dir < 0) {
    return;
  }

  // Discrete step: rebaseline immediately so the device must be tilted past
  // the threshold again to trigger another move, whether or not this one
  // was blocked by a wall (mirrors the debounce role of
  // HalTiltSensor's own COOLDOWN_MS for the reader's flick gesture).
  halTiltSensor.resetRelativeTiltAngle();
  lastMoveMs = now;

  if (!canMove(ballRow, ballCol, dir)) {
    return;
  }

  switch (dir) {
    case 0: ballRow--; break;
    case 1: ballCol++; break;
    case 2: ballRow++; break;
    default: ballCol--; break;
  }

  if (ballRow == goalRow && ballCol == goalCol) {
    onMazeSolved();
  } else {
    requestUpdate();
  }
}

void TiltMazeActivity::onMazeSolved() {
  {
    RenderLock lock;
    GUI.drawPopup(renderer, tr(STR_TILT_MAZE_SOLVED));
    renderer.displayBuffer();
  }
  delay(700);  // Let the popup be readable before the next maze appears
  generateMaze();
  ballRow = 0;
  ballCol = 0;
  halTiltSensor.resetRelativeTiltAngle();
  requestUpdate();
}

void TiltMazeActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    generateMaze();
    ballRow = 0;
    ballCol = 0;
    halTiltSensor.resetRelativeTiltAngle();
    requestUpdate();
    return;
  }

  handleTilt();
}

void TiltMazeActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto& metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_TILT_MAZE_TITLE));

  int marginTop, marginRight, marginBottom, marginLeft;
  renderer.getOrientedViewableTRBL(&marginTop, &marginRight, &marginBottom, &marginLeft);

  const int areaTop = metrics.topPadding + metrics.headerHeight + 15;
  const int areaBottom = pageHeight - metrics.buttonHintsHeight - std::max(marginBottom, 10);
  const int areaLeft = std::max(marginLeft, 10);
  const int areaRight = pageWidth - std::max(marginRight, 10);

  const int areaWidth = areaRight - areaLeft;
  const int areaHeight = areaBottom - areaTop - 30;  // leave room for the hint line below the grid

  const int cellSize = std::min(areaWidth / COLS, areaHeight / ROWS);
  const int mazeWidth = cellSize * COLS;
  const int mazeHeight = cellSize * ROWS;
  const int originX = areaLeft + (areaWidth - mazeWidth) / 2;
  const int originY = areaTop + (areaHeight - mazeHeight) / 2;

  constexpr int WALL_THICKNESS = 3;

  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      const int idx = r * COLS + c;
      const int x = originX + c * cellSize;
      const int y = originY + r * cellSize;

      if (r == 0) {
        renderer.drawLine(x, y, x + cellSize, y, WALL_THICKNESS, true);
      }
      if (c == 0) {
        renderer.drawLine(x, y, x, y + cellSize, WALL_THICKNESS, true);
      }
      if (wallGrid[idx] & WALL_E) {
        renderer.drawLine(x + cellSize, y, x + cellSize, y + cellSize, WALL_THICKNESS, true);
      }
      if (wallGrid[idx] & WALL_S) {
        renderer.drawLine(x, y + cellSize, x + cellSize, y + cellSize, WALL_THICKNESS, true);
      }
    }
  }

  // Goal marker: hollow rounded square.
  const int goalPad = cellSize / 5;
  renderer.drawRoundedRect(originX + goalCol * cellSize + goalPad, originY + goalRow * cellSize + goalPad,
                            cellSize - 2 * goalPad, cellSize - 2 * goalPad, 2, 4, true);

  // Ball: filled rounded square approximating a circle (GfxRenderer has no
  // dedicated circle primitive - see lib/GfxRenderer/GfxRenderer.h).
  const int ballPad = cellSize / 4;
  const int ballSize = cellSize - 2 * ballPad;
  renderer.fillRoundedRect(originX + ballCol * cellSize + ballPad, originY + ballRow * cellSize + ballPad, ballSize,
                            ballSize, ballSize / 2, Color::Black);

  const char* hint = halTiltSensor.isAvailable() ? tr(STR_TILT_MAZE_HINT) : tr(STR_TILT_MAZE_NO_SENSOR);
  renderer.drawCenteredText(UI_12_FONT_ID, areaBottom - 24, hint, true);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_NEW_MAZE), nullptr, nullptr);
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
