#include "app.h"

#include "mc_MeshCodec.h"

using DirectoryIter = std::filesystem::recursive_directory_iterator;

static std::vector<unsigned char> sWorkMemory(0x10000000);

bool AppContext::ReadFile(const std::string path, std::vector<u8>& data) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        return false;
    
    data.resize(file.tellg());
    file.seekg(0);

    file.read(reinterpret_cast<char*>(data.data()), data.size());

    file.close();

    return true;
}

void AppContext::WriteFile(const std::string path, const std::span<const u8>& data) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
}

bool AppContext::DecompressFile(const std::string path, std::vector<u8>& data) {
    std::vector<u8> fileData{};
    if (!ReadFile(path, fileData)) {
        std::cout << "Failed to read file: " << path << "\n";
        return false;
    }

    auto header = reinterpret_cast<const mc::ResMeshCodecPackageHeader*>(fileData.data());
    const size_t decompressedSize = header->GetDecompressedSize();
    data.resize(decompressedSize);

    if (!mc::DecompressMC(data.data(), data.size(), fileData.data(), fileData.size(), sWorkMemory.data(), sWorkMemory.size())) {
        std::cout << "Failed to decompress file: " << path << "\n";
        return false;
    }

    return true;
}

bool MaterialParser::Initialize() {
    if (mInitialized)
        return mInitialized;

    if (!mContext.InitializeExternalBinaryString(mExternalBinaryStringPath)) {
        std::cout << "Failed to load ExternalBinaryStrings\n";
        return false;
    }

    if (!mContext.InitializeShaderArchive(mMaterialArchivePath)) {
        std::cout << "Failed to load shader archive\n";
        return false;
    }

    mInitialized = true;
    return true;
}

void MaterialParser::ProcessFile(const std::string path, json& output, bool compressed) {
    std::vector<u8> fileBuffer{};
    
    if (compressed) {
        if (!mContext.DecompressFile(path, fileBuffer))
            throw std::runtime_error(std::format("Failed to read file: {}", path));
    } else {
        if (!mContext.ReadFile(path, fileBuffer))
            throw std::runtime_error(std::format("Failed to read file: {}", path));
    }
    
    ResFile* file = mContext.SetupFile(fileBuffer.data());

    if (file == nullptr)
        throw std::runtime_error(std::format("Failed to setup file: {}", path));

    const g3d2::ResShaderFile* shader_file = mContext.GetShaderArchive();

    const std::string filename = Path(path).filename().string();

    std::cout << filename << "\n";

    output[filename] = json({});
    
    for (size_t i = 0; i < file->model_count; ++i) {
        const auto& model = file->models[i];
        const std::string_view model_name = model.name->Get();
        output[filename][model_name] = json({});

        for (size_t j = 0; j < model.material_count; ++j) {
            const auto& mat = model.material_array[j];
            const std::string_view mat_name = mat.name->Get();
            json mat_info = {
                {"Static Options", json({})},
                {"Samplers", json::array()},
                {"Textures", json::array()},
                {"Render Info", json({})},
                {"Skin Counts", json::array()},
            };

            ShaderSelector selector{};
            selector.LoadOptions(&mat);

            for (size_t k = 0; k < mat.sampler_count; ++k)
                mat_info["Samplers"].push_back(mat.sampler_dict->entries[k + 1].key->Get());

            for (size_t k = 0; k < mat.texture_count; ++k)
                mat_info["Textures"].push_back(mat.texture_name_array[k]->Get());

            for (u8 k = 0; k < 0x10; ++k) {
                selector.SetOption(ShaderSelector::cWeightName, ShaderSelector::cNumberNames[k]);

                const auto program = selector.Search(shader_file);

                if (program != nullptr) {
                    mat_info["Skin Counts"].push_back(k);
                }
            }

            for (size_t k = 0; k < mat.shader_data->total_static_option_count; ++k) {
                const u16 index = mat.shader_data->static_option_index_array ? mat.shader_data->static_option_index_array[k] : static_cast<u16>(k);
                const std::string_view& key = mat.shader_data->shader_reflection->static_option_dict->entries[index + 1].key->Get();

                if (k < mat.shader_data->bool_static_option_count) {
                    mat_info["Static Options"][key] = (mat.shader_data->static_option_bool_value_array[k >> 5 & 0x7ffffff] >> (k & 0x1f) & 1) != 0;
                } else {
                    mat_info["Static Options"][key] = mat.shader_data->static_option_string_array[k - mat.shader_data->bool_static_option_count]->Get();
                }
            }

            for (size_t k = 0; k < mat.shader_data->shader_reflection->render_info_count; ++k) {
                const std::string_view render_info_name = mat.shader_data->shader_reflection->render_info_array[k].name->Get();
                const u16 offset = mat.render_info_value_offset_array[k];

                switch (mat.shader_data->shader_reflection->render_info_array[k].type) {
                    case 0:
                        mat_info["Render Info"][render_info_name] = *reinterpret_cast<s32*>(reinterpret_cast<uintptr_t>(mat.render_info_value_array) + offset);
                        break;
                    case 1:
                        mat_info["Render Info"][render_info_name] = *reinterpret_cast<f32*>(reinterpret_cast<uintptr_t>(mat.render_info_value_array) + offset);
                        break;
                    case 2:
                        mat_info["Render Info"][render_info_name] = (*reinterpret_cast<const BinString**>(reinterpret_cast<uintptr_t>(mat.render_info_value_array) + offset))->Get();
                        break;
                }
            }

            output[filename][model_name][mat_name] = std::move(mat_info);
        }
    }
}

