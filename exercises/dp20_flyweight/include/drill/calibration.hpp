// このファイルは編集しません（インタフェースの提示）。
#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace drill
{

// ---------------------------------------------------------------------------
// 1. マイコン向けの Flyweight（実装済み。読むだけでよい）
// ---------------------------------------------------------------------------
//
// 「型番ごとに決まっている較正係数」は、実行時に 1 個も作る必要がありません。
// constexpr にして ROM に置けば、RAM も確保もゼロで、全員が同じ実体を共有します。
// これが C++ で書ける一番軽い Flyweight です。
//
// 下の CalibrationRegistry を作るのは、「係数が実行時にしか分からない」
// （EEPROM から読む、設定ファイルから読む）ときだけです。

/// 型番ごとの較正係数。本質的状態（intrinsic）だけを持ちます。
struct CalibrationSpec
{
  std::string_view model_id;
  double gain;
  double offset;
};

/// ROM に置く較正テーブル。inline constexpr なのでヘッダに置けます。
inline constexpr CalibrationSpec kCalibrationRom[] = {
  {"MPU6050-GYRO", 0.0076294, 0.0},
  {"AS5600-ENC", 0.0878906, 0.0},
  {"ACS712-30A", 0.0666000, -2.5},
  {"NTC-10K", 0.0244140, -40.0},
};

/// ROM から型番を引く。見つからなければ nullptr。
/// constexpr なので、型番がコンパイル時に分かっていれば実行時コストはゼロです。
constexpr const CalibrationSpec * find_spec(std::string_view model_id)
{
  for (const CalibrationSpec & spec : kCalibrationRom) {
    if (spec.model_id == model_id) {
      return &spec;
    }
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// 2. 実行時に共有する Flyweight（ここから課題）
// ---------------------------------------------------------------------------

/// 共有される較正テーブル。**本質的状態しか持ちません。**
///
/// Java 版（結城本の BigChar）との違い:
///   - 共有するものは const にします。誰かが書き換えたら全員に波及するためです。
///     だから CalibrationRegistry は shared_ptr<const CalibrationTable> を返します。
///   - コピーを禁止しています。Flyweight をコピーしたら共有した意味がありません。
///
/// 生成回数・破棄回数を数えているのは、課題のテストが
/// 「引いた回数ではなく種類の数だけ作られたか」を見るためです。
class CalibrationTable
{
public:
  CalibrationTable(std::string model_id, double gain, double offset);
  ~CalibrationTable();

  CalibrationTable(const CalibrationTable &) = delete;
  CalibrationTable & operator=(const CalibrationTable &) = delete;

  const std::string & model_id() const { return model_id_; }
  double gain() const { return gain_; }
  double offset() const { return offset_; }

  /// これまでに何個作られたか。
  static std::size_t construction_count();
  /// これまでに何個壊されたか。
  static std::size_t destruction_count();
  /// カウンタを 0 に戻す（テスト用）。
  static void reset_counts();

private:
  std::string model_id_;
  double gain_;
  double offset_;
};

/// Flyweight のプール。結城本の BigCharFactory に対応します。
///
/// 【Java 版との一番大きい違い】
/// Java の HashMap<String, BigChar> は「プールが持ち続ける」ので、
/// 使われなくなった Flyweight も GC されません（結城本もそう書いています）。
/// C++ で std::map<std::string, std::shared_ptr<T>> にすると、まったく同じ問題が起きます。
/// **プロセスが終わるまで解放されません。**
///
/// そこでプールは std::weak_ptr で持ちます。
///   - 誰かが使っている間は weak_ptr から shared_ptr が取れる → 共有される
///   - 全員が手放したら CalibrationTable は破棄される → weak_ptr は expired になる
///   - expired になった残骸（map のエントリ自体）は自動では消えないので、
///     自分で掃除する必要があります。それが sweep_expired() です。
///
/// 【スレッド安全性】
/// このクラスはスレッド安全ではありません。shared_ptr の参照カウントはアトミックですが、
/// std::map への挿入・削除は保護されていません。複数スレッドから引くなら
/// std::mutex で get() と sweep_expired() を丸ごと囲んでください（記事 20.6 参照）。
class CalibrationRegistry
{
public:
  /// 利用者が受け取るハンドル。const が付いているのが要点です。
  using Handle = std::shared_ptr<const CalibrationTable>;

  /// 型番に対応する較正テーブルを得る。
  ///   - 生きているものがプールにあれば、それを返す（同じアドレスが返る）
  ///   - 無ければ ROM の CalibrationSpec から新しく作り、プールに登録して返す
  ///   - ROM に無い型番なら nullptr を返す（例外は投げません）
  Handle get(const std::string & model_id);

  /// プールが持っているエントリの数。expired な残骸も数に入ります。
  std::size_t pool_size() const;

  /// expired になった残骸を消す。消した個数を返す。
  std::size_t sweep_expired();

private:
  std::map<std::string, std::weak_ptr<const CalibrationTable>> pool_;
};

/// 較正テーブルを使う側。**付帯的状態（extrinsic）はこちらが持ちます。**
///
/// 個体ごとのゼロ点補正 zero_offset_ は Flyweight に入れてはいけません。
/// 入れた瞬間、同じ型番のセンサ同士で共有できなくなります。
class Sensor
{
public:
  Sensor(std::string name, CalibrationRegistry::Handle table, double zero_offset);

  /// 生の AD 値を物理量に変換する。
  ///   raw * gain + offset + zero_offset
  /// gain と offset は共有（intrinsic）、zero_offset は個体ごと（extrinsic）です。
  double convert(int raw) const;

  const std::string & name() const { return name_; }
  double zero_offset() const { return zero_offset_; }

  /// どの Flyweight を指しているか。テストがアドレスを比べます。
  const CalibrationTable * table() const { return table_.get(); }

private:
  std::string name_;
  CalibrationRegistry::Handle table_;
  double zero_offset_;
};

}  // namespace drill
