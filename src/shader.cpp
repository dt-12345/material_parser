#include "shader.h"

#include <array>
#include <cstring>
#include <format>
#include <string>

// the game searches from 0 to 17 even though most shaders definitely do not support that range or even accept this range as valid choices
// the material archive model supports 0-8
const std::array<std::string, 18> ShaderSelector::cNumberNames = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17"
};

const std::array<std::string, 4> ShaderSelector::cRenderStates = {
    "opaque", "mask", "translucent", "custom"
};

const std::array<std::string, 8> ShaderSelector::cCompareFuncs = {
    "never", "less", "equal", "lequal", "greater", "nequal", "gequal", "always"
};

const std::array<std::string, 4> ShaderSelector::cPasses = {
    "no_setting", "seal", "xlu_water", "reduced_buffer"
};

const std::array<std::string, 4> ShaderSelector::cDisplayFaces = {
    "both", "front", "back", "none"
};

const std::string ShaderSelector::cRenderStateName = "gsys_renderstate";
const std::string ShaderSelector::cAlphaTestFuncName = "gsys_alpha_test_func";
const std::string ShaderSelector::cAlphaTestEnableName = "gsys_alpha_test_enable";
const std::string ShaderSelector::cPassName = "gsys_pass";
const std::string ShaderSelector::cDisplayFaceTypeName = "gsys_display_face_type";
const std::string ShaderSelector::cIndexFormatName = "gsys_index_stream_format";
const std::string ShaderSelector::cTrue = "1";
const std::string ShaderSelector::cFalse = "0";
const std::string ShaderSelector::cWeightName = "gsys_weight";

void ShaderSelector::LoadOptions(const ResMaterial* material) {
    mArchiveName = material->shader_data->shader_reflection->archive_name->Get();
    mModelName = material->shader_data->shader_reflection->shading_model_name->Get();

    for (size_t i = 0; i < material->shader_data->total_static_option_count; ++i) {
        const u16 index = material->shader_data->static_option_index_array ? material->shader_data->static_option_index_array[i] : static_cast<u16>(i);
        const std::string_view& key = material->shader_data->shader_reflection->static_option_dict->entries[index + 1].key->Get();

        if (i < material->shader_data->bool_static_option_count) {
            mOptions.emplace(
                key,
                (material->shader_data->static_option_bool_value_array[i >> 5 & 0x7ffffff] >> (i & 0x1f) & 1) ? cTrue : cFalse
            );
        } else {
            mOptions.emplace(
                key,
                material->shader_data->static_option_string_array[i - material->shader_data->bool_static_option_count]->Get()
            );
        }
    }

    for (size_t i = 0; i < material->shader_data->shader_reflection->render_info_count; ++i) {
        const std::string_view render_info_name = material->shader_data->shader_reflection->render_info_array[i].name->Get();
        const u16 offset = material->render_info_value_offset_array[i];
        const auto value = *reinterpret_cast<const BinString**>(reinterpret_cast<uintptr_t>(material->render_info_value_array) + offset);

        // just assume they're all strings bc the only values we care about are strings
        if (render_info_name == "gsys_render_state_mode") {
            mOptions.emplace(cRenderStateName, cNumberNames.at(GetRenderState(value->Get())));
        } else if (render_info_name == "gsys_alpha_test_func") {
            mOptions.emplace(cAlphaTestFuncName, cNumberNames.at(GetCompareFunc(value->Get())));
        } else if (render_info_name == "gsys_alpha_test_enable") {
            mOptions.emplace(cAlphaTestEnableName, value->Get() == "true" ? cTrue : cFalse);
        } else if (render_info_name == "gsys_render_state_display_face") {
            mOptions.emplace(cDisplayFaceTypeName, cNumberNames.at(GetDisplayFace(value->Get())));
        } else if (render_info_name == "gsys_pass") {
            mOptions.emplace(cPassName, cNumberNames.at(GetPass(value->Get())));
        } /*else if (render_info_name == "gsys_override_shader") {
            // don't actually add this to the shader options
            // mIsOverride = *value->Get() == "true";
            // mOptions.emplace(cIndexFormatName, cNumberNames[shape->mesh->index_format > 3 ? shape->mesh->index_format + 1 : 0]);
        } */ else {
            continue;
        }
    }

    mOptions.emplace("gsys_assign_type", "gsys_assign_material");
}

