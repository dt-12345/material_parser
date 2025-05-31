import json
from json import JSONEncoder

# https://stackoverflow.com/a/52283196
class MarkedList:
    _list = None
    def __init__(self, l):
        self._list = l

class CustomJSONEncoder(JSONEncoder):
    def default(self, o):
        if isinstance(o, MarkedList):
            return "##<{}>##".format(json.dumps(o._list))

with open("Materials.json", "r", encoding="utf-8") as f:
    data = json.load(f)

for file in data:
    for model in data[file]:
        for mat in data[file][model]:
            data[file][model][mat]["Samplers"] = MarkedList(data[file][model][mat]["Samplers"])
            data[file][model][mat]["Skin Counts"] = MarkedList(data[file][model][mat]["Skin Counts"])
            data[file][model][mat]["Textures"] = MarkedList(data[file][model][mat]["Textures"])

b = json.dumps(data, indent=2, separators=(', ', ': '), cls=CustomJSONEncoder)
b = b.replace('"##<', "").replace('>##"', "").replace("\\\"", '"')

with open("Materials.json", "w", encoding="utf-8") as f:
    f.write(b)