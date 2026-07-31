# ros2-drill のタブ補完（bash）
#
# 有効にするには ~/.bashrc に次の行を足してください。
#   source /path/to/ros2-drill/completion/drill.bash
#
# 候補は ./drill 自身に問い合わせるので、課題を追加しても
# このファイルを書き換える必要はありません。
#
# 絞り込みも ./drill 側でやっています（compgen は前方一致しかできないが、
# ./drill run publisher のような部分一致も補完したいため）。
_drill_complete()
{
  local exe cur prev
  exe="${COMP_WORDS[0]}"
  cur="${COMP_WORDS[COMP_CWORD]}"
  prev="${COMP_WORDS[COMP_CWORD-1]}"

  # コマンド自体を補完する
  if [ "$COMP_CWORD" -eq 1 ]; then
    mapfile -t COMPREPLY < <("$exe" __complete commands "" "$cur" 2>/dev/null)
    return 0
  fi

  # コマンドの引数を補完する
  mapfile -t COMPREPLY < <("$exe" __complete args "$prev" "$cur" 2>/dev/null)
  return 0
}
complete -F _drill_complete drill ./drill
