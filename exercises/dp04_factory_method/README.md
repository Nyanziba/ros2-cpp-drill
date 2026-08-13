# dp04 Factory Method 〔デザインパターン編〕

結城本 第4章。生成の手順を基底クラスに固定し、**何を作るか**だけを派生クラスに任せます。
C++ の主題は 1 つです。**生成物の所有権を `std::unique_ptr` で呼び出し側に渡すこと。**

## やること

`src/logger_factory.cpp` の TODO を 5 つ埋めてください。

1. **`Logger::Logger` / `Logger::~Logger`**
   - `alive_counter_` が `nullptr` でなければ増減させる（生存数を外から観測するため）
2. **`MemoryLogger::write()`**
   - `"[タグ] メッセージ"` の 1 行を `lines_` に追加する
3. **`LoggerFactory::create()`** ← テンプレートメソッド
   - `create_logger()` → `nullptr` なら登録せず `nullptr` を返す → `register_logger()` → 返す
4. **`MemoryLoggerFactory::create_logger()`** ← ファクトリメソッド
   - `std::make_unique<MemoryLogger>(...)`。タグが空文字列なら `nullptr`
5. **`MemoryLoggerFactory::register_logger()`**
   - タグを `registered_tags_` に記録する

`include/drill/logger_factory.hpp` と `test/test_exercise.cpp` は編集しません。

## 動かしてみる

```bash
./drill run dp04
```

## つまずきポイント

- `create()` の最後は `return logger;` でよい。**ローカル変数なので自動でムーブされる。**
  `return std::move(logger);` と書くとムーブ最適化を邪魔します
- `create_logger()` の戻り値型は `std::unique_ptr<Logger>` です。
  `std::unique_ptr<MemoryLogger>` に変えると**共変戻り値型が効かず**コンパイルエラーになります
  （`std::make_unique<MemoryLogger>(...)` を `unique_ptr<Logger>` として返すのは通ります）
- 生成失敗は `nullptr` で表します。**例外は投げません**（マイコンで使えないため）
- `register_logger()` は `const Logger &` を受け取ります。所有権は渡っていません

## テスト

```bash
./drill run dp04
```

テストは 6 本です。うち 3 本は**所有権が呼び出し側に移ること**を見ています。

- スコープを抜けたら破棄されるか（ファクトリは所有しない）
- `std::move` で所有者が移るか
- ファクトリと生成物が同時に消えても二重解放にならないか

## 参考

- [4. Factory Method](../../docs/patterns/04_FactoryMethod.md)
- [C++編 6. スマートポインタ](../../docs/cpp/06_スマートポインタ.md)
- [cppreference: std::make_unique](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)
