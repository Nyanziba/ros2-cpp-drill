// このファイルは編集しません（インタフェースの提示）。
#pragma once

/// 動的メモリを管理するクラス。
/// ムーブセマンティクスを学びます。
class Buffer
{
public:
  explicit Buffer(int size);
  ~Buffer();

  // コピーは禁止
  Buffer(const Buffer &) = delete;
  Buffer & operator=(const Buffer &) = delete;

  // ムーブコンストラクタとムーブ代入を TODO で実装
  Buffer(Buffer&& other) noexcept;
  Buffer & operator=(Buffer&& other) noexcept;

  int size() const;
  bool empty() const;
  int* data();

private:
  int* data_;
  int size_;
};
