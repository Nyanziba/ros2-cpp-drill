// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>
#include <cstring>

#include "drill/string_ops.h"

TEST(StringOpsTest, ComputesStringLength)
{
  EXPECT_EQ(string_length("hello"), 5);
  EXPECT_EQ(string_length(""), 0);
  EXPECT_EQ(string_length("a"), 1);
  EXPECT_EQ(string_length("Hello, World!"), 13);
}

TEST(StringOpsTest, LengthReturnsMinusOneOnNull)
{
  EXPECT_EQ(string_length(NULL), -1);
}

TEST(StringOpsTest, CopiesString)
{
  char dest[10] = {};
  EXPECT_EQ(string_copy(dest, "hello", 10), 0);
  EXPECT_STREQ(dest, "hello");

  // バッファが十分なら全文字列がコピーされる
  char dest2[5] = {};
  EXPECT_EQ(string_copy(dest2, "test", 5), 0);
  EXPECT_STREQ(dest2, "test");
}

TEST(StringOpsTest, CopyTruncatesToBufferSize)
{
  char dest[4] = {};  // "abc\0" が入る
  EXPECT_EQ(string_copy(dest, "hello", 4), 0);
  EXPECT_STREQ(dest, "hel");  // 最後の 1 バイトは NUL
  EXPECT_EQ(dest[3], '\0');
}

TEST(StringOpsTest, CopyHandlesEmptyString)
{
  char dest[10] = {};
  EXPECT_EQ(string_copy(dest, "", 10), 0);
  EXPECT_STREQ(dest, "");
  EXPECT_EQ(dest[0], '\0');
}

TEST(StringOpsTest, CopyReturnsMinusOneOnInvalidArguments)
{
  char dest[10] = {};
  EXPECT_EQ(string_copy(NULL, "hello", 10), -1);
  EXPECT_EQ(string_copy(dest, NULL, 10), -1);
  EXPECT_EQ(string_copy(dest, "hello", 0), -1);
}

TEST(StringOpsTest, ConcatenatesStrings)
{
  char dest[20] = "hello";
  EXPECT_EQ(string_concat(dest, " world", 20), 0);
  EXPECT_STREQ(dest, "hello world");

  // バッファが十分なら連結
  char dest2[30] = "foo";
  EXPECT_EQ(string_concat(dest2, "bar", 30), 0);
  EXPECT_STREQ(dest2, "foobar");
}

TEST(StringOpsTest, ConcatTruncatesToBufferSize)
{
  char dest[10] = "hi";
  EXPECT_EQ(string_concat(dest, "bye", 10), 0);
  EXPECT_STREQ(dest, "hibye");
  EXPECT_EQ(dest[5], '\0');

  // バッファが足りない場合は切り詰め
  char dest2[8] = "aa";  // "aa" = 2 バイト。残り 5 バイト（NUL 含む）
  EXPECT_EQ(string_concat(dest2, "12345", 8), 0);
  EXPECT_STREQ(dest2, "aa1234");  // "aa1234\0" = 7 バイト
  EXPECT_EQ(dest2[7], '\0');
}

TEST(StringOpsTest, ConcatAppendsEmptyString)
{
  char dest[20] = "hello";
  EXPECT_EQ(string_concat(dest, "", 20), 0);
  EXPECT_STREQ(dest, "hello");
}

TEST(StringOpsTest, ConcatReturnsMinusOneOnInvalidArguments)
{
  char dest[20] = "hello";
  EXPECT_EQ(string_concat(NULL, "world", 20), -1);
  EXPECT_EQ(string_concat(dest, NULL, 20), -1);
  EXPECT_EQ(string_concat(dest, "world", 0), -1);
}
