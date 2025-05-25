#include "material_parser.h"

#include <cstring>

#define MAX_FILEPATH 0x1000

const std::string ParseInput(int argc, const char** argv, int index) {
    static std::string null_string = "";

    if (argc < 2 + index) {
        return null_string;
    }

    size_t size = strnlen(argv[1 + index], MAX_FILEPATH);
    std::string value{argv[1 + index], argv[1 + index] + size};
    return value;
}

int main(int argc [[maybe_unused]], const char* argv[[maybe_unused]][]) {

    const std::string romfs_path = ParseInput(argc, argv, 0);

    MaterialParser parser(romfs_path);

    parser.Run();

    return 0;
}