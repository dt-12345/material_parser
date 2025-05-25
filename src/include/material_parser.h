#pragma once

#include "bfres.h"
#include "bfsha.h"

#include <nlohmann/json.hpp>

#include <span>
#include <string>
#include <vector>

using json = nlohmann::json;

class MaterialParser {
public:
    MaterialParser() = delete;
    explicit MaterialParser(const std::string_view& romfs_path) : mRomfsPath(romfs_path) {}

    bool Initialize();
    bool Run();

private:
    const ResFile* GetExternalBinaryString() {
        return ResFile::ResCast(mExternalBinaryStringStorage.data());
    }

    const g3d2::ResShaderFile* GetShaderArchive() {
        return g3d2::ResShaderFile::ResCast(mShaderArchiveStorage.data());
    }

    static bool ReadFile(const std::string path, std::vector<u8>& data);

    static void WriteFile(const std::string path, const std::span<const u8>& data);

    static bool DecompressFile(const std::string path, std::vector<u8>& data);

    ResFile* SetupFile(void* file_data) {
        ResFile* file = ResFile::ResCast(file_data);

        if (!file->IsRelocatedExternalStrings())
            file->RelocateExternalStrings(GetExternalBinaryString());
        
        if (!file->IsRelocatedExternalStrings())
            return nullptr;

        return file;
    }

    void ProcessFile(const std::string path);

    std::string mRomfsPath{};
    std::vector<u8> mExternalBinaryStringStorage{};
    std::vector<u8> mShaderArchiveStorage{};
    json mOutput{};
    bool mInitialized = false;
};