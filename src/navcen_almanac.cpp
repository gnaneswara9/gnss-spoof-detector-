#include "navcen_almanac.hpp"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>

// Callback to write downloaded data to a file
size_t NavcenAlmanacFetcher::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    size_t total = size * nmemb;
    file->write(static_cast<char*>(contents), total);
    return total;
}

bool NavcenAlmanacFetcher::fetch_latest_almanac(const std::string& save_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL\n";
        return false;
    }

    std::ofstream out_file(save_path, std::ios::binary);
    if (!out_file) {
        std::cerr << "Failed to open output file\n";
        curl_easy_cleanup(curl);
        return false;
    }

    // NAVCEN FTP URL (example - check actual URL)
    const std::string url = "https://www.navcen.uscg.gov/sites/default/files/gps/almanac/current_yuma.alm";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // Enable debug output

    CURLcode res = curl_easy_perform(curl);
    out_file.close();

    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);
    std::cout << "Almanac downloaded to " << save_path << "\n";
    return true;
}

std::vector<AlmanacData> NavcenAlmanacFetcher::parse_yuma_file(const std::string& filepath) {
    std::vector<AlmanacData> almanacs;
    std::ifstream file(filepath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("PRN") != std::string::npos) {
            AlmanacData alm;
            std::istringstream iss(line);
            std::string token;

            // Example YUMA format parsing (adjust as needed)
            while (iss >> token) {
                if (token == "PRN:") iss >> alm.prn;
                else if (token == "ECC:") iss >> alm.ecc;
                else if (token == "SQRT_A:") iss >> alm.sqrta;
                // ... parse other parameters
            }
            almanacs.push_back(alm);
        }
    }
    return almanacs;
}