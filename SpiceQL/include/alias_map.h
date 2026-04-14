#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace SpiceQL {

    /**
     * @brief Load alias map given path or defaults to aliasMap.json in env
     * 
     * @param path string path to json file
     */
    void load_aliases(std::string path = ""); 

    /**
     * @brief Get SpiceQL mission given alias
     * 
     * @param alias string alias name
     * @return string mission name
     */
    std::string get_mission(const std::string& alias);

    /**
     * @brief Get aliases as map
     * 
     * @return JSON map of aliases
     */
    nlohmann::json get_aliases();
}
