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
  : data_(other.data_), size_(other.size_)
{
  other.data_ = nullptr;
  other.size_ = 0;
}

Buffer & Buffer::operator=(Buffer&& other) noexcept
{
  if (this != &other) {
    delete[] data_;
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
  }
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