void MaterialParser::Run() {
    if (!Initialize())
        return;

    const Path model_path = mRomfsPath / Path("Model");
    json output{};

    for (const auto& entry : DirectoryIter(model_path)) {
        if (entry.path().extension() == ".mc") {
            ProcessFile(entry.path().string(), output);
        } else if (entry.path().extension() == ".bfres") {
            ProcessFile(entry.path().string(), output, false);
        }
    }

    std::ofstream out(mOutputPath);
    out << std::setw(2) << output << std::endl;
}

bool MaterialSearcher::Initialize() {
    if (mInitialized)
        return mInitialized;

   if (!mContext.InitializeShaderArchive(mMaterialArchivePath)) {
        std::cout << "Failed to load shader archive\n";
        return false;
    }

    mInitialized = true;
    return true;
}

static u32 GetStaticKeyValue(const u32* keys, const g3d2::ResShaderOption* option) {
    return (keys[option->option_index] & option->option_mask) >> option->bit_offset;
}

static u32 GetDynamicKeyValue(const u32* keys, const g3d2::ResShaderOption* option, u32 static_key_count) {
    return (keys[static_key_count + option->option_index - option->dynamic_index_offset] & option->option_mask) >> option->bit_offset;
}

static bool IsStaticOptionRenderInfo(const std::string_view name) {
    return name == "gsys_render_state_mode"
           || name == "gsys_alpha_test_func"
           || name == "gsys_alpha_test_enable"
           || name == "gsys_render_state_display_face"
           || name == "gsys_pass";
}

static const std::string_view TryConvertRenderInfo(const std::string_view key, const std::string_view value) {
    if (key == ShaderSelector::cRenderStateName) {
        return ShaderSelector::cRenderStates.at(std::stoi(std::string(value)));
    } else if (key == ShaderSelector::cAlphaTestFuncName) {
        return ShaderSelector::cCompareFuncs.at(std::stoi(std::string(value)));
    } else if (key == ShaderSelector::cAlphaTestEnableName) {
        if (value == ShaderSelector::cTrue) {
            return "true";
        } else {
            return "false";
        }
    } else if (key == ShaderSelector::cDisplayFaceTypeName) {
        return ShaderSelector::cDisplayFaces.at(std::stoi(std::string(value)));
    } else if (key == ShaderSelector::cPassName) {
        return ShaderSelector::cPasses.at(std::stoi(std::string(value)));
    }
    return value;
}

