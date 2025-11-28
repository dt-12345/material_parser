#include "app.h"

#include <cctype>
#include <cstring>
#include <iostream>
#include <fstream>

#define MAX_FILEPATH 0x1000

static const std::string ParseInput(int argc, const char** argv, int index) {
    if (argc < 2 + index) {
        return "";
    }

    size_t size = strnlen(argv[1 + index], MAX_FILEPATH);
    std::string value{argv[1 + index], argv[1 + index] + size};
    return value;
}

int main(int argc , const char* argv[]) {

    int opt_index = 0;
    std::string opt = ParseInput(argc, argv, opt_index++);
    std::transform(opt.begin(), opt.end(), opt.begin(), [](unsigned char c){ return std::tolower(c); });

    if (opt == "dump") {
        std::string material_archive_path = "";
        std::string external_binary_string_path = "";
        while (opt_index + 3 < argc) {
            const std::string next_opt = ParseInput(argc, argv, opt_index++);
            if (next_opt == "--shader-archive") {
                material_archive_path = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--external-binary-string") {
                external_binary_string_path = ParseInput(argc, argv, opt_index++);
            }
        }
        std::string romfs_path = ParseInput(argc, argv, opt_index);
        MaterialParser(romfs_path, material_archive_path, external_binary_string_path).Run();
    } else if (opt == "search") {
        std::string material_archive_path = "";
        bool verbose = false;
        while (opt_index + 3 < argc) {
            const std::string next_opt = ParseInput(argc, argv, opt_index++);
            if (next_opt == "--shader-archive") {
                material_archive_path = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--verbose") {
                verbose = true;
            }
        }
        std::string config_path = ParseInput(argc, argv, opt_index);
        MaterialSearcher(config_path, material_archive_path, verbose).Run();
    } else if (opt == "info") {
        std::string archive = "";
        std::string archive_path = "";
        while (opt_index + 3 < argc) {
            const std::string next_opt = ParseInput(argc, argv, opt_index++);
            if (next_opt == "--shader-archive") {
                archive_path = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--archive-name") {
                archive = ParseInput(argc, argv, opt_index++);
            }
        }
        std::string model = ParseInput(argc, argv, opt_index);
        ShaderInfoPrinter(model, archive, archive_path).Run();
    } else if (opt == "" || opt == "help") {
        std::cout <<
        "Material Tool\n"
        "Usage: mat-tool [action]\n\n"
        "Actions:\n"
        "  dump [options] romfs_path\n"
        "    Dumps information about materials found in models to Materials.json\n"
        "    Arguments:\n"
        "      --shader-archive         : path to material bfsha shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'\n"
        "      --external-binary-string : path to ExternalBinaryString.bfres.mc; defaults to romfs_path/Shader/ExternalBinaryString.bfres.mc\n"
        "      romfs_path               : path to romfs with Models directory\n"
        "  search [options] query_config\n"
        "    Searches a shader archive for matching shaders given the a set of conditions (useful for material design)\n"
        "    Arguments:\n"
        "      --shader-archive         : path to the shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'\n"
        "      --verbose                : print all non-default shader options (as opposed to just the specified ones); defaults to false\n"
        "      query_config             : path to JSON search config file\n"
        "  info [options] shading_model_name\n"
        "    Outputs information about the possible shader options in the provided shading model to ShaderInfo.yml\n"
        "    Arguments:\n"
        "      --shader-archive         : path to the shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'\n"
        "      --archive-name           : name of shader archive; defaults to the shading model name\n"
        "      shading_model_name       : name of shading model within archive to print info about\n\n"
        "Examples:\n"
        "  Dump information about materials in romfs:\n"
        "    mat-tool dump TotK_ROMFS/\n"
        "  Search for matching shaders:\n"
        "    mat-tool search query.json\n"
        "  Output information about the material shading model in material.Product.140.product.Nin_NX_NVN.bfsha\n"
        "    mat-tool info --shader-archive material.Product.140.product.Nin_NX_NVN.bfsha material\n";
    } else {
        std::cout << "Unknown option: " << opt << "\n";
    }

    return 0;
}