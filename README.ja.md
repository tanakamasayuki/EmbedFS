# EmbedFS

EmbedFS は Arduino / ESP32 向けの小さな読み取り専用の仮想ファイルシステムです。
プログラムメモリ（フラッシュ）に `const` データとしてファイル（アセット）を埋め込み、
SD、SPIFFS、LittleFS のような一般的な Arduino 用 FS ライブラリと互換性のある
ファイルシステム風の API でアクセスできるようにします。

## 主な特徴

- フラッシュ上の配列（const データ）として保持される読み取り専用の仮想 FS。
- Arduino の `File`/`FS` スタイルの操作（open / read / exists / list / rewindDirectory 等）と互換。
- ファイルだけでなくディレクトリも開けるため、階層構造の列挙が可能。
- ESP32 等の Arduino 互換ボード向けに設計。
- Arduino CLI Wrapper が生成する `assets_embed.h` や、ディレクトリを C ヘッダに
  変換する他のツールで生成したアセットで動作。
- 実行時の書き込みは行わないため、書き換え不要の静的リソースに最適。

## 使い方の概略

1. `assets/` フォルダを Arduino CLI Wrapper などで C ヘッダ（例: `assets_embed.h`）に変換します。
   生成されたヘッダは埋め込みデータとファイルインデックス（パス、データポインタ、長さ等）を
   公開する必要があります。
2. スケッチで `assets_embed.h` をインクルードし、生成されたデータで EmbedFS を初期化します。
3. 親しみのある FS 風の呼び出しでファイルを開き、読み取ります。

注意: EmbedFS は読み取り専用です。ランタイムでのファイル更新や永続化が必要な場合は
SD や LittleFS を使用してください。

このリポジトリの `examples/BasicTest/` にあるスケッチは次のように初期化します:

```cpp
EmbedFS.begin(assets_file_names, assets_file_data, assets_file_sizes, assets_file_count);
```

つまり、生成ヘッダは次のシンボルを公開する形式になっています（名前と型の例）:

- `constexpr size_t assets_file_count` — 埋め込みファイルの数
- `const char* const assets_file_names[assets_file_count]` — ファイルパスの配列（C文字列）
- `const uint8_t* const assets_file_data[assets_file_count]` — ファイルバイトへのポインタ配列（PROGMEM/const）
- `const size_t assets_file_sizes[assets_file_count]` — 各ファイルのサイズ配列

この README で示す `begin()` は上記配列を取る形式を想定しています。Arduino CLI Wrapper が生成する
ヘッダは PROGMEM に対応しており、C 配列として定義されるため `begin()` にそのまま渡せます。

## 例（Arduino / ESP32）

以下は `examples/BasicTest/BasicTest.ino` をベースにした代表的な使い方の例です。
生成されたヘッダ（`assets_embed.h`）は前述の配列シンボルを公開していることを想定しています。

```cpp
#include <EmbedFS.h>
#include "assets_embed.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!EmbedFS.begin(assets_file_names, assets_file_data, assets_file_sizes, assets_file_count)) {
    Serial.println("EmbedFS mount failed");
    return;
  }

  Serial.println("Embedded files:");
  for (size_t i = 0; i < assets_file_count; ++i) {
    File file = EmbedFS.open(assets_file_names[i], "r");
    if (!file) {
      continue;
    }
    Serial.printf("- path: %s size: %u bytes\n", file.path(), static_cast<unsigned>(file.size()));
    while (file.available()) {
      Serial.write(file.read());
    }
    Serial.println();
    file.close();
  }

  Serial.println("Directory listing of /directory:");
  File dir = EmbedFS.open("/directory");
  if (dir && dir.isDirectory()) {
    dir.rewindDirectory();
    while (true) {
      File child = dir.openNextFile();
      if (!child) {
        break;
      }
      Serial.printf("- %s (%s)\n", child.path(), child.isDirectory() ? "dir" : "file");
      child.close();
    }
    dir.close();
  }
}

void loop() {
  delay(10000);
}
```

BasicTest では 10 秒ごとに上記処理を繰り返し、埋め込んだファイルの中身と
`/directory` 配下の内容をシリアルモニタに出力します。

## 推奨 API（コントラクト）

EmbedFS を Arduino プロジェクトで扱いやすくするため、最小限のコントラクトは
以下の通りです。

- `bool begin(const char* const file_names[], const uint8_t* const file_data[], const size_t file_sizes[], size_t file_count)`
  - 生成ヘッダの配列（ファイル名、データポインタ、サイズ、件数）で初期化します。
- `File open(const char* path, const char* mode = "r")`
  - 読み取りモード（`"r"`）でファイルを開きます。返却された `File` は `read()`、`available()`、`size()`、
    `path()`、`name()` をサポートします。ディレクトリパスを渡した場合は `isDirectory()` が true の
    ディレクトリエントリを返し、`openNextFile()` や `rewindDirectory()` で走査できます。
