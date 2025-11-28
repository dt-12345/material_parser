#pragma once

#include "types.h"
#include "bfres.h"
#include "bfsha.h"

#include <nlohmann/json.hpp>

#include <string_view>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

class ShaderSelector {
public:
    using OptionMap = std::unordered_map<std::string, std::string>;

    ShaderSelector() = default;

    void LoadOptions(const ResMaterial* material);
    void LoadOptions(const json& options);

    const g3d2::ResShaderProgram* Search(const g3d2::ResShaderFile* shader_file);

    const OptionMap& GetOptions() const { return mOptions; }

    void SetOption(const std::string& key, const std::string_view& value) {
        mOptions[key] = value;
    }

    static const std::array<std::string, 18> cNumberNames;
    static const std::array<std::string, 4> cRenderStates;
    static const std::array<std::string, 8> cCompareFuncs;
    static const std::array<std::string, 4> cPasses;
    static const std::array<std::string, 4> cDisplayFaces;

    static const std::string cRenderStateName;
    static const std::string cAlphaTestFuncName;
    static const std::string cAlphaTestEnableName;
    static const std::string cPassName;
    static const std::string cDisplayFaceTypeName;
    static const std::string cIndexFormatName;
    static const std::string cTrue;
    static const std::string cFalse;
    static const std::string cWeightName;

    static inline s32 GetRenderState(const std::string_view& value) {
        s32 i = 0;
        for (const auto& v : cRenderStates) {
            if (v == value)
                return i;
            ++i;
        }
        return 0; // opaque default
    }

    static inline s32 GetCompareFunc(const std::string_view& value) {
        s32 i = 0;
        for (const auto& v : cCompareFuncs) {
            if (v == value)
                return i;
            ++i;
        }
        return i; // 8 if none are matched
    }

    static inline s32 GetPass(const std::string_view& value) {
        s32 i = 0;
        for (const auto& v : cPasses) {
            if (v == value)
                return i;
            ++i;
        }
        return 0; // no_setting default
    }

    static inline s32 GetDisplayFace(const std::string_view& value) {
        s32 i = 0;
        for (const auto& v : cDisplayFaces) {
            if (v == value)
                return i;
            ++i;
        }
        return 1; // front default
    }

    u32 GetStaticKeyValue(const g3d2::ResShaderOption* option) const;
    u32 GetDynamicKeyValue(const g3d2::ResShaderOption* option) const;

private:
    void WriteKeys(const g3d2::ResShadingModel* model);
    void WriteDefaultKeys(const g3d2::ResShadingModel* model);

    void WriteStaticKey(const g3d2::ResShaderOption* option, u32 value);
    void WriteDynamicKey(const g3d2::ResShaderOption* option, u32 value);

    std::string mArchiveName;
    std::string mModelName;
    OptionMap mOptions{};
    std::vector<u32> mKeys{};
    u32 mStaticKeyCount = 0;
    bool mIsOverride = false;
};