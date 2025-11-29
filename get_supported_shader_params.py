import json
import os
import subprocess
import yaml

def run_command(cmd: list[str]) -> str:
    res: str = ""
    p: subprocess.Popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    while (exit_code := p.poll()) is None:
        res += p.stdout.read().decode()
    if exit_code != 0:
        raise subprocess.CalledProcessError(f"{cmd} exited with {exit_code}:\n{p.stderr.read().decode()}")
    return res

if __name__ == "__main__":
    import sys

    mat_tool: str = sys.argv[1]
    archive: str = sys.argv[2]
    info_path: str = sys.argv[3]
    shader_info_path: str = sys.argv[4]
    file: str = sys.argv[5]
    model: str = sys.argv[6]
    material: str = sys.argv[7]

    with open(info_path) as f:
        info: dict = json.load(f)

    with open(shader_info_path) as f:
        shader_info: dict = json.load(f)
    
    with open("temp_query.json", "w") as f:
        json.dump(info[file][model][material], f, indent=0)

    output: dict = yaml.load(run_command([mat_tool, "search", "temp_query.json", "--shader-archive", archive]), Loader=yaml.CLoader)

    for index in output:
        print(index, shader_info[index.replace("Shader Program ", "")])
    
    os.unlink("temp_query.json")