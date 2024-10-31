#pragma once
/**
 * @file
 *
 * internal database for the kernel inventory 
 *
 **/

#include <string>
#include <vector>
#include <tuple>
#include <limits>

#include <fc/btree.h>
#include <nlohmann/json.hpp>

#include "spice_types.h"

namespace SpiceQL {

  extern std::string DB_HDF_FILE; 
  extern std::string DB_START_TIME_KEY; 
  extern std::string DB_STOP_TIME_KEY; 
  extern std::string DB_TIME_FILES_KEY; 
  extern std::string DB_SS_TIME_INDICES_KEY; 

  std::string getCacheDir(); 
  std::string getHdfFile();
  
  class TimeIndexedKernels { 
    public: 
    frozenca::BTreeMap<double, size_t> start_times; 
    frozenca::BTreeMap<double, size_t> stop_times; 
    std::vector<std::string> file_paths; 
  };


  class InventoryImpl {
    public:
    InventoryImpl(bool force_regen=false); 
    template<class T> T getKey(std::string key);
    void write_database();
    nlohmann::json search_for_kernelset(std::string spiceql_name, std::vector<Kernel::Type> types, double start_time=-std::numeric_limits<double>::max(), double stop_time=std::numeric_limits<double>::max(),
                                            Kernel::Quality ckQuality=Kernel::Quality::SMITHED, Kernel::Quality spkQuality=Kernel::Quality::SMITHED,  bool enforce_quality=false);
    nlohmann::json search_for_kernelsets(std::vector<std::string> spiceql_names, std::vector<Kernel::Type> types, double start_time=-std::numeric_limits<double>::max(), double stop_time=std::numeric_limits<double>::max(),
                                            Kernel::Quality ckQuality=Kernel::Quality::SMITHED, Kernel::Quality spkQuality=Kernel::Quality::SMITHED,  bool enforce_quality=false, bool overwrite=false);
    nlohmann::json m_json_inventory; 

    std::map<std::string, std::vector<std::string>> m_nontimedep_kerns;  
    std::map<std::string, TimeIndexedKernels*> m_timedep_kerns;     
    
    // Kernels that always need to be furnished
    KernelSet m_required_kernels; 
  };
}
