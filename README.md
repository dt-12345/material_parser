# Material Tool

A command-line tool for querying information about materials and shaders in *Tears of the Kingdom*

## Usage

Run `mat-tool` to bring up the following help message:

```
Material Tool
Usage: mat-tool [action]

Actions:
  dump [options] romfs_path
    Dumps information about materials found in models
    Arguments:
      --shader-archive         : path to material bfsha shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'
      --external-binary-string : path to ExternalBinaryString.bfres.mc; defaults to romfs_path/Shader/ExternalBinaryString.bfres.mc
      --out                    : path to file to output to; defaults to 'Materials.json'
      romfs_path               : path to romfs with Models directory
  search [options] query_config
    Searches a shader archive for matching shaders given the a set of conditions (useful for material design)
    Arguments:
      --shader-archive         : path to the shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'
      --verbose                : print all non-default shader options (as opposed to just the specified ones); defaults to false
      --out                    : path to file to output to; defaults to stdout
      query_config             : path to JSON search config file
  info [options] shading_model_name
    Outputs information about the possible shader options in the provided shading model
    Arguments:
      --shader-archive         : path to the shader archive (needs to be decompressed); defaults to 'material.Product.140.product.Nin_NX_NVN.bfsha'
      --archive-name           : name of shader archive; defaults to the shading model name
      --out                    : path to file to output to; defaults to 'ShaderInfo.yml'
      shading_model_name       : name of shading model within archive to print info about

Examples:
  Dump information about materials in romfs:
    mat-tool dump TotK_ROMFS/
  Search for matching shaders:
    mat-tool search query.json
  Output information about the material shading model in material.Product.140.product.Nin_NX_NVN.bfsha
    mat-tool info --shader-archive material.Product.140.product.Nin_NX_NVN.bfsha material
```

## Search Query JSON Example

```json
{
    "Archive Name" : "material",            // Shader archive name, defaults to "material" if unspecified
    "Model Name" : "material",              // Shading model name, defaults to "material" if unspecified
    "Static Options": {                     // Static shader options
        "o_ao_color": "400"                 // constrain an option to the specified value - this matches shaders supporting o_ao_color == 400
    },
    "Render Info": {                        // Material Render Info
        "gsys_alpha_test_enable": null      // leave an option unconstrained - this matches shaders supporting any valid value for gsys_alpha_test_enable
    },
    "Dynamic Options": {                    // Dynamic Shader Options
        // Note: gsys_weight is equivalent to a shape's skin count
        "gsys_weight": [0, 1, 2, 3, 4, 5]   // constrain an option to a range of values - this matches shaders supporting skin counts of 0-5
    }
}
```

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Note: I don't know if this is just me, but for some reason MSVC seems to completely choke when compiling this? It gets stuck for a while on seemingly nothing then CPU usage goes to 100% while the project is open in Visual Studio. GCC seems to have no issues so I don't know if this is an issue with this project specifically or MSVC.