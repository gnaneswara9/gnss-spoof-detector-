#include "ubx_parser.hpp"
#include "navcen_almanac.hpp"
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <unistd.h>

std::unordered_map<uint32_t, UbxNavAlm> almanac_database;
std::unordered_map<uint32_t, UbxNavEph> ephemeris_cache;

constexpr double ALMANAC_THRESHOLD = 0.01;  // 1% tolerance
constexpr double POSITION_THRESHOLD = 1000.0; // 1km tolerance

// Layer 1: Almanac validation
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
    return true;
}

//  Layer 2 - Ephemeris validation
bool validate_ephemeris(const UbxNavEph& eph, const UbxNavAlm& alm) {
    // Simplified position prediction (replace with GPSTk for production)
    double t = eph.tow - alm.toa;
    double mean_anomaly = alm.M0 + t * sqrt(3.986e14/pow(alm.sqrtA,6));
    
    double pred_x = alm.sqrtA * alm.sqrtA * cos(mean_anomaly);
    double pred_y = alm.sqrtA * alm.sqrtA * sin(mean_anomaly);
    
    double dx = eph.pos[0] - pred_x;
    double dy = eph.pos[1] - pred_y;
    double distance = sqrt(dx*dx + dy*dy);
    
    if (distance > POSITION_THRESHOLD) {
        std::cerr << "[EPHEMERIS MISMATCH] SV " << eph.svid 
                 << ": Predicted (" << pred_x << "," << pred_y << ")"
                 << " vs Actual (" << eph.pos[0] << "," << eph.pos[1] << ")\n";
        return false;
    }
    return true;
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

        while (true) {
            // Process almanac (Layer 1)
            UbxNavAlm alm;
            if (ubx_parser.read_ubx_nav_alm(alm)) {
                almanac_database[alm.svid] = alm;
                if (!validate_against_navcen(alm, official_almanacs)) {
                    std::cerr << "[ALERT] Almanac spoofing for SV " << alm.svid << "!\n";
                }
            }

            // Process ephemeris (Layer 2)
            UbxNavEph eph;
            if (ubx_parser.read_ubx_nav_eph(eph)) {
                ephemeris_cache[eph.svid] = eph;
                if (almanac_database.count(eph.svid)) {
                    if (!validate_ephemeris(eph, almanac_database[eph.svid])) {
                        std::cerr << "[ALERT] Possible signal spoofing for SV " 
                                 << eph.svid << "!\n";
                    }
                }
            }

            usleep(100000);  // 100ms delay
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
