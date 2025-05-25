#include "material_parser.h"
#include "shader.h"

#include "mc_MeshCodec.h"

#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

using Path = std::filesystem::path;
using DirectoryIter = std::filesystem::recursive_directory_iterator;

static std::vector<unsigned char> sWorkMemory(0x10000000);

static const std::array<std::string, 16> cNumberNames = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"
};

bool MaterialParser::Initialize() {
    if (mInitialized)
        return mInitialized;

    const Path shader_path = Path(mRomfsPath) / Path("Shader");

    if (!DecompressFile((shader_path / Path("ExternalBinaryString.bfres.mc")).string(), mExternalBinaryStringStorage))
        return false;

    if (!ReadFile("material.Product.110.product.Nin_NX_NVN.bfsha", mShaderArchiveStorage))
        return false;

    ResFile::ResCast(mExternalBinaryStringStorage.data());
    g3d2::ResShaderFile::ResCast(mShaderArchiveStorage.data());

    mInitialized = true;

    return true;
}

bool MaterialParser::ReadFile(const std::string path, std::vector<u8>& data) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        return false;
    
    data.resize(file.tellg());
    file.seekg(0);

    file.read(reinterpret_cast<char*>(data.data()), data.size());

    file.close();

    return true;
}

void MaterialParser::WriteFile(const std::string path, const std::span<const u8>& data) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
}

bool MaterialParser::DecompressFile(const std::string path, std::vector<u8>& data) {
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

void MaterialParser::ProcessFile(const std::string path) {
    std::vector<u8> fileBuffer{};
    
    if (!DecompressFile(path, fileBuffer))
        throw std::runtime_error(std::format("Failed to read file: {}", path));
    
    ResFile* file = SetupFile(fileBuffer.data());

    if (file == nullptr)
        throw std::runtime_error(std::format("Failed to setup file: {}", path));

    const g3d2::ResShaderFile* shader_file = GetShaderArchive();

    const std::string filename = Path(path).filename().string();

    std::cout << filename << "\n";

    mOutput[filename] = json({});
    
    for (size_t i = 0; i < file->model_count; ++i) {
        const auto& model = file->models[i];
        const std::string_view model_name = model.name->Get();
        mOutput[filename][model_name] = json({});

        for (size_t j = 0; j < model.material_count; ++j) {
            const auto& mat = model.material_array[j];
            const std::string_view mat_name = mat.name->Get();
            json mat_info = {
                {"Samplers", json::array()},
                {"Textures", json::array()},
                {"Render Info", json({})},
                {"Skin Counts", json::array()},
            };

            ShaderSelector selector{};
            selector.LoadOptions(&mat);

            for (size_t i = 0; i < mat.sampler_count; ++i)
                mat_info["Samplers"].push_back(mat.sampler_dict->entries[i + 1].key->Get());

            for (size_t i = 0; i < mat.texture_count; ++i)
                mat_info["Textures"].push_back(mat.texture_name_array[i]->Get());

            for (u8 i = 0; i < 0x10; ++i) {
                selector.SetOption("gsys_weight", cNumberNames[i]);

                const int index = selector.Search(shader_file);

                if (index != -1)
                    mat_info["Skin Counts"].push_back(i);
            }

            for (size_t i = 0; i < mat.shader_data->shader_reflection->render_info_count; ++i) {
                const std::string_view render_info_name = mat.shader_data->shader_reflection->render_info_array[i].name->Get();
                const u16 offset = mat.render_info_value_offset_array[i];

                switch (mat.shader_data->shader_reflection->render_info_array[i].type) {
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

            mOutput[filename][model_name][mat_name] = std::move(mat_info);
        }
    }
}

bool MaterialParser::Run() {
    if (!Initialize())
        return false;

    const Path model_path = mRomfsPath / Path("Model");

    for (const auto& entry : DirectoryIter(model_path)) {
        if (entry.path().extension() == ".mc")
            ProcessFile(entry.path().string());
    }

    std::ofstream out("Materials.json");
    out << std::setw(2) << mOutput << std::endl;

    return true;
}