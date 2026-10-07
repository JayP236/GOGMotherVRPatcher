#include "utilities.h"
#include <iostream>
#include <fstream>

unsigned int FilePatcher::calculateCrc32(const std::string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "calculateCrc32: Failed to open file " << filename << endl;
        return 0;
    }

    unsigned int crc32Table[256];
    unsigned int polynomial = 0xEDB88320;
    for (unsigned int i = 0; i < 256; ++i) {
        unsigned int crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crc32Table[i] = crc;
    }

    unsigned int crc = 0xFFFFFFFF;
    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
        streamsize bytesRead = file.gcount();
        for (streamsize i = 0; i < bytesRead; ++i) {
            BYTE byte = static_cast<BYTE>(buffer[i]);
            crc = (crc >> 8) ^ crc32Table[(crc ^ byte) & 0xFF];
        }
    }

    return crc ^ 0xFFFFFFFF;
}

bool FilePatcher::loadPatchEmbedded(const string& entry, Patch& patch) {
    HMODULE hModule = GetModuleHandleA(0);
    HRSRC hRes = FindResourceA(hModule, entry.c_str(), MAKEINTRESOURCEA(10));
    if (!hRes) {
        cout << "Failed to find embedded patch resource " << entry << endl;
        return false;
    }

    DWORD totalSize = SizeofResource(hModule, hRes);
    if (totalSize == 0) {
        cout << "Embedded patch resource " << entry << " is empty." << endl;
        return false;
    }

    HGLOBAL hData = LoadResource(hModule, hRes);
    if (!hData) {
        cout << "Failed to load embedded patch resource " << entry << endl;
        return false;
    }

    const uint8_t* pCurrent = reinterpret_cast<const uint8_t*>(LockResource(hData));
    if (!pCurrent) {
        cout << "Failed to lock embedded patch resource " << entry << endl;
        return false;
    }

    const uint8_t* pEnd = pCurrent + totalSize;

    patch.clear();

    while (pCurrent < pEnd) {
        constexpr size_t metadataSize = sizeof(size_t) + sizeof(bool) + sizeof(size_t) + sizeof(size_t);
        if (pCurrent + metadataSize > pEnd) {
            break;
        }

        // offsetA
        size_t offsetA = *reinterpret_cast<const size_t*>(pCurrent);
        pCurrent += sizeof(size_t);

        // isInsert
        bool isInsert = *reinterpret_cast<const bool*>(pCurrent);
        pCurrent += sizeof(bool);

        // copyLength
        size_t copyLength = *reinterpret_cast<const size_t*>(pCurrent);
        pCurrent += sizeof(size_t);

        // dataSize
        size_t dataSize = *reinterpret_cast<const size_t*>(pCurrent);
        pCurrent += sizeof(size_t);

        // validate
        if (pCurrent + dataSize > pEnd) {
            cout << "Embedded patch file corruption detected (Length out of bounds)." << endl;
            return false;
        }

        // data
        vector<uint8_t> dataBytes;
        if (dataSize > 0) {
            dataBytes.assign(pCurrent, pCurrent + dataSize);
            pCurrent += dataSize;
        }

        patch.emplace_back(PatchHunk{ offsetA, isInsert, copyLength, dataBytes });
    }

    return true;
}

bool FilePatcher::applyPatch(const string& inputFilePath, const string& outputFilePath, const Patch& patch) {
    try {
        // read file
        ifstream a(inputFilePath, ios::binary | ios::ate);
        if (!a) return false;

        vector<uint8_t> bufferA(static_cast<size_t>(a.tellg()));
        a.seekg(0);
        a.read(reinterpret_cast<char*>(bufferA.data()), bufferA.size());
        a.close();

        // apply patch
        vector<uint8_t> outData;
        for (const auto& hunk : patch) {
            if (hunk.isInsert) {
                outData.insert(outData.end(), hunk.data.begin(), hunk.data.end());
            }
            else {
                if (hunk.offsetA + hunk.copyLength > bufferA.size()) {
                    cout << "Failed to apply patch: hunk offset validation failed." << endl;
                    return false;
                }
                outData.insert(outData.end(), bufferA.begin() + hunk.offsetA, bufferA.begin() + hunk.offsetA + hunk.copyLength);
            }
        }

        // write output
        ofstream c(outputFilePath, ios::binary);
        if (!c) return false;
        c.write(reinterpret_cast<const char*>(outData.data()), outData.size());

        return true;
    }
    catch (...) {
        cout << "Failed to apply patch." << endl;
        return false;
    }
}