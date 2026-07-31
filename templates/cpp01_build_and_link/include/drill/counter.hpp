// このファイルも編集します（C++編 c01 だけの例外です）。
//
// 直すところは 2 つあり、1 つはこのヘッダの中にあります。
#pragma once

// TODO(1): このままだとリンクエラーで落ちます。
//
//   multiple definition of `add_one(int)'
//
// 理由: このヘッダは 2 つの .cpp（src/counter.cpp と test/test_exercise.cpp）から
//       include されています。#pragma once が保証するのは「同じ翻訳単位に
//       2 回貼り付けない」ことだけなので、.cpp ごとに add_one の定義が
//       1 個ずつでき、リンカが「定義が複数ある」と怒ります。
//
// 直し方は 2 通りあります。どちらでも合格します。
//   (a) この定義に inline を付ける（1 単語の修正）
//   (b) 中身を src/counter.cpp に移し、ここには宣言だけ残す
//
// 講習資料: docs/cpp/01_ビルドとリンクの仕組み.md の 1.4 節
int add_one(int x)
{
  return x + 1;
}

/// 呼び出すたびに 1, 2, 3, ... と増えていく値を返す。
///
/// ここには宣言だけがあり、定義は src/counter.cpp にあります。
/// これが「宣言と定義を分ける」の基本形です。
int next_id();
