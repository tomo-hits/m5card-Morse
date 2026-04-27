# m5card-Morse

M5Stack Cardputer で動作するモールス信号変換器です。

本体キーボードで入力したテキストをモールス信号に変換して画面に表示し、内蔵スピーカーで実際に鳴らします。

## 機能

- 英数字をモールス信号に変換
- ひらがな・カタカナを和文モールス信号に変換
- 濁音・半濁音を和文モールスの濁点・半濁点符号で表現
- Cardputer 本体キーボードでローマ字からひらがな入力
- 一部の記号に対応
- 入力テキストと変換後のモールス信号を画面表示
- `Enter` キーでモールス信号を再生
- `Backspace` キーで入力を削除
- 初期表示は `SOS`

## 対応文字

- `A-Z`
- `0-9`
- `. , ? ! / - = + @ ( ) :`
- ひらがな
- カタカナ
- `、 。 ー （ ）`
- スペースは単語区切りとして `/` に変換されます

ひらがなとカタカナは同じ和文モールス符号に変換されます。小書き文字は対応する通常文字として扱います。

## 必要なもの

- M5Stack Cardputer
- USB-C ケーブル
- PlatformIO

## ビルド

```sh
pio run
```

`pio` が PATH にない場合は、PlatformIO IDE または PlatformIO Core のインストール状態を確認してください。

## 書き込み

Cardputer をダウンロードモードにします。

1. 本体上側のスイッチを `OFF`
2. `G0` ボタンを押したまま USB-C ケーブルを接続
3. 接続後に `G0` ボタンを離す
4. 次のコマンドで書き込み

```sh
pio run --target upload
```

`pio` が PATH にない場合は、PlatformIO IDE から Upload を実行することもできます。

## 使い方

1. Cardputer を起動します
2. キーボードで文字を入力します
3. 画面にモールス信号が表示されます
4. `Enter` を押すとブザーで再生します
5. `Backspace` で 1 文字削除できます

入力が空の状態で `Enter` を押すと、`SOS` が再セットされて再生されます。

起動時は `JP` モードです。ローマ字で入力すると、ひらがなに変換されます。

例:

```text
konnichiha
```

入力結果:

```text
こんにちは
```

`Tab` キーで `JP` と `ABC` を切り替えられます。英数字をそのまま入力したい場合は `ABC` モードにしてください。

シリアルモニターから UTF-8 のテキストを 1 行送信することもできます。改行を受け取ると画面の入力テキストがその内容に置き換わります。

```sh
pio device monitor
```

## 設定

音の高さや速度は [src/main.cpp](src/main.cpp) の先頭付近で変更できます。

```cpp
constexpr int kToneHz = 700;
constexpr int kWordsPerMinute = 15;
```

- `kToneHz`: ブザー音の周波数
- `kWordsPerMinute`: モールス信号の再生速度

## PlatformIO 設定

プロジェクト設定は [platformio.ini](platformio.ini) にあります。

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino
monitor_speed = 115200
lib_deps =
  m5stack/M5Cardputer@^1.1.1
```

## ライセンス

このプロジェクトは [MIT License](LICENSE) で公開しています。
