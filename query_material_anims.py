import argparse
import json
import os
import subprocess

COMPONENTS: tuple[str, str, str, str] = ("x", "y", "z", "w")

def run_command(cmd: list[str]) -> str:
    res: str = ""
    p: subprocess.Popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while (exit_code := p.poll()) is None:
        res += p.stdout.read().decode()
    return res
        
def convert_to_access(offset: int) -> str:
    return f"fp_c11.data[{offset // 0x10}].{COMPONENTS[offset % 0x10 // 4]}"

def get_shader_params(info: dict) -> dict[str, int]:
    params: dict[str, int] = {}
    for ubo in info["Models"]["material"]["UBOs"]:
        if ubo != "gsys_material":
            continue
        for uniform in info["Models"]["material"]["UBOs"][ubo]["Uniforms"]:
            params[uniform["Name"]] = convert_to_access(uniform["Offset"])
    return params

if __name__ == "__main__":
    
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        prog="Query Material Animation Info",
        description="Query information about supported material animations for a material"
    )
    parser.add_argument("--shader-info", help="Path to JSON file containing shader information", default="ShaderInfo.json")
    parser.add_argument("--shaders", help="Path to directoy containing extracted shaders", default="shaders")
    parser.add_argument("--ryujinx-shader-tools", help="Path to Ryujinx shader tools executable", default="Ryujinx.ShaderTools")
    args, _ = parser.parse_known_args()
    
    with open(args.shader_info, "r", encoding="utf-8") as f:
        shader_info: dict = json.load(f)

    params: dict[str, int] = get_shader_params(shader_info)

    output: dict[int, list[str]] = {}

    num_files: int = len(list(f for f in os.listdir(args.shaders) if os.path.isfile(os.path.join(args.shaders, f)))) // 4

    for i in range(num_files):
        code_path: str = os.path.join(args.shaders, f"material_material_{i}_Fragment_code.bin")
        code: str = run_command([args.ryujinx_shader_tools, code_path])
        output[i] = []
        print(f"Processing {code_path}")
        for param in params:
            if params[param] in code:
                output[i].append(param)
    
    with open("SupportedParamsByShader.json", "w", encoding="utf-8") as f:
        json.dump(output, f, indent=2)