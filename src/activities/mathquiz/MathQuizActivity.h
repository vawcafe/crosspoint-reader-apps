#pragma once

#include "activities/Activity.h"

// Second CrossPoint mini app for kids: a math flashcard quiz. Shows a random
// arithmetic problem (addition/subtraction/multiplication, switched via
// Left/Right) with four multiple-choice answers navigated via Up/Down and
// submitted with Confirm. No persistence -- score resets whenever the app
// is re-entered or the operation type is switched, same scope as Dice.
class MathQuizActivity final : public Activity {
 public:
  explicit MathQuizActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("MathQuiz", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class Mode { Addition, Subtraction, Multiplication };
  static constexpr int MODE_COUNT = 3;
  static constexpr int CHOICE_COUNT = 4;

  Mode currentMode = Mode::Addition;
  int a = 0;
  int b = 0;
  int correctAnswer = 0;
  int choices[CHOICE_COUNT] = {};
  int correctChoiceIndex = 0;
  int selectedIndex = 0;

  int correctCount = 0;
  int totalCount = 0;

  void generateProblem();
  void submitAnswer();
  [[nodiscard]] const char* modeLabel(Mode mode) const;
};
