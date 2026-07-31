# ros2-drill のタブ補完（zsh）
#
# 有効にするには ~/.zshrc に次の行を足してください。
#   source /path/to/ros2-drill/completion/drill.zsh
_drill_complete()
{
  local exe="${words[1]}"
  local cur="${words[CURRENT]}"
  if (( CURRENT == 2 )); then
    compadd -- ${(f)"$("$exe" __complete commands '' "$cur" 2>/dev/null)"}
  else
    compadd -- ${(f)"$("$exe" __complete args "${words[CURRENT-1]}" "$cur" 2>/dev/null)"}
  fi
}
compdef _drill_complete drill ./drill
