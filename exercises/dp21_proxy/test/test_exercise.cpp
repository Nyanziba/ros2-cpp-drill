// このファイルは編集しません（採点用）。
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "drill/calibration_proxy.hpp"

namespace
{

using drill::CalibrationProxy;
using drill::CalibrationTable;
using drill::RegisterAccess;
using drill::RegisterFile;
using drill::SafeRegisterProxy;

/// ログに指定の文字列が何回出てくるか。
std::size_t count_in_log(const std::vector<std::string> & log, const std::string & entry)
{
  std::size_t count = 0;
  for (const std::string & line : log) {
    if (line == entry) {
      ++count;
    }
  }
  return count;
}

}  // namespace

// ---------------------------------------------------------------------------
// Virtual Proxy
// ---------------------------------------------------------------------------

TEST(ProxyTest, operator矢印で本体のメソッドが呼べる)
{
  CalibrationTable::reset_load_count();
  const CalibrationProxy proxy{"eeprom"};

  ASSERT_NE(proxy.operator->(), nullptr)
    << "operator-> が nullptr を返しています。まだ実装されていません";

  // "eeprom" は 6 文字。entries_[i] = 6 + 0.25 * i。
  EXPECT_DOUBLE_EQ(proxy->entry(0), 6.0);
  EXPECT_DOUBLE_EQ(proxy->entry(3), 6.75);
  EXPECT_EQ(proxy->source(), std::string{"eeprom"});

  // 範囲外は本体側が 0.0 を返す。
  EXPECT_DOUBLE_EQ(proxy->entry(CalibrationTable::kEntryCount), 0.0);
}

TEST(ProxyTest, 最初のアクセスまで本体は作られない)
{
  CalibrationTable::reset_load_count();
  CalibrationProxy proxy{"eeprom"};

  // Proxy を作っただけでは本体はまだ無い。
  EXPECT_FALSE(proxy.is_loaded()) << "コンストラクタで本体を作ってしまっています";
  EXPECT_EQ(CalibrationTable::load_count(), 0u);

  // source() は本体を作らずに答えられる。これが Proxy の存在理由。
  EXPECT_EQ(proxy.source(), std::string{"eeprom"});
  EXPECT_EQ(CalibrationTable::load_count(), 0u)
    << "source() は Proxy が自分で答えられます。本体を作ってはいけません";

  // 最初のアクセスで初めて作られる。
  ASSERT_NE(proxy.operator->(), nullptr);
  EXPECT_TRUE(proxy.is_loaded());
  EXPECT_EQ(CalibrationTable::load_count(), 1u)
    << "operator-> で本体が生成されていません";
}

TEST(ProxyTest, 二度目以降のアクセスで本体は作り直されない)
{
  CalibrationTable::reset_load_count();
  CalibrationProxy proxy{"eeprom"};

  const CalibrationTable * first = proxy.operator->();
  ASSERT_NE(first, nullptr);

  const CalibrationTable * second = proxy.operator->();
  const CalibrationTable * third = &(*proxy);

  EXPECT_EQ(first, second) << "アクセスのたびに作り直しています。real_ を使い回してください";
  EXPECT_EQ(first, third) << "operator* と operator-> が別の実体を指しています";
  EXPECT_EQ(CalibrationTable::load_count(), 1u)
    << "生成回数が 1 ではありません。毎回 make_unique していませんか";
}

TEST(ProxyTest, constなProxyからも遅延生成できる)
{
  CalibrationTable::reset_load_count();
  const CalibrationProxy proxy{"flash"};

  EXPECT_FALSE(proxy.is_loaded());
  EXPECT_EQ(CalibrationTable::load_count(), 0u);

  // const な Proxy に対して operator-> を呼ぶ。
  // real_ が mutable でなければ、そもそもコンパイルが通りません。
  ASSERT_NE(proxy.operator->(), nullptr);
  EXPECT_TRUE(proxy.is_loaded()) << "const な Proxy から遅延生成できていません";
  EXPECT_EQ(CalibrationTable::load_count(), 1u);

  // "flash" は 5 文字。
  EXPECT_DOUBLE_EQ((*proxy).entry(2), 5.5);
}

TEST(ProxyTest, operator矢印の戻り値はconstポインタである)
{
  // Proxy が返すのは本体そのものではなくポインタ。
  // ここでポインタが返るので、operator-> の連鎖は 1 段で止まります。
  static_assert(
    std::is_same<decltype(std::declval<const CalibrationProxy &>().operator->()),
                 const CalibrationTable *>::value,
    "CalibrationProxy::operator-> は const CalibrationTable * を返します");

  // Proxy はコピーできない（本体を二重に持つのか共有するのかが曖昧になるため）。
  static_assert(
    !std::is_copy_constructible<CalibrationProxy>::value,
    "CalibrationProxy はコピー禁止です");

  // 型が合っていても、中身が nullptr では Proxy になりません。
  CalibrationTable::reset_load_count();
  const CalibrationProxy proxy{"eeprom"};
  EXPECT_NE(proxy.operator->(), nullptr)
    << "operator-> が nullptr を返しています";
}

// ---------------------------------------------------------------------------
// Protection Proxy（記録と範囲検査）
// ---------------------------------------------------------------------------

