#pragma once

#include "types.h"
#include "bfres.h"
#include "bfsha.h"

#include <string_view>
#include <unordered_map>
#include <vector>

class ShaderSelector {
public:
    using OptionMap = std::unordered_map<std::string_view, std::string_view>;

    ShaderSelector() = default;

    void LoadOptions(const ResMaterial* material);

    int Search(const g3d2::ResShaderFile* shader_file);

    const OptionMap& GetOptions() const { return mOptions; }

    void SetOption(const std::string_view& key, const std::string_view& value) {
        mOptions[key] = value;
    }

private:
    void WriteKeys(const g3d2::ResShadingModel* model);
    void WriteDefaultKeys(const g3d2::ResShadingModel* model);

    void WriteStaticKey(const g3d2::ResShaderOption* option, u32 value);
    void WriteDynamicKey(const g3d2::ResShaderOption* option, u32 value);

    std::string_view mArchiveName;
    std::string_view mModelName;
    OptionMap mOptions{};
    std::vector<u32> mKeys{};
    u32 mStaticKeyCount = 0;
    bool mIsOverride = false;
};