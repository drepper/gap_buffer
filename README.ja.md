# Gap Buffer

テキストエディタで効率的なテキスト挿入と削除操作に一般的に使用されるギャップバッファのモダンC++実装です。

## 概要

ギャップバッファは、挿入と削除を最適化するためにメモリ内にギャップ（未使用のスペース）を維持する動的配列データ構造です。この実装は、標準ライブラリとの互換性を持つ`std::vector`に似たコンテナインターフェースを提供します。

主な特徴:
- カーソル位置での効率的な挿入と削除操作
- STL互換のイテレータ
- C++コンテナ要件への完全準拠
- 一般的な編集操作中の再割り当てを最小限に抑えたメモリ効率
- あらゆるデータ型に対応するテンプレートベースの実装

## 要件

- C++11以降
- 標準テンプレートライブラリ

## 使用方法

### 基本的な使用法

```cpp
#include "gap_buffer.hpp"
#include <iostream>
#include <string>

int main() {
    // 整数のギャップバッファを作成
    gap_buffer<int> buffer = {1, 2, 3, 4, 5};
    
    // ベクターのように要素にアクセス
    std::cout << "最初の要素: " << buffer.front() << std::endl;
    std::cout << "最後の要素: " << buffer.back() << std::endl;
    std::cout << "インデックス2の要素: " << buffer[2] << std::endl;
    
    // 位置に挿入
    auto it = buffer.begin() + 2;
    buffer.insert(it, 10);
    
    // すべての要素を表示
    std::cout << "挿入後: ";
    for (const auto& value : buffer) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    
    // 要素を削除
    buffer.erase(buffer.begin() + 1, buffer.begin() + 3);
    
    // 削除後を表示
    std::cout << "削除後: ";
    for (const auto& value : buffer) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

### テキストエディタの例

```cpp
#include "gap_buffer.hpp"
#include <iostream>
#include <string>

class SimpleTextEditor {
private:
    gap_buffer<char> text;
    size_t cursor_position = 0;
    
public:
    // カーソル位置にテキストを挿入
    void insertText(const std::string& str) {
        auto it = text.begin() + cursor_position;
        text.insert(it, str.begin(), str.end());
        cursor_position += str.length();
    }
    
    // カーソル前のn文字を削除
    void backspace(size_t n = 1) {
        if (cursor_position >= n) {
            auto start = text.begin() + (cursor_position - n);
            auto end = text.begin() + cursor_position;
            text.erase(start, end);
            cursor_position -= n;
        }
    }
    
    // カーソル後のn文字を削除
    void deleteForward(size_t n = 1) {
        if (cursor_position < text.size()) {
            auto start = text.begin() + cursor_position;
            auto end = text.begin() + std::min(cursor_position + n, text.size());
            text.erase(start, end);
        }
    }
    
    // カーソルを移動
    void moveCursor(int offset) {
        int new_pos = static_cast<int>(cursor_position) + offset;
        if (new_pos >= 0 && new_pos <= static_cast<int>(text.size())) {
            cursor_position = static_cast<size_t>(new_pos);
        }
    }
    
    // テキスト内容を取得
    std::string getContent() const {
        return std::string(text.begin(), text.end());
    }
    
    // カーソル付きでテキストを表示
    void display() const {
        std::string content = getContent();
        content.insert(cursor_position, "|");
        std::cout << content << std::endl;
    }
};

int main() {
    SimpleTextEditor editor;
    
    editor.insertText("Hello world");
    editor.display();  // Hello world|
    
    editor.moveCursor(-5);
    editor.display();  // Hello |world
    
    editor.insertText("beautiful ");
    editor.display();  // Hello beautiful |world
    
    editor.backspace(2);
    editor.display();  // Hello beautifu|world
    
    editor.deleteForward(3);
    editor.display();  // Hello beautifu|ld
    
    return 0;
}
```

## API リファレンス

### コンストラクタと代入

```cpp
// デフォルトコンストラクタ
gap_buffer();

// アロケータコンストラクタ
explicit gap_buffer(const Allocator& alloc);

// フィルコンストラクタ
gap_buffer(size_type count, const T& value, const Allocator& alloc = Allocator());

// カウントコンストラクタ
explicit gap_buffer(size_type count, const Allocator& alloc = Allocator());

// 範囲コンストラクタ
template <typename InputIt>
gap_buffer(InputIt first, InputIt last, const Allocator& alloc = Allocator());

