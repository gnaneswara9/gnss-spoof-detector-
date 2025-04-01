#include "ubx_parser.hpp"
#include "navcen_almanac.hpp"
#include <iostream>
#include <unordered_map>
#include <unistd.h>

std::unordered_map<uint32_t, UbxNavAlm> almanac_database;
std::unordered_map<uint32_t, UbxNavEph> ephemeris_cache;

constexpr double ALMANAC_THRESHOLD = 0.01;  // 1% tolerance for Keplerian elements
constexpr double EPHEMERIS_THRESHOLD = 1000.0; // 1km position mismatch

bool validate_against_navcen(const UbxNavAlm& local_alm, 
                           const std::vector<AlmanacData>& official_almanacs) {
    for (const auto& official : official_almanacs) {
        if (official.prn == local_alm.svid) {
            double delta_sqrtA = std::abs(official.sqrta - local_alm.sqrtA);
            double delta_e = std::abs(official.ecc - local_alm.e);
            if (delta_sqrtA > ALMANAC_THRESHOLD || delta_e > 1e-4) {
                std::cerr << "[WARNING] Almanac mismatch for SV " << local_alm.svid << "\n";
                return false;
            }
            return true;
        }
    }
    return true;  // Missing almanac isn't necessarily spoofing
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <serial_port> <almanac_path>\n";
        return 1;
    }

    try {
        UbxParser ubx_parser(argv[1]);
        NavcenAlmanacFetcher navcen_fetcher;

        if (!navcen_fetcher.fetch_latest_almanac(argv[2])) {
            std::cerr << "Warning: Using cached almanac\n";
        }
        auto official_almanacs = navcen_fetcher.parse_yuma_file(argv[2]);

        // Debug: Print number of official almanacs loaded
        std::cout << "Loaded " << official_almanacs.size() << " official almanacs\n";

        while (true) {
            UbxNavAlm alm;
            if (ubx_parser.read_ubx_nav_alm(alm)) {
                std::cout << "Received almanac for SV " << alm.svid << "\n";
                almanac_database[alm.svid] = alm;
                
                if (!validate_against_navcen(alm, official_almanacs)) {
                    std::cerr << "[ALERT] Possible almanac spoofing for SV " << alm.svid << "!\n";
                }
                
                // Debug: Print current almanac count
                std::cout << "Almanac database size: " << almanac_database.size() << "\n";
            }
            usleep(100000);  // 100ms delay
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}