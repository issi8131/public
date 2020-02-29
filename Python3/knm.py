from msvcrt import getch

# 注）OSはWindows、コマンドプロンプトまたは
# Pythonランチャー(py.exe)で動作することを確認

input_que = [0] * 10  # 入力キーの文字コードを保持するキュー
CMD = ["↑", "↑", "↓", "↓", "←", "→", "←", "→", " b", " a"]  # 受理するコマンド列


def translate(code):  # 文字コードを文字列に変換して返す
    if 32 <= code and code <= 126:  # asciiコード上表示可能文字なら
        return " " + chr(code)  # 表示の都合上スペース追加
    code -= 224
    if code == 72:
        return "↑"
    elif code == 80:
        return "↓"
    elif code == 75:
        return "←"
    elif code == 77:
        return "→"
    return "？"


# スクリプトここからスタート
print()
while True:
    cmd = list(map(translate, input_que))  # コマンド文字リスト取得
    print('\r' + " ".join(cmd), end="")
    if cmd == CMD:
        break
    key = ord(getch())
    if key == 224:  # 矢印とか制御キーだったら
        key += ord(getch())
    input_que = input_que[1:]  # 1つキューから追い出す
    input_que.append(key)  # キューにコマンドを追加

print("\n\nコマンド入力成功")
input("Enterで終了…")
