#pragma once

#include "bfres.h"
#include "bfsha.h"
#include "shader.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
using Path = std::filesystem::path;

struct AppContext {
public:
    AppContext() {}

    const ResFile* GetExternalBinaryString() {
        if (mShaderArchiveStorage.empty()) {
            throw std::runtime_error("Tried to access external binary strings before initialization!");
        }
        return ResFile::ResCast(mExternalBinaryStringStorage.data());
    }

    const g3d2::ResShaderFile* GetShaderArchive() {
        if (mShaderArchiveStorage.empty()) {
            throw std::runtime_error("Tried to access shader archive before initialization!");
        }
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

    bool InitializeExternalBinaryString(const std::string& path) {
        if (!DecompressFile(path, mExternalBinaryStringStorage)) {
            std::cout << "Failed to open " << path << "\n";
            return false;
        }
        
        return ResFile::ResCast(mExternalBinaryStringStorage.data()) != nullptr;
    }

    bool InitializeShaderArchive(const std::string& path) {
        if (!ReadFile(path, mShaderArchiveStorage)) {
            std::cout << "Failed to open " << path << "\n";
            return false;
        }

        return g3d2::ResShaderFile::ResCast(mShaderArchiveStorage.data()) != nullptr;
    }

    std::vector<u8> mExternalBinaryStringStorage{};
    std::vector<u8> mShaderArchiveStorage{};
    bool mInitialized = false;
};

class MaterialParser {
public:
    MaterialParser() = delete;
    explicit MaterialParser(const std::string_view romfs_path,
                            const std::string_view material_archive_path = "",
                            const std::string_view external_binary_string_path = "",
                            const std::string_view output_path = "")
        : mRomfsPath(romfs_path), mMaterialArchivePath(material_archive_path), mExternalBinaryStringPath(external_binary_string_path), mOutputPath(output_path) {
        if (mMaterialArchivePath == "") {
            mMaterialArchivePath = "material.Product.140.product.Nin_NX_NVN.bfsha";
        }
        if (mExternalBinaryStringPath == "") {
            mExternalBinaryStringPath = (Path(mRomfsPath) / Path("Shader") / Path("ExternalBinaryString.bfres.mc")).string();
        }
        if (mOutputPath == "") {
            mOutputPath = "Materials.json";
        }
    }

    bool Initialize();
    void Run();

private:
    void ProcessFile(const std::string path, json& output, bool compressed = true);

    std::string mRomfsPath{};
    std::string mMaterialArchivePath{};
    std::string mExternalBinaryStringPath{};
    std::string mOutputPath{};
    AppContext mContext{};
    bool mInitialized = false;
};

template <bool IsDynamic>
struct Constraint {
    Constraint(const std::string_view key, const json& value, const g3d2::ResShadingModel* model) {
        std::string_view opt_name = key;
        if (key == "gsys_render_state_mode") {
            opt_name = ShaderSelector::cRenderStateName;
        } else if (key == "gsys_alpha_test_func") {
            opt_name = ShaderSelector::cAlphaTestFuncName;
        } else if (key == "gsys_alpha_test_enable") {
            opt_name = ShaderSelector::cAlphaTestEnableName;
        } else if (key == "gsys_render_state_display_face") {
            opt_name = ShaderSelector::cDisplayFaceTypeName;
        } else if (key == "gsys_pass") {
            opt_name = ShaderSelector::cPassName;
        }
        int index;
        if constexpr (IsDynamic) {
            index = model->dynamic_option_dict->FindIndex(opt_name);
        } else {
            index = model->static_option_dict->FindIndex(opt_name);
        }
        if (index == -1) {
            throw std::runtime_error(std::format("Unknown option: {}", key));
        }
        if constexpr (IsDynamic) {
            option = model->dynamic_option_array + index;
        } else {
            option = model->static_option_array + index;
        }
        if (value.is_array()) {
            for (const auto& v : value) {
                ParseValue(key, v);
            }
        } else {
            if (value.is_null()) {
                for (const auto i : std::ranges::views::iota(0u, static_cast<u32>(option->choice_count))) {
                    values.push_back(i);
                }
            } else {
                ParseValue(key, value);
            }
        }
    }

    bool Match(const g3d2::ResShadingModel* model, const u32* key_table) const {
        u32 value;
        if constexpr (IsDynamic) {
            value = (key_table[model->static_key_count + option->option_index - option->dynamic_index_offset] & option->option_mask) >> option->bit_offset;
        } else {
            value = (key_table[option->option_index] & option->option_mask) >> option->bit_offset;
        }
        for (const auto val : values) {
            if (value == val)
                return true;
        }
        return false;
    }

