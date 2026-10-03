// ROS 2 練習帳 テストの文言を日英で切り替えるヘルパ
//
// このファイルは編集しません。各課題のテストから include されます。
//
// DRILL_LANG=en のときだけ英語の文言を返します（./drill の localized() と同じ規則）。
// テスト名（TEST の第 2 引数）は識別子なので切り替えられません。英語の識別子で書きます。
#pragma once

#include <cctype>
#include <cstdlib>
#include <string>

namespace drill
{

/// DRILL_LANG が "en" で始まるか（大文字小文字は区別しない）。
inline bool is_english()
{
  const char * language = std::getenv("DRILL_LANG");
  if (language == nullptr) {
    return false;
  }
  const std::string prefix(language, 0, 2);
  return prefix.size() == 2 &&
         std::tolower(static_cast<unsigned char>(prefix[0])) == 'e' &&
         std::tolower(static_cast<unsigned char>(prefix[1])) == 'n';
}

/// 失敗メッセージを言語に合わせて選ぶ。
inline const char * localized(const char * japanese, const char * english)
{
  return is_english() ? english : japanese;
}

}  // namespace drill
