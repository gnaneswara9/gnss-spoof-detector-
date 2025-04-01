#pragma once
#include <vector>
#include <cstdint>
#include <libserial/SerialPort.h>
#include <stdexcept>

struct UbxNavAlm {
    uint32_t svid;
    uint32_t week;
    uint32_t toa;
    double sqrtA;
    double e;
    double i0;
    double omega0;
    double omega;
    double M0;
    double af0;
    double af1;
    int32_t health;
    uint32_t ioda;
};

struct UbxNavEph {
    uint32_t svid;
    uint32_t week;
    double tow;
    double pos[3];
    double vel[3];
    double acc[3];
    double af0;
    double af1;
    double af2;
    double tgd;
    double crs, crc;
    double cuc, cus;
    double cic, cis;
    double toe;
    uint8_t iode;
    uint8_t health;
    uint8_t fitInterval;
};

class UbxParser {
public:
    UbxParser(const std::string& port);
    ~UbxParser();
    bool read_ubx_nav_alm(UbxNavAlm& alm);
    bool read_ubx_nav_eph(UbxNavEph& eph);
private:
    LibSerial::SerialPort serial_;
    bool verify_checksum(const std::vector<uint8_t>& packet);
    bool parse_ubx_nav_alm(const std::vector<uint8_t>& packet, UbxNavAlm& alm);
    bool parse_ubx_nav_eph(const std::vector<uint8_t>& packet, UbxNavEph& eph);
    template <typename T> T read_le(const uint8_t* data);
};