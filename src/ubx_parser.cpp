#include "ubx_parser.hpp"
#include <iostream>
#include <cstring> // For std::memcpy

UbxParser::UbxParser(const std::string& port) {
    try {
        serial_.Open(port);
        serial_.SetBaudRate(LibSerial::BaudRate::BAUD_9600);
        serial_.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
        serial_.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);
        serial_.SetParity(LibSerial::Parity::PARITY_NONE);
        serial_.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
        serial_.SetVTime(100); // 100ms timeout
        std::cout << "Successfully opened " << port << " | Bytes available: " << serial_.GetNumberOfBytesAvailable() << std::endl;
    } catch (const LibSerial::OpenFailed&) {
        throw std::runtime_error("Failed to open serial port: " + port);
    }
}

UbxParser::~UbxParser() {
    if (serial_.IsOpen()) serial_.Close();
}

template <typename T>
T UbxParser::read_le(const uint8_t* data) {
    T value;
    std::memcpy(&value, data, sizeof(T)); // Correctly handle different data types
    return value;
}

bool UbxParser::verify_checksum(const std::vector<uint8_t>& packet) {
    if (packet.size() < 8) return false;
    uint8_t ck_a = 0, ck_b = 0;
    for (size_t i = 2; i < packet.size() - 2; i++) {
        ck_a += packet[i];
        ck_b += ck_a;
    }
    return (ck_a == packet[packet.size()-2] && ck_b == packet[packet.size()-1]);
}

bool UbxParser::parse_ubx_nav_alm(const std::vector<uint8_t>& packet, UbxNavAlm& alm) {
    if (packet.size() < 48 + 8) return false;
    const uint8_t* payload = &packet[6];
    
    alm.svid = payload[0];
    alm.week = read_le<uint32_t>(&payload[4]);
    alm.toa = read_le<uint32_t>(&payload[8]);
    alm.sqrtA = read_le<double>(&payload[12]);
    alm.e = read_le<double>(&payload[20]);
    alm.i0 = read_le<double>(&payload[28]);
    alm.omega0 = read_le<double>(&payload[36]);
    alm.omega = read_le<double>(&payload[44]);
    alm.M0 = read_le<double>(&payload[52]);
    alm.af0 = read_le<double>(&payload[60]);
    alm.af1 = read_le<double>(&payload[68]);
    alm.health = read_le<int32_t>(&payload[76]);
    alm.ioda = read_le<uint32_t>(&payload[80]);
    
    return true;
}

bool UbxParser::read_ubx_nav_alm(UbxNavAlm& alm) {
    std::vector<uint8_t> packet;
    uint8_t byte;

    // Sync char search
    for (int retry = 0; retry < 3; retry++) {
        serial_.ReadByte(byte); // ReadByte() returns void, so just call it
        if (byte != 0xB5) continue;
        
        serial_.ReadByte(byte);
        if (byte == 0x62) break;
    }

    // Read header (class, id, length)
    for (int i = 0; i < 4; i++) {
        serial_.ReadByte(byte);
        packet.push_back(byte);
    }

    // Check UBX-NAV-ALM (class 0x01, id 0x30)
    if (packet[2] != 0x01 || packet[3] != 0x30) return false;

    uint16_t length;
    serial_.ReadByte(byte);
    length = byte;
    serial_.ReadByte(byte);
    length |= (byte << 8);

    if (length < 40) return false;

    // Read payload
    for (int i = 0; i < length; i++) {
        serial_.ReadByte(byte);
        packet.push_back(byte);
    }

    // Read checksum
    serial_.ReadByte(byte);
    packet.push_back(byte);
    serial_.ReadByte(byte);
    packet.push_back(byte);

    if (!verify_checksum(packet)) return false;
    return parse_ubx_nav_alm(packet, alm);
}