    std::vector<u32> values{};
    g3d2::ResShaderOption* option;

private:
    void ParseValue(const std::string_view key, const json& value) {
        if (key == "gsys_render_state_mode") {
            values.push_back(GetOptionValue(ShaderSelector::cNumberNames.at(ShaderSelector::GetRenderState(value.get<std::string_view>())), option));
        } else if (key == "gsys_alpha_test_func") {
            values.push_back(GetOptionValue(ShaderSelector::cNumberNames.at(ShaderSelector::GetCompareFunc(value.get<std::string_view>())), option));
        } else if (key == "gsys_alpha_test_enable") {
            values.push_back(GetOptionValue(value.get<std::string_view>() == "true" ? ShaderSelector::cTrue : ShaderSelector::cFalse, option));
        } else if (key == "gsys_render_state_display_face") {
            values.push_back(GetOptionValue(ShaderSelector::cNumberNames.at(ShaderSelector::GetDisplayFace(value.get<std::string_view>())), option));
        } else if (key == "gsys_pass") {
            values.push_back(GetOptionValue(ShaderSelector::cNumberNames.at(ShaderSelector::GetPass(value.get<std::string_view>())), option));
        } else {
            if (value.is_string()) {
                values.push_back(GetOptionValue(value.get<std::string_view>(), option));
            } else if (value.is_boolean()) {
                values.push_back(GetOptionValue(value.get<bool>() ? ShaderSelector::cTrue : ShaderSelector::cFalse, option));
            } else if (value.is_number_integer()) {
                values.push_back(GetOptionValue(std::format("{}", value.get<int>()), option));
            } else if (value.is_number_float()) {
                values.push_back(GetOptionValue(std::format("{}", value.get<float>()), option));
            } else {
                throw std::runtime_error(std::format("Invalid option value for {}", key));
            }
        }
    }

    static u32 GetOptionValue(const std::string_view value, const g3d2::ResShaderOption* option) {
        const int index = option->choice_dict->FindIndex(value);
        if (index == -1) {
            throw std::runtime_error(std::format("Unknown value for option {}: {}", option->name->Get(), value));
        }
        return static_cast<u32>(index);
    }
};

class MaterialSearcher {
public:
    MaterialSearcher() = delete;
    explicit MaterialSearcher(const std::string config_path,
                              const std::string_view material_archive_path = "",
                              const std::string_view output_path = "",
                              bool verbose = false)
            : mConfigPath(config_path), mMaterialArchivePath(material_archive_path), mOutputFileStream(std::string(output_path)), mVerbose(verbose) {
        if (mMaterialArchivePath == "") {
            mMaterialArchivePath = "material.Product.140.product.Nin_NX_NVN.bfsha";
        }
        if (output_path == "") {
            mOutStream = &std::cout;
        } else {
            mOutStream = &mOutputFileStream;
        }
    }

    bool Initialize();
    void Run();

private:
    void Print(const g3d2::ResShadingModel* model, const u32* keys, size_t index) const;

    std::string mConfigPath{};
    std::string mMaterialArchivePath{};
    std::vector<Constraint<false>> mStaticConstraints{};
    std::vector<Constraint<true>> mDynamicConstraints{};
    AppContext mContext{};
    std::ostream* mOutStream = nullptr;
    std::ofstream mOutputFileStream;
    bool mInitialized = false;
    bool mVerbose = false;
};

class ShaderInfoPrinter {
public:
    ShaderInfoPrinter() = delete;
    explicit ShaderInfoPrinter(const std::string_view archive_path,
                               const std::string_view output_path = "",
                               const std::string_view model_name = "",
                               int program_index = -1,
                               bool dump_options = true)
        : mArchivePath(archive_path), mOutputPath(output_path), mModelName(model_name), mProgramIndex(program_index), mDumpOptions(dump_options) {
        if (mOutputPath == "") {
            mOutputPath = "ShaderInfo.json";
        }
        if (mProgramIndex >= 0 && mModelName == "") {
            mModelName = "material";
        }
    }

    bool Initialize();
    void Run();

private:
    void ProcessModel(ordered_json& output, const g3d2::ResShadingModel* model) const;
    void ProcessInterfaces(ordered_json& output, const g3d2::ResShadingModel* model, const ResDic* names, BinString* const* interfaces, u16 count) const;

    std::string mArchivePath{};
    std::string mOutputPath{};
    std::string mModelName{};
    AppContext mContext{};
    int mProgramIndex = -1;
    bool mInitialized = false;
    bool mDumpOptions = true;
};