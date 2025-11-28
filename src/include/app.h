#pragma once

#include "bfres.h"
#include "bfsha.h"
#include "shader.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <vector>

using json = nlohmann::json;
using Path = std::filesystem::path;

struct AppContext {
public:
    AppContext() {}

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

    std::vector<u8> mExternalBinaryStringStorage{};
    std::vector<u8> mShaderArchiveStorage{};
    bool mInitialized = false;
};

class MaterialParser {
public:
    MaterialParser() = delete;
    explicit MaterialParser(const std::string_view romfs_path,
                            const std::string_view material_archive_path = "",
                            const std::string_view external_binary_string_path = "")
        : mRomfsPath(romfs_path), mMaterialArchivePath(material_archive_path), mExternalBinaryStringPath{external_binary_string_path} {
        if (mMaterialArchivePath == "") {
            mMaterialArchivePath = "material.Product.140.product.Nin_NX_NVN.bfsha";
        }
        if (mExternalBinaryStringPath == "") {
            mExternalBinaryStringPath = (Path(mRomfsPath) / Path("Shader") / Path("ExternalBinaryString.bfres.mc")).string();
        }
    }

    bool Initialize();
    void Run();

private:
    void ProcessFile(const std::string path, json& output, bool compressed = true);

    std::string mRomfsPath{};
    std::string mMaterialArchivePath{};
    std::string mExternalBinaryStringPath{};
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
            ParseValue(key, value);
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
    explicit MaterialSearcher(const std::string config_path, const std::string_view material_archive_path = "", bool verbose = false)
        : mConfigPath(config_path), mMaterialArchivePath(material_archive_path), mVerbose(verbose) {
        if (mMaterialArchivePath == "") {
            mMaterialArchivePath = "material.Product.140.product.Nin_NX_NVN.bfsha";
        }
    }

    bool Initialize();
    void Run();

private:
    void Print(const g3d2::ResShadingModel* model, const u32* keys) const;

    std::string mConfigPath{};
    std::string mMaterialArchivePath{};
    std::vector<Constraint<false>> mStaticConstraints{};
    std::vector<Constraint<true>> mDynamicConstraints{};
    AppContext mContext{};
    bool mInitialized = false;
    bool mVerbose = false;
};

class ShaderInfoPrinter {
public:
    ShaderInfoPrinter() = delete;
    explicit ShaderInfoPrinter(const std::string model, const std::string archive = "", const std::string archive_path = "")
        : mArchivePath(archive_path), mArchiveName(archive), mModelName(model) {
        if (archive == "") {
            mArchiveName = mModelName;
        }
        if (archive_path == "") {
            mArchivePath = "material.Product.140.product.Nin_NX_NVN.bfsha";
        }
    }

    bool Initialize();
    void Run();

private:
    std::string mArchivePath{};
    std::string mArchiveName{};
    std::string mModelName{};
    AppContext mContext{};
    bool mInitialized = false;
};