void MaterialSearcher::Print(const g3d2::ResShadingModel* model, const u32* keys, size_t index) const {
    *mOutStream << std::format("Shader Program {}:\n", index);
    if (mVerbose) {
        if (model->static_option_count > 0) {
            *mOutStream << "  Static:\n";
            for (size_t i = 0; i < model->static_option_count; ++i) {
                const auto& opt = model->static_option_array[i];
                const u32 value = GetStaticKeyValue(keys, &opt);
                if (value == opt.default_choice) {
                    continue;
                }
                const std::string_view key = opt.name->Get();
                const std::string_view val = opt.choice_dict->entries[value + 1].key->Get();
                *mOutStream << std::format("    {}: {}\n", key, TryConvertRenderInfo(key, val));
            }
        }
        if (model->dynamic_option_count > 0) {
            *mOutStream << "  Dynamic:\n";
            for (size_t i = 0; i < model->dynamic_option_count; ++i) {
                const auto& opt = model->dynamic_option_array[i];
                const u32 value = GetDynamicKeyValue(keys, &opt, model->static_key_count);
                if (value == opt.default_choice) {
                    continue;
                }
                *mOutStream << std::format("    {}: {}\n", opt.name->Get(), opt.choice_dict->entries[value + 1].key->Get());
            }
        }
    } else {
        if (!mStaticConstraints.empty()) {
            *mOutStream << "  Static:\n";
            for (const auto& constraint : mStaticConstraints) {
                const u32 value = GetStaticKeyValue(keys, constraint.option);
                const std::string_view key = constraint.option->name->Get();
                const std::string_view val = constraint.option->choice_dict->entries[value + 1].key->Get();
                *mOutStream << std::format("    {}: {}\n", key, TryConvertRenderInfo(key, val));
            }
        }
        if (!mDynamicConstraints.empty()) {
            *mOutStream << "  Dynamic:\n";
            for (const auto& constraint : mDynamicConstraints) {
                const u32 value = GetDynamicKeyValue(keys, constraint.option, model->static_key_count);
                *mOutStream << std::format("    {}: {}\n", constraint.option->name->Get(), constraint.option->choice_dict->entries[value + 1].key->Get());
            }
        }
    }
}

void MaterialSearcher::Run() {
    if (!Initialize())
        return;

    std::ifstream f(mConfigPath);
    json data = json::parse(f);
    const std::string model_name = data.value("Model Name", "material");
    
    const g3d2::ResShaderFile* shader_file = mContext.GetShaderArchive();

    g3d2::ResShadingModel* model = nullptr;
    for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
        if (shader_file->archive->shading_model_array[i].name->Get() == model_name) {
            model = shader_file->archive->shading_model_array + i;
            break;
        }
    }

    if (model == nullptr) {
        std::cout << std::format("No model named {}\n", model_name);
        std::cout << "Available models:\n";
        for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
            std::cout << "  " << shader_file->archive->shading_model_array[i].name->Get() << "\n";
        }
        return;
    }

    const auto& static_opts = data.value("Static Options", json({}));
    for (const auto& [key, val] : static_opts.items()) {
        mStaticConstraints.emplace_back(key, val, model);
    }
    const auto& render_info = data.value("Render Info", json({}));
    for (const auto& [key, val] : render_info.items()) {
        if (IsStaticOptionRenderInfo(key)) {
            mStaticConstraints.emplace_back(key, val, model);
        }
    }
    const auto& dynamic_opts = data.value("Dynamic Options", json({}));
    for (const auto& [key, val] : dynamic_opts.items()) {
        mDynamicConstraints.emplace_back(key, val, model);
    }

    for (size_t i = 0; i < model->shader_program_count; ++i) {
        const int opt_index = (model->static_key_count + model->dynamic_key_count) * static_cast<int>(i);
        const u32* keys = model->key_table + opt_index;
        bool matched = true;
        for (const auto& constraint : mStaticConstraints) {
            if (!matched) {
                break;
            }
            matched = matched && constraint.Match(model, keys);
        }
        for (const auto& constraint : mDynamicConstraints) {
            if (!matched) {
                break;
            }
            matched = matched && constraint.Match(model, keys);
        }
        if (matched) {
            Print(model, keys, i);
        }
    }
}

