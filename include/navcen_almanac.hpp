#pragma once
#include <string>
#include <vector>

struct AlmanacData {
    uint32_t prn;           // Satellite PRN number
    double ecc;             // Eccentricity
    double sqrta;           // Square root of semi-major axis (√m)
    double delta_i;         // Inclination offset (rad)
    // ... add other parameters as needed
};

class NavcenAlmanacFetcher {
public:
    bool fetch_latest_almanac(const std::string& save_path = "latest.alm");
    std::vector<AlmanacData> parse_yuma_file(const std::string& filepath);
private:
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
};