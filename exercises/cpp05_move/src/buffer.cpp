// I AM NOT DONE
//
// ムーブセマンティクスを実装してください。

#include "drill/buffer.hpp"

Buffer::Buffer(int size)
  : data_(new int[size]), size_(size)
{
}

Buffer::~Buffer()
{
  delete[] data_;
}

Buffer::Buffer(Buffer&& other) noexcept
{
  // TODO: other からデータをムーブしてきます。
  // other のポインタは nullptr に、size は 0 に。
}

Buffer & Buffer::operator=(Buffer&& other) noexcept
{
  // TODO: この->data_ の古いメモリを解放してから、other からムーブしてきます。
  // 自己代入チェックも考えましょう。
  return *this;
}

int Buffer::size() const
{
  return size_;
}

bool Buffer::empty() const
{
  return size_ == 0;
}

int* Buffer::data()
{
  return data_;
}