bool ShaderInfoPrinter::Initialize() {
    if (mInitialized)
        return mInitialized;

    if (!mContext.InitializeShaderArchive(mArchivePath)) {
        std::cout << "Failed to load shader archive\n";
        return false;
    }

    mInitialized = true;
    return true;
}

void ShaderInfoPrinter::ProcessInterfaces(ordered_json& output, const g3d2::ResShadingModel* model, const ResDic* names, BinString* const* interfaces, u16 count) const {
    int base_index = 0;
    for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
        const std::string_view name = names->entries[i + 1].key->Get();
        output[name] = ordered_json({});
        if (model->vertex_stage_base_location_index != -1) {
            const BinString* ifc = interfaces[model->vertex_stage_base_location_index + base_index];
            if (ifc != nullptr) {
                output[name]["Vertex"] = ifc->Get();
            }
        }
        if (model->geometry_stage_base_location_index != -1) {
            const BinString* ifc = interfaces[model->geometry_stage_base_location_index + base_index];
            if (ifc != nullptr) {
                output[name]["Geometry"] = ifc->Get();
            }
        }
        if (model->fragment_stage_base_location_index != -1) {
            const BinString* ifc = interfaces[model->fragment_stage_base_location_index + base_index];
            if (ifc != nullptr) {
                output[name]["Fragment"] = ifc->Get();
            }
        }
        if (model->compute_stage_base_location_index != -1) {
            const BinString* ifc = interfaces[model->compute_stage_base_location_index + base_index];
            if (ifc != nullptr) {
                output[name]["Compute"] = ifc->Get();
            }
        }
        if (model->hull_stage_base_location_index != -1) {
            const BinString* ifc = interfaces[model->hull_stage_base_location_index + base_index];
            if (ifc != nullptr) {
                output[name]["Hull"] = ifc->Get();
            }
        }
        if (model->domain_stage_base_location_index != -1) {
            const BinString* ifc = interfaces[model->domain_stage_base_location_index + base_index];
            if (ifc != nullptr) {
                output[name]["Domain"] = ifc->Get();
            }
        }
        base_index += model->shader_stage_count;
    }
}