// コピーコンストラクタ
gap_buffer(const gap_buffer& other);

// ムーブコンストラクタ
gap_buffer(gap_buffer&& other) noexcept;

// 初期化子リストコンストラクタ
gap_buffer(std::initializer_list<T> init, const Allocator& alloc = Allocator());

// コピー代入
gap_buffer& operator=(const gap_buffer& other);

// ムーブ代入
gap_buffer& operator=(gap_buffer&& other) noexcept;

// 初期化子リスト代入
gap_buffer& operator=(std::initializer_list<T> ilist);

// コンテンツの代入
void assign(size_type count, const T& value);
template <typename InputIt>
void assign(InputIt first, InputIt last);
void assign(std::initializer_list<T> ilist);
```

### 要素アクセス

```cpp
// 境界チェック付きアクセス
reference at(size_type pos);
const_reference at(size_type pos) const;

// 境界チェックなしアクセス
reference operator[](size_type pos);
const_reference operator[](size_type pos) const;

// 最初の要素にアクセス
reference front();
const_reference front() const;

// 最後の要素にアクセス
reference back();
const_reference back() const;

// 基礎となるデータにアクセス
T* data() noexcept;
const T* data() const noexcept;
```

### イテレータ

```cpp
iterator begin() noexcept;
const_iterator begin() const noexcept;
const_iterator cbegin() const noexcept;

iterator end() noexcept;
const_iterator end() const noexcept;
const_iterator cend() const noexcept;

reverse_iterator rbegin() noexcept;
const_reverse_iterator rbegin() const noexcept;
const_reverse_iterator crbegin() const noexcept;

reverse_iterator rend() noexcept;
const_reverse_iterator rend() const noexcept;
const_reverse_iterator crend() const noexcept;
```

### 容量

```cpp
[[nodiscard]] bool empty() const noexcept;
size_type size() const noexcept;
size_type max_size() const noexcept;
void reserve(size_type new_cap);
size_type capacity() const noexcept;
void shrink_to_fit();
```

### 修飾子

```cpp
// コンテンツをクリア
void clear() noexcept;

// 要素を挿入
iterator insert(const_iterator pos, const T& value);
iterator insert(const_iterator pos, T&& value);
iterator insert(const_iterator pos, size_type count, const T& value);
template <typename InputIt>
iterator insert(const_iterator pos, InputIt first, InputIt last);
iterator insert(const_iterator pos, std::initializer_list<T> ilist);

// その場で要素を構築
template <typename... Args>
iterator emplace(const_iterator pos, Args&&... args);

// 要素を削除
iterator erase(const_iterator pos);
iterator erase(const_iterator first, const_iterator last);

// 末尾に要素を追加
void push_back(const T& value);
void push_back(T&& value);
template <typename... Args>
reference emplace_back(Args&&... args);

// 最後の要素を削除
void pop_back();

// サイズを変更
void resize(size_type count);
void resize(size_type count, const value_type& value);

// コンテンツを交換
void swap(gap_buffer& other) noexcept;
```

### 非メンバー関数

```cpp
// 比較
template <typename T, typename Alloc>
bool operator==(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator!=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator<(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator>(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator<=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);
template <typename T, typename Alloc>
bool operator>=(const gap_buffer<T, Alloc>& lhs, const gap_buffer<T, Alloc>& rhs);

// 特殊化されたswap
template <typename T, typename Alloc>
void swap(gap_buffer<T, Alloc>& lhs, gap_buffer<T, Alloc>& rhs) noexcept;
```

## パフォーマンス

ギャップバッファは以下のパフォーマンス特性を提供します：

- カーソル位置での挿入/削除: 償却O(1)
- カーソルから離れた位置での挿入/削除: O(n)（nはカーソルからの距離）
- ランダムアクセス: O(1)
- カーソル移動: O(n)（nは現在位置からの距離）

これにより、ギャップバッファは特に編集のほとんどがカーソル位置の近くで行われるテキスト編集操作に適しています。

## 実装の詳細

ギャップバッファは以下の内部構造を維持します：
- 連続したメモリバッファ
- ギャップ開始位置
- ギャップ終了位置

挿入や削除が発生すると、まずギャップが操作位置に移動し、その後ギャップの端で操作が実行されるため、メモリ移動が最小限に抑えられます。

## ライセンス

[MITライセンス](LICENSE)

## 貢献

貢献は歓迎します！ぜひプルリクエストを提出してください。
