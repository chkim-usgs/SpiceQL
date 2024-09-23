
#include <iostream>
#include <regex>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "inventory.h"
#include "inventoryimpl.h"
#include "spice_types.h"

using json = nlohmann::json;
using namespace std; 

namespace SpiceQL { 
    namespace Inventory { 
        nlohmann::json search_for_kernelset(std::string instrument, std::vector<string> types, double start_time, double stop_time,  string ckQuality, string spkQuality, bool enforce_quality) { 
            InventoryImpl impl;
            vector<Kernel::Type> enum_types;
            Kernel::Quality enum_ck_quality = Kernel::translateQuality(ckQuality); 
            Kernel::Quality enum_spk_quality = Kernel::translateQuality(spkQuality);  

            for (auto &e:types) { 
                enum_types.push_back(Kernel::translateType(e));
            }

            cout << "entering search" << endl;
            return impl.search_for_kernelset(instrument, enum_types, start_time, stop_time, enum_ck_quality, enum_spk_quality, enforce_quality);
        }

        void create_database() { 
            // force generate the database
            InventoryImpl db(true);
        }
    }
}