void ShaderInfoPrinter::ProcessModel(ordered_json& output, const g3d2::ResShadingModel* model) const {
    output["Program Count"] = model->shader_program_count;
    output["Interfaces"] = {
        { "Samplers", ordered_json({}) },
        { "Images", ordered_json({}) },
        { "UBOs", ordered_json({}) },
        { "SSBOs", ordered_json({}) },
    };

    ProcessInterfaces(output["Interfaces"]["Samplers"], model, model->sampler_dict, model->interfaces->sampler_locations, model->sampler_count);
    // no idea if this ResDic* here is actually the image dictionary, but it would make sense if it was
    ProcessInterfaces(output["Interfaces"]["Images"], model, model->_50, model->interfaces->image_locations, model->image_count);
    ProcessInterfaces(output["Interfaces"]["UBOs"], model, model->uniform_block_dict, model->interfaces->ubo_locations, model->uniform_block_count);
    ProcessInterfaces(output["Interfaces"]["SSBOs"], model, model->shader_storage_block_dict, model->interfaces->ssbo_locations, model->shader_storage_block_count);

    output["Vertex Attributes"] = ordered_json({});
    for (size_t i = 0; i < model->vertex_attribute_count; ++i) {
        const std::string_view name = model->vertex_attribute_dict->entries[i + 1].key->Get();
        const auto& loc = model->vertex_attribute_array[i];
        output["Vertex Attributes"][name] = json({
            { "Location", loc.location },
            { "Index", loc.index },
        });
    }

    output["Samplers"] = ordered_json({});
    for (size_t i = 0; i < model->sampler_count; ++i) {
        const std::string_view name = model->sampler_dict->entries[i + 1].key->Get();
        const auto& sampler = model->sampler_array[i];
        output["Samplers"][name] = json({
            { "Annotation", sampler.annotation->Get() },
            { "Index", sampler.index },
        });
    }

    output["UBOs"] = ordered_json({});
    for (size_t i = 0; i < model->uniform_block_count; ++i) {
        const std::string_view name = model->uniform_block_dict->entries[i + 1].key->Get();
        const auto& ubo = model->uniform_block_array[i];
        ordered_json ubo_info = {
            { "Type", static_cast<u8>(ubo.type) },
            { "Index", ubo.index },
            { "Size", ubo.size },
            { "Uniforms", json::array() },
        };
        for (u32 j = 0; j < ubo.member_count; ++j) {
            const auto& uniform = ubo.members[j];
            const std::string_view name = ubo.member_dict->entries[j + 1].key->Get();
            if (uniform.block_index != i) {
                std::cout << "Warning: mismatching block index\n";
            }
            ubo_info["Uniforms"].push_back(ordered_json({
                { "Name", name },
                { "Annotation", uniform.annotation->Get() },
                { "Index", uniform.index },
                { "Offset", uniform.offset },
            }));
        }
        output["UBOs"][name] = std::move(ubo_info);
    }

    output["SSBOs"] = ordered_json({});
    for (size_t i = 0; i < model->shader_storage_block_count; ++i) {
        const std::string_view name = model->shader_storage_block_dict->entries[i + 1].key->Get();
        const auto& ssbo = model->shader_storage_block_array[i];
        ordered_json ssbo_info = {
            { "Type", static_cast<u8>(ssbo.type) },
            { "Index", ssbo.index },
            { "Size", ssbo.size },
            { "Members", json::array() },
        };
        for (u32 j = 0; j < ssbo.member_count; ++j) {
            const auto& uniform = ssbo.members[j];
            const std::string_view name = ssbo.member_dict->entries[j + 1].key->Get();
            if (uniform.block_index != i) {
                std::cout << "Warning: mismatching block index\n";
            }
            ssbo_info["Members"].push_back(ordered_json({
                { "Name", name },
                { "Annotation", uniform.annotation->Get() },
                { "Index", uniform.index },
                { "Offset", uniform.offset },
            }));
        }
        output["SSBOs"][name] = std::move(ssbo_info);
    }

    if (mDumpOptions) {
        json static_options = {};
        for (size_t i = 0; i < model->static_option_count; ++i) {
            const auto& opt = model->static_option_array[i];
            const std::string_view name = opt.name->Get();
            static_options[name] = { { "Values", json::array() } };
            for (size_t j = 0; j < opt.choice_count; ++j) {
                static_options[name]["Values"].push_back(opt.choice_dict->entries[j + 1].key->Get());
                if (j == opt.default_choice) {
                    static_options[name]["Default"] = opt.choice_dict->entries[j + 1].key->Get();
                }
            }
        }
        json dynamic_options = {};
        for (size_t i = 0; i < model->dynamic_option_count; ++i) {
            const auto& opt = model->dynamic_option_array[i];
            const std::string_view name = opt.name->Get();
            dynamic_options[name] = { { "Values", json::array() } };
            for (size_t j = 0; j < opt.choice_count; ++j) {
                dynamic_options[name]["Values"].push_back(opt.choice_dict->entries[j + 1].key->Get());
                if (j == opt.default_choice) {
                    dynamic_options[name]["Default"] = opt.choice_dict->entries[j + 1].key->Get();
                }
            }
        }
        output["Static Options"] = static_options;
        output["Dynamic Options"] = dynamic_options;
    }
}

