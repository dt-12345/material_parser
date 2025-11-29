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

static void MakeMissingDirectories(const std::string_view path) {
    const Path dir = Path(path).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
}

int main(int argc , const char* argv[]) {

    int opt_index = 0;
    std::string opt = ParseInput(argc, argv, opt_index++);
    std::transform(opt.begin(), opt.end(), opt.begin(), [](unsigned char c){ return std::tolower(c); });

    if (opt == "dump") {
        std::string material_archive_path = "";
        std::string external_binary_string_path = "";
        std::string output_path = "";
        std::string romfs_path = "";
        while (opt_index + 1 < argc) {
            const std::string next_opt = ParseInput(argc, argv, opt_index++);
            if (next_opt == "--shader-archive") {
                material_archive_path = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--external-binary-string") {
                external_binary_string_path = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--out") {
                output_path = ParseInput(argc, argv, opt_index++);
                if (output_path == "-") {
                    output_path = "";
                }
            } else if (next_opt == "--romfs") {
                romfs_path = ParseInput(argc, argv, opt_index++);
            } else {
                romfs_path = next_opt;
            }
        }
        MakeMissingDirectories(output_path);
        try {
            MaterialParser(romfs_path, material_archive_path, external_binary_string_path, output_path).Run();
        } catch (const std::runtime_error& e) {
            std::cerr << "Exception caught: [" << e.what() << "]\n";
            return 1;
        }
    } else if (opt == "search") {
        std::string config_path = "";
        std::string material_archive_path = "";
        std::string output_path = "";
        bool verbose = false;
        while (opt_index + 1 < argc) {
            const std::string next_opt = ParseInput(argc, argv, opt_index++);
            if (next_opt == "--shader-archive") {
                material_archive_path = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--verbose") {
                verbose = true;
            } else if (next_opt == "--out") {
                output_path = ParseInput(argc, argv, opt_index++);
                if (output_path == "-") {
                    output_path = "";
                }
            } else if (next_opt == "--config") {
                config_path = ParseInput(argc, argv, opt_index++);
            } else {
                config_path = next_opt;
            }
        }
        MakeMissingDirectories(output_path);
        try {
            MaterialSearcher(config_path, material_archive_path, output_path, verbose).Run();
        } catch (const std::runtime_error& e) {
            std::cerr << "Exception caught: [" << e.what() << "]\n";
            return 1;
        }
    } else if (opt == "info") {
        std::string archive_path = "";
        std::string output_path = "";
        std::string model_name = "";
        bool dump_opts = true;
        int program_index = -1;
        while (opt_index + 1 < argc) {
            const std::string next_opt = ParseInput(argc, argv, opt_index++);
            if (next_opt == "--out") {
                output_path = ParseInput(argc, argv, opt_index++);
                if (output_path == "-") {
                    output_path = "";
                }
            } else if (next_opt == "--no-options") {
                dump_opts = false;
            } else if (next_opt == "--model-name") {
                model_name = ParseInput(argc, argv, opt_index++);
            } else if (next_opt == "--program" || next_opt == "--variation" || next_opt == "--index") {
                program_index = std::stoi(ParseInput(argc, argv, opt_index++));
            } else if (next_opt == "--shader-archive") {
                archive_path = ParseInput(argc, argv, opt_index++);
            } else {
                archive_path = next_opt;
            }
        }
        MakeMissingDirectories(output_path);
        try {
            ShaderInfoPrinter(archive_path, output_path, model_name, program_index, dump_opts).Run();
        } catch (const std::runtime_error& e) {
            std::cerr << "Exception caught: [" << e.what() << "]\n";
            return 1;
        }
    } else if (opt == "" || opt == "help") {
        std::cout <<
        "Material Tool\n"
        "Usage: mat-tool [action]\n\n"
        "Actions:\n"
        "  dump [options] romfs_path\n"
        "    Dumps information about materials found in models\n"
        "    Arguments:\n"
        "      --shader-archive         : path to material bfsha shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'\n"
        "      --external-binary-string : path to ExternalBinaryString.bfres.mc; defaults to romfs_path/Shader/ExternalBinaryString.bfres.mc\n"
        "      --out                    : path to file to output to; defaults to 'Materials.json'\n"
        "      romfs_path               : path to romfs with Models directory\n"
        "  search [options] query_config\n"
        "    Searches a shader archive for matching shaders given the a set of conditions (useful for material design)\n"
        "    Arguments:\n"
        "      --shader-archive         : path to the shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'\n"
        "      --verbose                : print all non-default shader options (as opposed to just the specified ones); defaults to false\n"
        "      --out                    : path to file to output to; defaults to stdout\n"
        "      query_config             : path to JSON search config file\n"
        "  info [options] shading_model_name\n"
        "    Outputs information about specified shading model(s) (or shader program if a program index is provided)\n"
        "    Arguments:\n"
        "      --shader-archive         : path to the shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'\n"
        "      --model-name             : name of shading model, specify if information only about a specific model is desired; defaults to all models in the archive\n"
        "      --index                  : index of shader program to dump information about, ignore to dump information about an entire shading model; defaults to -1\n"
        "      --no-options             : skip dumping of shader options in output; defaults to include options\n"
        "      --out                    : path to file to output to; defaults to 'ShaderInfo.json'\n"
        "      shading_model_name       : name of shading model within archive to print info about\n\n"
        "Examples:\n"
        "  Dump information about materials in romfs:\n"
        "    mat-tool dump TotK_ROMFS/\n"
        "  Search for matching shaders:\n"
        "    mat-tool search query.json\n"
        "  Output information about the material shading model in material.Product.140.product.Nin_NX_NVN.bfsha\n"
        "    mat-tool info --shader-archive material.Product.140.product.Nin_NX_NVN.bfsha material\n";
    } else {
        std::cerr << "Unknown option: " << opt << "\n";
        return 1;
    }

    return 0;
}