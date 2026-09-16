#include "MathQuizActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdlib>
#include <string>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

void MathQuizActivity::onEnter() {
  Activity::onEnter();
  srand(millis());
  correctCount = 0;
  totalCount = 0;
  generateProblem();
  requestUpdate();
}

const char* MathQuizActivity::modeLabel(Mode mode) const {
  switch (mode) {
    case Mode::Addition:
      return tr(STR_MATH_ADDITION);
    case Mode::Subtraction:
      return tr(STR_MATH_SUBTRACTION);
    case Mode::Multiplication:
      return tr(STR_MATH_MULTIPLICATION);
  }
  return "";
}

void MathQuizActivity::generateProblem() {
  switch (currentMode) {
    case Mode::Addition:
      a = 1 + rand() % 20;
      b = 1 + rand() % 20;
      correctAnswer = a + b;
      break;
    case Mode::Subtraction:
      a = 1 + rand() % 20;
      b = rand() % a;  // 0..a-1, so the result is never negative
      correctAnswer = a - b;
      break;
    case Mode::Multiplication:
      a = 1 + rand() % 10;
      b = 1 + rand() % 10;
      correctAnswer = a * b;
      break;
  }

  // Pick 3 unique, non-negative distractors near the correct answer.
  int values[CHOICE_COUNT];
  values[0] = correctAnswer;
  int count = 1;
  int attempts = 0;
  while (count < CHOICE_COUNT && attempts < 50) {
    attempts++;
    const int delta = (rand() % 10) - 5;  // -5..4, excluding 0
    if (delta == 0) continue;
    const int candidate = correctAnswer + delta;
    if (candidate < 0) continue;
    bool duplicate = false;
    for (int i = 0; i < count; i++) {
      if (values[i] == candidate) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) values[count++] = candidate;
  }
  // Fallback for the rare case the random search above didn't find enough
  // unique values (tiny answer ranges): scan upward deterministically.
  int nextCandidate = correctAnswer + 1;
  while (count < CHOICE_COUNT) {
    bool duplicate = false;
    for (int i = 0; i < count; i++) {
      if (values[i] == nextCandidate) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) values[count++] = nextCandidate;
    nextCandidate++;
  }

  // Fisher-Yates shuffle so the correct answer isn't always choice 0.
  for (int i = CHOICE_COUNT - 1; i > 0; i--) {
    const int j = rand() % (i + 1);
    std::swap(values[i], values[j]);
  }
  for (int i = 0; i < CHOICE_COUNT; i++) {
    choices[i] = values[i];
    if (values[i] == correctAnswer) correctChoiceIndex = i;
  }
  selectedIndex = 0;
}

void MathQuizActivity::submitAnswer() {
  totalCount++;
  const bool isCorrect = (selectedIndex == correctChoiceIndex);
  if (isCorrect) correctCount++;

  {
    RenderLock lock;
    if (isCorrect) {
      GUI.drawPopup(renderer, tr(STR_MATH_CORRECT));
    } else {
      char buf[32];
      snprintf(buf, sizeof(buf), tr(STR_MATH_WRONG_FORMAT), correctAnswer);
      GUI.drawPopup(renderer, buf);
    }
    renderer.displayBuffer();
  }
  delay(isCorrect ? 400 : 900);  // longer pause on a miss so the answer can be read

  generateProblem();
  requestUpdate();
}

void MathQuizActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    currentMode = static_cast<Mode>((static_cast<int>(currentMode) - 1 + MODE_COUNT) % MODE_COUNT);
    correctCount = 0;
    totalCount = 0;
    generateProblem();
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Right)) {
    currentMode = static_cast<Mode>((static_cast<int>(currentMode) + 1) % MODE_COUNT);
    correctCount = 0;
    totalCount = 0;
    generateProblem();
    requestUpdate();
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    selectedIndex = (selectedIndex - 1 + CHOICE_COUNT) % CHOICE_COUNT;
    requestUpdate();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    selectedIndex = (selectedIndex + 1) % CHOICE_COUNT;
    requestUpdate();
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    submitAnswer();
  }
}

void MathQuizActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_MATH_QUIZ_TITLE));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentBottom = pageHeight - metrics.buttonHintsHeight - metrics.verticalSpacing;
  const int contentHeight = contentBottom - contentTop;

  renderer.drawCenteredText(NOTOSANS_14_FONT_ID, contentTop, modeLabel(currentMode));

  const char* fmt = tr(STR_MATH_ADD_FORMAT);
  if (currentMode == Mode::Subtraction) {
    fmt = tr(STR_MATH_SUB_FORMAT);
  } else if (currentMode == Mode::Multiplication) {
    fmt = tr(STR_MATH_MUL_FORMAT);
  }
  char problemBuf[32];
  snprintf(problemBuf, sizeof(problemBuf), fmt, a, b);
  renderer.drawCenteredText(NOTOSANS_18_FONT_ID, contentTop + 30, problemBuf, true, EpdFontFamily::BOLD);

  char scoreBuf[32];
  snprintf(scoreBuf, sizeof(scoreBuf), tr(STR_MATH_SCORE_FORMAT), correctCount, totalCount);
  renderer.drawCenteredText(NOTOSANS_14_FONT_ID, contentTop + 65, scoreBuf);

  const int listTop = contentTop + 100;
  const int listHeight = contentHeight - 100 - 24;
  GUI.drawList(renderer, Rect{0, listTop, pageWidth, listHeight}, CHOICE_COUNT, selectedIndex,
              [this](int index) -> std::string { return std::to_string(choices[index]); });

  GUI.drawHelpText(renderer, Rect{0, pageHeight - metrics.buttonHintsHeight - 24, pageWidth, 20},
                   tr(STR_MATH_CHANGE_TYPE));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_MATH_SUBMIT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