- `bool exists(const char* path)`
  - 指定パスの埋め込みファイルまたはディレクトリが存在するかを返します。
- `size_t totalBytes()` / `size_t usedBytes()`
  - 埋め込まれている合計サイズを返します（読み取り専用のため、両者は同じ値です）。

エラー動作:
- `begin()` は無効なポインタや件数 0 のときに `false` を返します。
- `open()` はファイル/ディレクトリが見つからない、もしくはサポート外のモードの場合に
  無効な `File` を返します。

設計上の注意: 読み取り専用 API に留め、書き込みが必要な場合は SD や LittleFS を使ってください。

クラス構造の推奨（LittleFS 風）

ユーザに馴染み深くするため、LittleFS のようなクラス構造を持ちグローバルインスタンス `EmbedFS` を提供しています。
実装は以下のような構成です。

```cpp
class EmbedFSFS : public FS {
public:
  bool begin(const char* const file_names[], const uint8_t* const file_data[], const size_t file_sizes[], size_t file_count);
  bool begin(bool formatOnFail = false, const char* basePath = "/embedfs", uint8_t maxOpenFiles = 10, const char* partitionLabel = nullptr);
  bool exists(const char* path) const;
  File open(const char* path, const char* mode = "r") const;
  void end();
  size_t totalBytes();
  size_t usedBytes();
};

extern EmbedFSFS EmbedFS;
```

`examples/BasicTest/` では `EmbedFS.begin(assets_file_names, assets_file_data, assets_file_sizes, assets_file_count);` を呼び出し、
返ってきた `File` でテキストをストリームしながら読み出し、さらに `/directory` を開いて
`openNextFile()` で子要素を列挙する例を示しています。

## `assets_embed.h` の生成方法

推奨: Arduino CLI Wrapper（`assets/` を `assets_embed.h` に変換するツール）を利用します。
生成ヘッダは通常、ファイルデータとインデックステーブルを含みます。最小限の構成は以下の通りです。

- PROGMEM / const に格納されたファイルバイトの配列
- パス、データポインタ、長さを含む構造体配列（インデックス）
- エントリ件数を示すシンボル

Arduino CLI Wrapper を使わない場合は、スクリプトで同様のヘッダを作成できます。
簡単な生成スクリプト（参考）:

```py
#!/usr/bin/env python3
import sys
from pathlib import Path

out = []
files = list(Path('assets').rglob('*'))

for p in files:
    if p.is_file():
        name = '/' + str(p.relative_to('assets')).replace('\\', '/')
        b = p.read_bytes()
        arr = ','.join(str(x) for x in b)
        out.append(f"// {name}\nstatic const uint8_t data_{len(out)}[] PROGMEM = {{{arr}}};\n")

print('\n'.join(out))
```

ただし、`EmbedFS::begin()` が期待する形式（どのシンボル名、どの構造体レイアウトか）に合わせて
インデックスを構成する必要があります。

## examples フォルダ

このリポジトリの `examples/BasicTest/` を参照してください。Arduino のスケッチに加え、
Arduino CLI Wrapper で生成した `assets_embed.h` のサンプルを含んでおり、テスト用の期待される
ヘッダ形式とディレクトリ列挙の流れが確認できます。

## 制限と注意点

- 読み取り専用です。実行時の更新はできません。
- フラッシュ容量: アセットはプログラムメモリを消費します。小容量の MCU では注意してください。
- パスの正規化: 生成されるインデックスはパスの形式（先頭スラッシュ有無や大/小文字）を一貫させるべきです。
- バイナリファイル: バイナリデータをシリアル出力する場合は `Serial.write` を使うなど注意が必要です。

## パフォーマンス

- フラッシュに格納されたデータへのアクセスは速く、SD カードの待ち時間を回避できます。
- メモリ使用: ファイル全体を RAM にコピーしない設計（ストリーム読み出し）を採用してください。

## 貢献

貢献は歓迎します。Issue または PR を作る際は、以下を含めてください:

- 使用ケースの短い説明
- 再現性のある最小コード（ボード種別、Arduino core バージョン、スケッチ、アセット）

## ライセンス

リポジトリルートの `LICENSE` を参照してください。

## トラブルシューティング

- `begin()` が失敗する: 生成ヘッダのシンボルが存在するか、インデックス形式が期待通りかを確認してください。
- ファイルが見つからない: スケッチで使っているパスが生成インデックスのパスと一致しているか（先頭スラッシュ、大文字小文字、区切り文字）を確認してください。

## まとめ

EmbedFS は静的なアセットをファームウェアにバンドルし、慣れ親しんだ FS ライクな API で
読み出せるようにする便利な仕組みです。小さなウェブアセットや設定テンプレートなど、
実行時に変更されないリソースに特に有用です。
