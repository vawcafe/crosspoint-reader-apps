#pragma once

#include <cstdint>

#include "activities/Activity.h"

// Tilt-controlled maze game: roll a ball from the top-left to the bottom-right
// corner of a randomly generated maze by physically tilting the device.
//
// Uses HalTiltSensor's generic relative-angle API (see
// HalTiltSensor::getRelativeTiltAngleDeg(), lib/hal/HalTiltSensor.h:136) rather
// than the reader's tilt-page-turn flick detector — movement is a discrete,
// one-cell-per-gesture step (not continuous/animated), consistent with the
// project's e-ink "no animations, static layout updates" constraint.
class TiltMazeActivity final : public Activity {
 private:
  static constexpr int COLS = 10;
  static constexpr int ROWS = 6;
  static constexpr int CELL_COUNT = COLS * ROWS;

  // Per-cell wall bitmask: bit0=N, bit1=E, bit2=S, bit3=W. A set bit means a
  // wall is present on that side of the cell. Sized for COLS*ROWS=60 cells
  // (60 bytes) - a member array, not a local/stack allocation.
  uint8_t wallGrid[CELL_COUNT] = {};

  int ballRow = 0;
  int ballCol = 0;
  int goalRow = ROWS - 1;
  int goalCol = COLS - 1;

  unsigned long lastMoveMs = 0;

  // Tuning constants for tilt-gesture-to-move mapping.
  // NOT verified on hardware yet - see chat response for the exact
  // verification steps (log TILT angle values via LOG_DBG and confirm the
  // 25 deg threshold feels right, and that directions aren't inverted).
  static constexpr float TILT_MOVE_THRESHOLD_DEG = 25.0f;
  static constexpr unsigned long MOVE_COOLDOWN_MS = 250;

  void generateMaze();
  bool canMove(int row, int col, int dir) const;
  void handleTilt();
  void onMazeSolved();

 public:
  explicit TiltMazeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("TiltMaze", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

  bool wantsTiltSensor() const override { return true; }
};
