// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace drill
{

// ---------------------------------------------------------------------------
// 1. Virtual Proxy — 重い較正テーブルの遅延ロード
// ---------------------------------------------------------------------------
//
// 結城本の Printer / PrinterProxy に対応します。
// 本の PrinterProxy は「名前を付けるだけなら実体は要らない。印刷するときに初めて作る」
// でした。ここでは「較正テーブルは EEPROM から読むので重い。
// 使われないまま終わることもある」という設定です。
//
// C++ 版で本と決定的に違うのは、Proxy が Printer を継承しないことです。
// 継承の代わりに operator-> を書きます。**それが C++ の Proxy です。**

/// 重い本体。生成された回数を数えているのは、テストが
/// 「最初のアクセスまで作られないこと」「二度目に作り直されないこと」を見るためです。
class CalibrationTable
{
public:
  static constexpr std::size_t kEntryCount = 8;

  /// 生成のたびに load_count() が 1 増えます（＝ EEPROM を読んだ回数）。
  explicit CalibrationTable(std::string source);

  /// index が範囲外なら 0.0 を返します（例外は投げません）。
  double entry(std::size_t index) const;

  const std::string & source() const { return source_; }

  static std::size_t load_count();
  static void reset_load_count();

private:
  std::string source_;
  double entries_[kEntryCount] = {};
};

/// CalibrationTable の Virtual Proxy。
///
/// 【この課題の中心】
///   const CalibrationTable * operator->() const;
/// を実装すると、
///   proxy->entry(3)
/// と書けるようになります。コンパイラが proxy.operator->() を呼び、
/// 返ってきたポインタに対して ->entry(3) を続けます。
///
/// 【なぜ real_ が mutable なのか】
/// operator-> は const メンバ関数です（const な Proxy からも使いたいので）。
/// その中で本体を作る＝メンバを書き換えるので、real_ には mutable が要ります。
/// 「論理的には const だが、物理的には書き換える」典型例です。
///
/// コピーを禁止しているのは、Proxy をコピーすると本体まで
/// 二重に持つのか共有するのかが曖昧になるためです。
class CalibrationProxy
{
public:
  explicit CalibrationProxy(std::string source);

  CalibrationProxy(const CalibrationProxy &) = delete;
  CalibrationProxy & operator=(const CalibrationProxy &) = delete;

  /// 本体へのポインタを返す。まだ無ければここで作る（遅延生成）。
  /// **課題: これを実装します。**
  const CalibrationTable * operator->() const;

  /// operator* は operator-> の上に乗せてあります（実装済み）。
  const CalibrationTable & operator*() const { return *operator->(); }

  /// 本体が既に作られているか。テストが遅延を確認するために使います。
  bool is_loaded() const { return real_ != nullptr; }

  /// 本体を作らずに答えられること。Proxy の存在理由そのものです。
  const std::string & source() const { return source_; }

private:
  std::string source_;
  mutable std::unique_ptr<CalibrationTable> real_;
};

// ---------------------------------------------------------------------------
// 2. Protection Proxy — レジスタへのアクセスを記録し、範囲を検査する
// ---------------------------------------------------------------------------

/// 「ハードウェアのレジスタ」に見立てた本体（実装済み）。
///
/// read_raw / write_raw は範囲検査をしません。実機なら範囲外の書き込みは
/// 別のペリフェラルを壊します。だから直接触らせず、Proxy 越しに使わせます。
///
/// hardware_access_count_ が mutable なのは、read_raw が const なのに
/// 回数を数えたいからです。CalibrationProxy::real_ と同じ理由です。
class RegisterFile
{
public:
  static constexpr std::size_t kRegisterCount = 4;

  /// 範囲外なら 0 を返す（この教材では未定義動作にしないための妥協です）。
  std::uint16_t read_raw(std::size_t index) const;
  void write_raw(std::size_t index, std::uint16_t value);

  /// read_raw / write_raw が呼ばれた回数。範囲外の呼び出しも数えます。
  std::size_t hardware_access_count() const { return hardware_access_count_; }
  void reset_counts();

private:
  std::uint16_t registers_[kRegisterCount] = {};
  mutable std::size_t hardware_access_count_ = 0;
};

class SafeRegisterProxy;

/// SafeRegisterProxy::operator-> が返す**一時オブジェクト**。
///
/// 【C++ 固有の要点 — operator-> の連鎖（drill-down）】
/// operator-> は「ポインタが返るまで繰り返し呼ばれる」という規則です。
///
///   proxy->read_raw(0);
///     → SafeRegisterProxy::operator->()  … RegisterAccess（ポインタではない）
///     → RegisterAccess::operator->()     … RegisterFile *（ポインタ。ここで止まる）
///     → RegisterFile::read_raw(0)
///
/// この規則があるおかげで、**アクセスの前後に処理を挟めます**。
/// RegisterAccess はコンストラクタで "enter" を、デストラクタで "leave" を記録します。
/// 一時オブジェクトは式の終わりまで生きるので、
/// read_raw(0) の呼び出しは必ず enter と leave に挟まれます。
/// 実務では、この位置で std::mutex をロック／アンロックします（記事 21.6）。
///
/// コピーもムーブも禁止しています。それでも
///   RegisterAccess operator->();
/// が書けるのは、C++17 の保証されたコピー省略のおかげです。
class RegisterAccess
{
public:
  explicit RegisterAccess(SafeRegisterProxy & owner);
  ~RegisterAccess();

  RegisterAccess(const RegisterAccess &) = delete;
  RegisterAccess & operator=(const RegisterAccess &) = delete;

  /// ここでポインタが返るので、operator-> の連鎖が止まります。
  RegisterFile * operator->() const;

private:
  SafeRegisterProxy & owner_;
};

/// RegisterFile へのアクセスを記録し、範囲を検査する Proxy。
///
/// 【Decorator（第12章）との違い】
/// Decorator は「振る舞いを足す」。Proxy は「同じ振る舞いに見せかけて、
/// アクセスを制御する」。read/write は RegisterFile と同じことをしているように見えて、
/// 範囲外を弾き、記録を残しています。**呼ぶ側の書き方は変わりません。**
///
/// 【寿命】
/// file_ は参照です。RegisterFile より Proxy を長生きさせてはいけません。
/// Java なら GC が生かしてくれますが、C++ では落ちます。
class SafeRegisterProxy
{
public:
  explicit SafeRegisterProxy(RegisterFile & file);

  /// 検査つきの読み出し。範囲外なら std::nullopt を返し、本体には触りません。
  /// **課題: これを実装します。**
  std::optional<std::uint16_t> read(std::size_t index);

  /// 検査つきの書き込み。範囲外なら false を返し、本体には触りません。
  /// **課題: これを実装します。**
  bool write(std::size_t index, std::uint16_t value);

  /// 検査を通さない直接アクセス（自己責任）。一時オブジェクト経由で前後が記録されます。
  /// **課題: これを実装します。**
  RegisterAccess operator->();

  /// 記録。文字列の形式は README と課題のコメントに書いてあります。
  const std::vector<std::string> & log() const { return log_; }
  /// 範囲外で弾いた回数。
  std::size_t rejected_count() const { return rejected_; }
  /// どの本体を包んでいるか（テストがアドレスを比べます）。
  const RegisterFile & file() const { return file_; }

  /// 記録を 1 行足す（実装済み）。RegisterAccess からも呼びます。
  void record(std::string entry) { log_.push_back(std::move(entry)); }

private:
  // RegisterAccess だけが、検査を通さない生の RegisterFile に到達できます。
  // Proxy の中身を外に開けずに一時オブジェクトへ渡すための friend です。
  friend class RegisterAccess;

  RegisterFile & file_;
  std::vector<std::string> log_;
  std::size_t rejected_ = 0;
};

}  // namespace drill
