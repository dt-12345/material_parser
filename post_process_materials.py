import json
import sys

if __name__ == "__main__":

    materials_path: str = "Materials.json"
    if len(sys.argv) > 1:
        materials_path = sys.argv[1]
    
    params_path: str = "SupportedParamsByShader.json"
    if len(sys.argv) > 2:
        params_path = sys.argv[2]
    
    with open(materials_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    with open(params_path, "r", encoding="utf-8") as f:
        params = json.load(f)
    
    for file in data:
        for model in data[file]:
            for mat in data[file][model]:
                data[file][model][mat]["Supported Params"] = []
                for idx in data[file][model][mat]["Shader Indices"]:
                    data[file][model][mat]["Supported Params"].append(params[str(idx)])
    
    with open(materials_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)