void ShaderInfoPrinter::Run() {
    if (!Initialize())
        return;
    
    const auto shader_file = mContext.GetShaderArchive();
    
    ordered_json output = {};
    if (mProgramIndex < 0) {
        output["Archive Name"] = shader_file->archive->name->Get();
        if (mModelName == "") {
            // dump all shading models
            output["Models"] = {};
            for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
                const g3d2::ResShadingModel* model = shader_file->archive->shading_model_array + i;
                const std::string_view name = model->name->Get();
                ordered_json info = ordered_json();
                ProcessModel(info, model);
                output["Models"][name] = std::move(info);
            }
        } else {
            // dump only the specified model
            g3d2::ResShadingModel* model = nullptr;
            for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
                if (shader_file->archive->shading_model_array[i].name->Get() == mModelName) {
                    model = shader_file->archive->shading_model_array + i;
                    break;
                }
            }
            if (model == nullptr) {
                std::cout << "No shading model named " << mModelName << "\n";
                return;
            }
            output["Model Name"] = model->name->Get();
            ProcessModel(output, model);
        }
    } else {
        g3d2::ResShadingModel* model = nullptr;
        for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
            if (shader_file->archive->shading_model_array[i].name->Get() == mModelName) {
                model = shader_file->archive->shading_model_array + i;
                break;
            }
        }
        if (model == nullptr) {
            std::cout << "No shading model named " << mModelName << "\n";
            return;
        }
        if (mProgramIndex >= model->shader_program_count) {
            std::cout << std::format("Out of range program index for model {}: {}\n", model->name->Get(), mProgramIndex);
            return;
        }

        output["Archive Name"] = shader_file->archive->name->Get();
        output["Model Name"] = model->name->Get();
        output["Program Index"] = mProgramIndex;

        const auto& program = model->program_array[mProgramIndex];

        if (program.variation->binary == nullptr) {
            std::cout << std::format("No associated shader binary with program index {} for model {}\n", mProgramIndex, model->name->Get());
            return;
        }

        const auto* ifc_table = program.variation->binary->interfaces;

        constexpr static auto cShaderStageNames = std::to_array<std::string_view>({
            "Vertex", "Tessellation Control", "Tessellation Evaluation",
            "Geometry", "Fragment", "Compute",
        });

        constexpr static auto cInterfaceTypeNames = std::to_array<std::string_view>({
            "Input Attributes", "Output Attributes", "Samplers", "Uniforms",
            "Storage Buffers", "Images", "Separate Textures", "Separate Samplers",
        });

        for (u32 stage = 0; stage < gfx::ShaderStage_End; ++stage) {
            const auto* table = ifc_table->stages[stage];
            if (table == nullptr) {
                continue;
            }
            ordered_json stage_info = {};
            for (u32 ifc_type = 0; ifc_type < gfx::ShaderInterfaceType_End; ++ifc_type) {
                const auto* dic = table->GetResDic(static_cast<gfx::ShaderInterfaceType>(ifc_type));
                if (dic == nullptr) {
                    continue;
                }
                ordered_json ifc_info = {};
                for (size_t i = 0; i < dic->node_count; ++i) {
                    const std::string_view name = dic->entries[i + 1].key->Get();
                    ifc_info[name] = table->GetInterfaceSlot(static_cast<gfx::ShaderInterfaceType>(ifc_type), i);
                }
                stage_info[cInterfaceTypeNames[ifc_type]] = std::move(ifc_info);
            }
            const auto* code_ptr = program.variation->binary->shader_code_ptrs[stage];
            stage_info["Code Size"] = code_ptr->code_size;
            stage_info["Control Size"] = code_ptr->control_size;
            output[cShaderStageNames[stage]] = std::move(stage_info);
            if (mDumpBin) {
                const std::string basename = std::format("{}_{}_{}_{}", shader_file->archive->name->Get(), model->name->Get(), mProgramIndex, cShaderStageNames[stage]);
                const Path dir = Path(mOutputPath).parent_path();
                const std::string code_name = (dir / Path(std::format("{}_code.bin", basename))).string();
                const std::string control_name = (dir / Path(std::format("{}_control.bin", basename))).string();
                mContext.WriteFile(code_name, { code_ptr->code, code_ptr->code_size });
                mContext.WriteFile(control_name, { code_ptr->control, code_ptr->control_size });
            }
        }
    }

    if (mOutputPath != "-") {
        std::ofstream out(mOutputPath);
        out << std::setw(2) << output << std::endl;
    } else {
        std::cout << std::setw(2) << output << std::endl;
    }
}