void ShaderSelector::LoadOptions(const json& options) {
    mArchiveName = options.value("Archive Name", "material");
    mModelName = options.value("Model Name", "material");

    const auto& static_options = options.value("Static Options", json({}));
    for (const auto& [key, val] : static_options.items()) {
        if (val.is_boolean()) {
            mOptions.emplace(key, val.get<bool>() ? cTrue : cFalse);
        } else {
            mOptions.emplace(key, val.get<std::string>());
        }
    }

    mOptions.emplace("gsys_assign_type", options.value("Assign Type", "gsys_assign_material"));
}

void ShaderSelector::WriteStaticKey(const g3d2::ResShaderOption* option, u32 value) {
    mKeys[option->option_index] = (value << option->bit_offset) | (mKeys[option->option_index] & ~option->option_mask);
}

void ShaderSelector::WriteDynamicKey(const g3d2::ResShaderOption* option, u32 value) {
    const u32 opt_index = mStaticKeyCount + option->option_index - option->dynamic_index_offset;
    mKeys[opt_index] = (value << option->bit_offset) | (mKeys[opt_index] & ~option->option_mask);
}

u32 ShaderSelector::GetStaticKeyValue(const g3d2::ResShaderOption* option) const {
    return (mKeys[option->option_index] & option->option_mask) >> option->bit_offset;
}

u32 ShaderSelector::GetDynamicKeyValue(const g3d2::ResShaderOption* option) const {
    return (mKeys[mStaticKeyCount + option->option_index - option->dynamic_index_offset] & option->option_mask) >> option->bit_offset;
}

void ShaderSelector::WriteKeys(const g3d2::ResShadingModel* model) {
    WriteDefaultKeys(model);

    for (size_t i = 0; i < model->static_option_count; ++i) {
        const auto& option = model->static_option_array[i];
        if (!mOptions.contains(std::string(option.name->Get())))
            continue;

        const int index = option.choice_dict->FindIndex(mOptions.at(std::string(option.name->Get())));
        if (index == -1)
            continue;

        WriteStaticKey(&option, index);
    }

    for (size_t i = 0; i < model->dynamic_option_count; ++i) {
        const auto& option = model->dynamic_option_array[i];
        if (!mOptions.contains(std::string(option.name->Get())))
            continue;
        
        const int index = option.choice_dict->FindIndex(mOptions.at(std::string(option.name->Get())));
        if (index == -1)
            continue;

        WriteDynamicKey(&option, index);
    }
}

void ShaderSelector::WriteDefaultKeys(const g3d2::ResShadingModel* model) {
    mKeys.clear();
    mKeys.resize(model->static_key_count + model->dynamic_key_count);
    mStaticKeyCount = model->static_key_count;
    std::memset(mKeys.data(), 0, mKeys.size() * sizeof(u32));

    for (size_t i = 0; i < model->static_option_count; ++i) {
        const auto& option = model->static_option_array[i];

        WriteStaticKey(&option, option.default_choice);
    }

    for (size_t i = 0; i < model->dynamic_option_count; ++i) {
        const auto& option = model->dynamic_option_array[i];

        WriteDynamicKey(&option, option.default_choice);
    }
}

const g3d2::ResShaderProgram* ShaderSelector::Search(const g3d2::ResShaderFile* shader_file) {
    if (shader_file->archive->name->Get() != mArchiveName) {
        return nullptr;
    }
    
    const g3d2::ResShadingModel* model = nullptr;
    const std::string_view model_name = /* mIsOverride ? std::format("{}_override", mModelName) : */ mModelName;
    for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
        if (shader_file->archive->shading_model_array[i].name->Get() == model_name) {
            model = shader_file->archive->shading_model_array + i;
            break;
        }
    }

    if (model == nullptr) {
        return nullptr;
    }

    WriteKeys(model);

    for (size_t i = 0; i < model->shader_program_count; ++i) {
        const int opt_index = (model->static_key_count + model->dynamic_key_count) * static_cast<int>(i);
        if (std::memcmp(model->key_table + opt_index, mKeys.data(), mKeys.size() * sizeof(u32)) == 0)
            return model->program_array + i;
    }

    return nullptr;
}