#include "TodoStore.h"

#include <HalStorage.h>
#include <JsonSettingsIO.h>
#include <Logging.h>

TodoStore TodoStore::instance;

namespace {
constexpr char TODO_FILE_JSON[] = "/.crosspoint/todos.json";
}  // namespace

bool TodoStore::saveToFile() const {
  Storage.mkdir("/.crosspoint");
  return JsonSettingsIO::saveTodos(*this, TODO_FILE_JSON);
}

bool TodoStore::loadFromFile() {
  if (!Storage.exists(TODO_FILE_JSON)) {
    return false;
  }

  String json = Storage.readFile(TODO_FILE_JSON);
  if (json.isEmpty()) {
    return false;
  }

  return JsonSettingsIO::loadTodos(*this, json.c_str());
}

bool TodoStore::addItem(const std::string& text) {
  if (items.size() >= MAX_ITEMS) {
    LOG_DBG("TODO", "Cannot add more items, limit of %zu reached", MAX_ITEMS);
    return false;
  }

  items.push_back(TodoItem{text, false});
  LOG_DBG("TODO", "Added item: %s", text.c_str());
  return saveToFile();
}

bool TodoStore::toggleDone(size_t index) {
  if (index >= items.size()) {
    return false;
  }

  items[index].done = !items[index].done;
  return saveToFile();
}

bool TodoStore::removeItem(size_t index) {
  if (index >= items.size()) {
    return false;
  }

  LOG_DBG("TODO", "Removed item: %s", items[index].text.c_str());
  items.erase(items.begin() + static_cast<ptrdiff_t>(index));
  return saveToFile();
}