TEST(ProxyTest, Proxy経由の読み書きは記録される)
{
  RegisterFile file;
  SafeRegisterProxy proxy{file};

  EXPECT_TRUE(proxy.write(1, 0x00ff));
  EXPECT_TRUE(proxy.write(3, 7));

  const std::optional<std::uint16_t> value = proxy.read(1);
  ASSERT_TRUE(value.has_value()) << "範囲内の読み出しが nullopt を返しています";
  EXPECT_EQ(*value, 0x00ff);

  const std::vector<std::string> & log = proxy.log();
  EXPECT_EQ(count_in_log(log, "write:1=255"), 1u)
    << "書き込みの記録がありません。形式は write:<index>=<value> です";
  EXPECT_EQ(count_in_log(log, "write:3=7"), 1u);
  EXPECT_EQ(count_in_log(log, "read:1"), 1u)
    << "読み出しの記録がありません。形式は read:<index> です";

  EXPECT_EQ(proxy.rejected_count(), 0u);
  EXPECT_EQ(&proxy.file(), &file);
}

TEST(ProxyTest, 範囲外のアクセスは弾かれ本体に届かない)
{
  RegisterFile file;
  SafeRegisterProxy proxy{file};

  ASSERT_TRUE(proxy.write(0, 1));
  file.reset_counts();

  EXPECT_FALSE(proxy.write(RegisterFile::kRegisterCount, 99))
    << "範囲外の書き込みが通っています";
  EXPECT_FALSE(proxy.read(99).has_value()) << "範囲外の読み出しが通っています";

  EXPECT_EQ(file.hardware_access_count(), 0u)
    << "弾いたのに本体（RegisterFile）に触っています。実機ならここで別の回路が壊れます";
  EXPECT_EQ(proxy.rejected_count(), 2u);

  const std::vector<std::string> & log = proxy.log();
  EXPECT_EQ(count_in_log(log, "reject:write:4"), 1u)
    << "形式は reject:write:<index> です";
  EXPECT_EQ(count_in_log(log, "reject:read:99"), 1u)
    << "形式は reject:read:<index> です";

  // 弾かれても、既に書いた値は壊れていない。
  const std::optional<std::uint16_t> value = proxy.read(0);
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, 1);
}

// ---------------------------------------------------------------------------
// operator-> の連鎖（drill-down）と一時オブジェクト
// ---------------------------------------------------------------------------

TEST(ProxyTest, operator矢印は一時オブジェクトを経由して連鎖する)
{
  // SafeRegisterProxy::operator-> はポインタではなく RegisterAccess を返す。
  // ポインタが返るまで operator-> が繰り返し呼ばれる、という規則で
  // RegisterAccess::operator-> がもう一度呼ばれ、そこで RegisterFile * が返る。
  static_assert(
    std::is_same<decltype(std::declval<SafeRegisterProxy &>().operator->()),
                 RegisterAccess>::value,
    "SafeRegisterProxy::operator-> は RegisterAccess を値で返します");
  static_assert(
    std::is_same<decltype(std::declval<const RegisterAccess &>().operator->()),
                 RegisterFile *>::value,
    "RegisterAccess::operator-> は RegisterFile * を返します");

  // コピーもムーブもできないのに値で返せるのは C++17 の保証されたコピー省略のため。
  static_assert(
    !std::is_copy_constructible<RegisterAccess>::value, "RegisterAccess はコピー禁止です");
  static_assert(
    !std::is_move_constructible<RegisterAccess>::value, "RegisterAccess はムーブ禁止です");

  RegisterFile file;
  SafeRegisterProxy proxy{file};

  {
    const RegisterAccess access{proxy};
    ASSERT_NE(access.operator->(), nullptr)
      << "RegisterAccess::operator-> が nullptr を返しています。まだ実装されていません";
  }

  proxy->write_raw(2, 0x1234);
  EXPECT_EQ(proxy->read_raw(2), 0x1234)
    << "operator-> の連鎖で RegisterFile まで届いていません";
}

TEST(ProxyTest, 一時オブジェクトの寿命が本体アクセスを挟む)
{
  RegisterFile file;
  {
    SafeRegisterProxy probe{file};
    const RegisterAccess access{probe};
    ASSERT_NE(access.operator->(), nullptr)
      << "RegisterAccess::operator-> が nullptr を返しています。まだ実装されていません";
  }

  SafeRegisterProxy proxy{file};
  proxy->write_raw(0, 5);

  const std::vector<std::string> & log = proxy.log();
  ASSERT_EQ(log.size(), 2u)
    << "1 回のアクセスにつき enter と leave が 1 行ずつ出るはずです";
  EXPECT_EQ(log[0], std::string{"enter"})
    << "RegisterAccess のコンストラクタで enter を記録してください";
  EXPECT_EQ(log[1], std::string{"leave"})
    << "RegisterAccess のデストラクタで leave を記録してください";

  // 2 回アクセスすれば 2 組。一時オブジェクトは式ごとに作られて壊れる。
  proxy->read_raw(0);
  proxy->read_raw(0);
  EXPECT_EQ(count_in_log(proxy.log(), "enter"), 3u);
  EXPECT_EQ(count_in_log(proxy.log(), "leave"), 3u);

  // 検査つきの read/write は一時オブジェクトを通らないので enter/leave は増えない。
  EXPECT_TRUE(proxy.write(1, 9));
  EXPECT_EQ(count_in_log(proxy.log(), "enter"), 3u)
    << "read/write が operator-> を経由してしまっています";
}
