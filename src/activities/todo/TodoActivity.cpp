#include "TodoActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <string>

#include "MappedInputManager.h"
#include "TodoStore.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

int TodoActivity::getItemCount() const {
  // +1 for the virtual "Add Item" row (same trick as OpdsServerListActivity).
  return static_cast<int>(TODO_STORE.getCount()) + 1;
}

void TodoActivity::onEnter() {
  Activity::onEnter();
  TODO_STORE.loadFromFile();
  selectedIndex = 0;
  requestUpdate();
}

void TodoActivity::onExit() {
  Activity::onExit();
}

void TodoActivity::handleConfirmTap() {
  const int todoCount = static_cast<int>(TODO_STORE.getCount());

  if (selectedIndex < todoCount) {
    TODO_STORE.toggleDone(static_cast<size_t>(selectedIndex));
    requestUpdate();
    return;
  }

  // Selected the virtual "Add Item" row.
  auto handler = [this](const ActivityResult& result) {
    if (!result.isCancelled) {
      const auto& kb = std::get<KeyboardResult>(result.data);
      if (!kb.text.empty()) {
        TODO_STORE.addItem(kb.text);
      }
    }
    requestUpdate();
  };
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_NEW_TODO), "", 100, InputType::Text),
      handler);
}

void TodoActivity::handleDeleteSelected() {
  const int todoCount = static_cast<int>(TODO_STORE.getCount());
  if (selectedIndex >= todoCount) {
    return;  // can't delete the virtual "Add Item" row
  }

  {
    RenderLock lock;
    GUI.drawPopup(renderer, tr(STR_TODO_DELETED));
    renderer.displayBuffer();
  }
  delay(400);  // Let the popup be readable before the list redraws

  TODO_STORE.removeItem(static_cast<size_t>(selectedIndex));

  const int newItemCount = getItemCount();
  if (selectedIndex >= newItemCount) {
    selectedIndex = newItemCount - 1;
  }
  requestUpdate();
}

void TodoActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    confirmPressStartMs = millis();
    confirmLongHandled = false;
  }

  if (mappedInput.isPressed(MappedInputManager::Button::Confirm) && !confirmLongHandled &&
      (millis() - confirmPressStartMs) >= DELETE_LONG_PRESS_MS) {
    confirmLongHandled = true;
    handleDeleteSelected();
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm) && !confirmLongHandled) {
    handleConfirmTap();
  }

  const int itemCount = getItemCount();
  buttonNavigator.onNext([this, itemCount] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, itemCount);
    requestUpdate();
  });
  buttonNavigator.onPrevious([this, itemCount] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, itemCount);
    requestUpdate();
  });
}

void TodoActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_TODO_LIST_TITLE));

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing * 2 - 20;

  const auto& items = TODO_STORE.getItems();
  const int todoCount = static_cast<int>(items.size());
  const int itemCount = todoCount + 1;

  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, itemCount, selectedIndex,
      [&items, todoCount](int index) -> std::string {
        if (index < todoCount) {
          return items[index].text;
        }
        return std::string(tr(STR_ADD_TODO));
      },
      nullptr, nullptr,
      [&items, todoCount](int index) -> std::string {
        if (index < todoCount) {
          return items[index].done ? "[x]" : "[ ]";
        }
        return std::string("");
      },
      false, [&items, todoCount](int index) -> bool { return index < todoCount && items[index].done; });

  GUI.drawHelpText(renderer, Rect{0, pageHeight - metrics.buttonHintsHeight - 24, pageWidth, 20},
                   tr(STR_TODO_HOLD_TO_DELETE));

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_TOGGLE), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
