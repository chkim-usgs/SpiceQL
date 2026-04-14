#include "alias_map.h"
#include "utils.h"
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <spdlog/spdlog.h>

using json = nlohmann::json;
using namespace std;

namespace SpiceQL {

    // internal
    static unordered_map<string, string> m_lookupTable;
    static mutex m_mutex;
    static bool m_initialized = false;

    void load_aliases_internal(string path) {
      if (path.empty()) {
        path = getAliasMapJsonFile();
      }

      SPDLOG_INFO("Loading aliases from {}", path);
      ifstream file(path);
      if (!file.is_open()) {
        throw runtime_error("Failed to open alias file: " + path);
      }

      json j;
      file >> j;

      m_lookupTable.clear();
      for (auto& [mission, aliases] : j.items()) {
        for (const string& alias : aliases) {
          m_lookupTable[toUpper(alias)] = mission;
        }
      }
      
      m_initialized = true;
      SPDLOG_INFO("Successfully loaded {} aliases into memory.", m_lookupTable.size());
    }

    void ensure_init() {
      if (!m_initialized) {
        SPDLOG_DEBUG("Loading default aliases.");
        load_aliases_internal(""); 
      }
    }

    // public
    void load_aliases(string path) {
      lock_guard<mutex> lock(m_mutex);
      load_aliases_internal(path); 
    }

    string get_mission(const string& alias) {
      lock_guard<mutex> lock(m_mutex);
      ensure_init();

      string upperAlias = toUpper(alias);
      auto it = m_lookupTable.find(upperAlias);
      if (it != m_lookupTable.end()) {
        return it->second;
      }
      throw invalid_argument("Alias [" + alias + "] not found.");
    }

    json get_aliases() {
      lock_guard<mutex> lock(m_mutex);
      ensure_init();
      return m_lookupTable;
    }
}
