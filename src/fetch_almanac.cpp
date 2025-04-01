#include "navcen_almanac.hpp"
#include <iostream>

int main() {
    NavcenAlmanacFetcher fetcher;
    
    // Step 1: Download almanac
    if (!fetcher.fetch_latest_almanac()) {
        std::cerr << "Failed to fetch almanac\n";
        return 1;
    }

    // Step 2: Parse almanac
    std::vector<AlmanacData> almanacs = fetcher.parse_yuma_file("latest.alm");
    std::cout << "Loaded " << almanacs.size() << " satellite entries\n";

    // Step 3: Use almanacs for validation
    for (const auto& alm : almanacs) {
        std::cout << "PRN: " << alm.prn 
                  << ", SQRT_A: " << alm.sqrta 
                  << ", ECC: " << alm.ecc << "\n";
    }

    return 0;
}