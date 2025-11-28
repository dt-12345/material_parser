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

    if (!mContext.DecompressFile(mExternalBinaryStringPath, mContext.mExternalBinaryStringStorage)) {
        std::cout << "Failed to open " << mExternalBinaryStringPath << "\n";
        return false;
    }
    
    ResFile::ResCast(mContext.mExternalBinaryStringStorage.data());

    if (!mContext.ReadFile(mMaterialArchivePath, mContext.mShaderArchiveStorage)) {
        std::cout << "Failed to open " << mMaterialArchivePath << "\n";
        return false;
    }

    g3d2::ResShaderFile::ResCast(mContext.mShaderArchiveStorage.data());

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

    if (!mContext.ReadFile(mMaterialArchivePath, mContext.mShaderArchiveStorage)) {
        std::cout << "Failed to open " << mMaterialArchivePath << "\n";
        return false;
    }

    g3d2::ResShaderFile::ResCast(mContext.mShaderArchiveStorage.data());

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

void MaterialSearcher::Print(const g3d2::ResShadingModel* model, const u32* keys) const {
    *mOutStream << "Shader found with the following options:\n";
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
    const std::string archive_name = data.value("Archive Name", "material");
    const std::string model_name = data.value("Model Name", "material");
    
    const g3d2::ResShaderFile* shader_file = mContext.GetShaderArchive();
    if (shader_file->archive->name->Get() != archive_name) {
        std::cout << std::format("Expected archive {} but got {}\n", archive_name, shader_file->archive->name->Get());
        return;
    }

    g3d2::ResShadingModel* model = nullptr;
    for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
        if (shader_file->archive->shading_model_array[i].name->Get() == model_name) {
            model = shader_file->archive->shading_model_array + i;
            break;
        }
    }

    if (model == nullptr) {
        std::cout << std::format("No model named {}\n", model_name);
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
            Print(model, keys);
        }
    }
}

bool ShaderInfoPrinter::Initialize() {
    if (mInitialized)
        return mInitialized;

    if (!mContext.ReadFile(mArchivePath, mContext.mShaderArchiveStorage)) {
        std::cout << "Failed to open " << mArchivePath << "\n";
        return false;
    }

    g3d2::ResShaderFile::ResCast(mContext.mShaderArchiveStorage.data());

    mInitialized = true;

    return true;
}

void ShaderInfoPrinter::Run() {
    if (!Initialize())
        return;
    
    const auto shader_file = mContext.GetShaderArchive();
    if (shader_file->archive->name->Get() != mArchiveName) {
        std::cout << "Archive is named " << shader_file->archive->name->Get() << " when " << mArchiveName << " was expected\n";
        return;
    }

    const g3d2::ResShadingModel* model = nullptr;
    for (size_t i = 0; i < shader_file->archive->shading_model_count; ++i) {
        if (shader_file->archive->shading_model_array[i].name->Get() == mModelName) {
            model = shader_file->archive->shading_model_array + i;
            break;
        }
    }

    if (model == nullptr) {
        std::cout << "No model named " << mModelName << "\n";
        return;
    }

    std::ofstream out(mOutputPath);

    out << std::format("# Shader Options for {}\n", mModelName);
    out << "Static Options:\n";
    for (size_t i = 0; i < model->static_option_count; ++i) {
        const auto& opt = model->static_option_array[i];
        out << "  " << opt.name->Get() << ":\n";
        for (size_t j = 0; j < opt.choice_count; ++j) {
            if (j == opt.default_choice) {
                out << "  - " << opt.choice_dict->entries[j + 1].key->Get() << " # Default\n";
            } else {
                out << "  - " << opt.choice_dict->entries[j + 1].key->Get() << "\n";
            }
        }
    }
    out << "Dynamic Options:\n";
    for (size_t i = 0; i < model->dynamic_option_count; ++i) {
        const auto& opt = model->dynamic_option_array[i];
        out << "  " << opt.name->Get() << ":\n";
        for (size_t j = 0; j < opt.choice_count; ++j) {
            if (j == opt.default_choice) {
                out << "  - " << opt.choice_dict->entries[j + 1].key->Get() << " # Default\n";
            } else {
                out << "  - " << opt.choice_dict->entries[j + 1].key->Get() << "\n";
            }
        }
    }
}