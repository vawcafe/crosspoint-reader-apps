#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct TodoItem {
  std::string text;
  bool done = false;
};

class TodoStore;
namespace JsonSettingsIO {
bool saveTodos(const TodoStore& store, const char* path);
bool loadTodos(TodoStore& store, const char* json);
}  // namespace JsonSettingsIO

// Singleton persisting the user's to-do list as JSON on the SD card.
// Follows the same save/load-on-every-mutation pattern as OpdsServerStore
// (see src/OpdsServerStore.h) — writes are already deliberate, discrete user
// actions (add/toggle/remove), not per-frame or per-keystroke, so no extra
// debouncing is needed on top of that.
class TodoStore {
 private:
  static TodoStore instance;
  std::vector<TodoItem> items;

  static constexpr size_t MAX_ITEMS = 50;

  TodoStore() = default;

  friend bool JsonSettingsIO::saveTodos(const TodoStore&, const char*);
  friend bool JsonSettingsIO::loadTodos(TodoStore&, const char*);

 public:
  TodoStore(const TodoStore&) = delete;
  TodoStore& operator=(const TodoStore&) = delete;

  static TodoStore& getInstance() { return instance; }

  bool saveToFile() const;
  bool loadFromFile();

  bool addItem(const std::string& text);
  bool toggleDone(size_t index);
  bool removeItem(size_t index);

  const std::vector<TodoItem>& getItems() const { return items; }
  size_t getCount() const { return items.size(); }
};

#define TODO_STORE TodoStore::getInstance()
