import json

f = open("src/game/behaviors/progress.json", "r", encoding="utf-8")
datas = json.load(f)

total_line = 0
understood_line = 0
for data in datas:
    line_count = data["line_count"]
    total_line += line_count
    if data["understood"]:
        understood_line += line_count

percentage = understood_line / total_line * 100
print(percentage, "%")