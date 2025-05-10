import json
import os

datas = []

dir = "src/game/behaviors/"
for file in os.listdir(dir):
    f = open(dir + file, "r", encoding="utf-8")
    # 行数を取得
    lines = f.readlines()
    f.close()
    line_count = len(lines)
    data ={
        "name": file,
        "line_count": line_count,
        "understood": False
    }
    datas.append(data)

json_str = json.dumps(datas, indent=4)
print(json_str)

wf = open(dir + "progress.json", "w", encoding="utf-8")
wf.write(json_str)