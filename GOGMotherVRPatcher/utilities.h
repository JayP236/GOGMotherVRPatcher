#pragma once
#include <windows.h>
#include <vector>
#include <string>

using namespace std;

class FilePatcher {
public:
    struct PatchHunk {
        size_t offsetA;
        bool isInsert;
        size_t copyLength;
        vector<uint8_t> data;
    };
    using Patch = vector<PatchHunk>;

    static unsigned int calculateCrc32(const std::string& filename);

    static bool applyPatch(const string& inputFilePath, const string& outputFilePath, const Patch& patch);

    static bool loadPatchEmbedded(const string& entry, Patch& patch);
};