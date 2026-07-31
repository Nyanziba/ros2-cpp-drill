// このファイルは編集しません（インタフェースの提示）。
#pragma once

/// ストップウォッチクラス。
/// 経過時間を管理します。const メンバを含むため、メンバ初期化リストでの初期化が必須です。
class Stopwatch
{
public:
  /// コンストラクタ: max_time に最大時間（ミリ秒）を受け取ります。
  /// メンバ初期化リストを使って max_time_ と elapsed_ を初期化してください。
  explicit Stopwatch(int max_time_ms);

  /// 経過時間を増やします。
  void advance(int ms);

  /// 経過時間を返します（const メンバ関数）。
  int elapsed() const;

  /// 最大時間を返します（const メンバ関数）。
  int max_time() const;

private:
  const int max_time_;  // const メンバ変数（一度初期化したら変更できない）
  int elapsed_;
};
