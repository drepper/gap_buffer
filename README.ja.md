# ギャップバッファ実装

テキストエディタ向け特化機能を備えた高性能なC++ギャップバッファデータ構造実装

## 概要

ギャップバッファは、未使用領域の連続した「ギャップ」を維持する動的配列データ構造で、カーソル位置での挿入・削除を非常に効率的に行えます。この実装では、汎用的な`gap_buffer`テンプレートクラスと、テキスト編集操作に最適化された`text_editor_buffer`特化クラスの両方を提供します。

## 機能

### コアギャップバッファ (`gap_buffer<T>`)
- **STL準拠のコンテナインターフェース** と完全なイテレータサポート
- **例外安全な操作** とRAIIベースメモリ管理
- **カスタムアロケータサポート** による特殊メモリ管理
- **ムーブセマンティクス** による効率的オブジェクト転送
- **範囲チェック付きランダムアクセスイテレータ**
- **テンプレートベース設計** でコピー/ムーブ可能な任意の型をサポート

### テキストエディタバッファ (`text_editor_buffer`)
- **カーソル位置管理** と行・列トラッキング
- **キャッシュされた行インデックス** による効率的な行操作
- **検索・置換機能** でリテラルと正規表現パターンの両方をサポート
- **ファイルI/O操作** と自動文字エンコード検出
- **改行コード変換** (LF, CRLF, CR) と自動検出
- **拡張UTF-8検証** と適切なUnicodeサポート
- **単語ベースカーソル移動** によるテキスト編集ワークフロー

## パフォーマンス特性

| 操作 | 計算量 | 備考 |
|------|--------|------|
| カーソル位置への挿入 | O(1) 償却 | ギャップがカーソル位置にある |
| 他の位置への挿入 | O(n) 最悪ケース | ギャップ移動が必要 |
| カーソル位置の削除 | O(1) | ギャップを拡張 |
| ランダムアクセス | O(1) | 直接配列インデックス |
| イテレータ走査 | 要素あたりO(1) | ギャップを自動的にスキップ |

## 使用例

### 基本的なギャップバッファ

```cpp
#include "gap_buffer.hpp"

// 作成と初期化
gap_buffer<char> buffer;
buffer.push_back('H');
buffer.push_back('e');
buffer.push_back('l');
buffer.push_back('o');

// 先頭への挿入
buffer.insert(buffer.begin(), 'W');  // "WHello"

// ランダムアクセス
char c = buffer[2];  // 'e'

// STLアルゴリズムの使用
std::sort(buffer.begin(), buffer.end());
```

### テキストエディタ操作

```cpp
#include "gap_buffer.hpp"

// ファイル読み込み
text_editor_buffer editor;
editor.load_from_file("document.txt");

// 行・列による移動
editor.set_cursor_line_column(10, 5);  // 10行目、5列目
auto pos = editor.get_cursor_line_column();

// テキスト操作
editor.insert_text("Hello World!\n");
editor.delete_text(0, 5);  // 最初の5文字を削除

// 検索・置換
auto result = editor.find_text("TODO");
if (result.found) {
    editor.replace_text(result.position, result.length, "DONE");
}

// 正規表現操作
editor.replace_all_regex(R"(\b\d{4}\b)", "YEAR");

// 変更保存
editor.save_to_file("document.txt");
```

### 高度な機能

```cpp
// カスタムアロケータ
gap_buffer<int, std::allocator<int>> custom_buffer;

// 行操作
size_t line_count = editor.get_line_count();
std::string line5 = editor.get_line(5);
size_t line_length = editor.get_line_length(5);

// UTF-8検証
if (editor.is_valid_utf8()) {
    std::cout << "有効なUTF-8エンコーディング" << std::endl;
}

// 改行コード検出・変換
auto ending_type = editor.detect_line_ending();
editor.convert_line_endings(text_editor_buffer::line_ending_type::CRLF);

// パフォーマンス統計
auto stats = editor.get_stats();
std::cout << "ギャップ比率: " << stats.gap_ratio << std::endl;
```

## ビルド方法

### 要件
- C++17対応コンパイラ (GCC 7+, Clang 6+, MSVC 2017+)
- `<regex>`サポート付き標準ライブラリ

### コンパイル
```bash
# 基本コンパイル
g++ -std=c++17 -O3 your_program.cpp -o your_program

# デバッグ情報付き
g++ -std=c++17 -g -DDEBUG your_program.cpp -o your_program_debug

# ベンチマーク実行
g++ -std=c++17 -O3 benchmark.cpp -o benchmark
./benchmark
```

## ベンチマーク

`std::vector`とのパフォーマンス比較：

| 操作 | ギャップバッファ | std::vector | 高速化 |
|------|------------------|-------------|---------|
| 先頭への挿入 | ~0.1ms | ~10.2ms | ~100倍 |
| 中間への挿入 | ~0.5ms | ~5.1ms | ~10倍 |
| ランダムアクセス | ~0.3ms | ~0.3ms | ~1倍 |
| 末尾への追加 | ~0.2ms | ~0.2ms | ~1倍 |

*一般的なハードウェアでのベンチマークスイート結果*

## アーキテクチャ

実装では**プロテクテッド継承モデル**を使用し、`text_editor_buffer`が`gap_buffer<char>`を拡張しています。主要な設計決定：

1. **ギャップ管理**: 自動ギャップ配置とサイズ調整
2. **メモリ安全性**: 例外安全性を持つRAIIベースリソース管理
3. **イテレータ設計**: 適切な境界チェックでギャップを透過的にスキップ
4. **行キャッシュ**: 無効化機能付き効率的行開始位置キャッシュ
5. **Unicodeサポート**: 適切なUTF-8検証と処理

## スレッドセーフティ

この実装は**スレッドセーフではありません**。並行アクセスには：
- 外部同期化（ミューテックス、読み書きロック）を使用
- スレッドごとに別々のインスタンスを作成
- 高性能シナリオではロックフリー代替案を検討

## テスト

```bash
# テストコンパイル（テストフレームワークが利用可能な場合）
g++ -std=c++17 -g tests.cpp -o tests
./tests

# ベンチマーク実行
./benchmark
```

## 貢献

1. リポジトリをフォーク
2. フィーチャーブランチを作成（`git checkout -b feature/amazing-feature`）
3. 変更をコミット（`git commit -m 'Add amazing feature'`）
4. ブランチにプッシュ（`git push origin feature/amazing-feature`）
5. プルリクエストを開く

### 開発ガイドライン
- 既存のコードスタイルと命名規則に従う
- 新機能にはテストを追加
- パフォーマンス重要な変更にはベンチマークを更新
- 例外安全性と適切なリソース管理を確保

## ライセンス

このプロジェクトはMITライセンスの下でライセンスされています - 詳細は[LICENSE](LICENSE)ファイルを参照してください。

## 謝辞

- 古典的なテキストエディタ実装に基づくギャップバッファアルゴリズム
- STLコンテナ設計パターン
- メモリ管理と例外安全性のためのモダンC++ベストプラクティス
