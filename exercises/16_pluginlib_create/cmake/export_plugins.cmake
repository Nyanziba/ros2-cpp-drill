# I AM NOT DONE
# このファイルは CMakeLists.txt から include される（${PROJECT_NAME} はそのまま使える）。
#
# TODO: plugins.xml を pluginlib に登録すること。
#       pluginlib_export_plugin_description_file(<基底クラスがあるパッケージ> <XML のパス>)
#       基底クラス drill::VelocityFilter はこのパッケージの中にあるので、
#       第 1 引数は自分のパッケージ名（${PROJECT_NAME}）。
#       これが ament のリソースインデックスに plugins.xml の場所を書き込み、
#       ClassLoader が「このパッケージの、この基底クラスのプラグイン」を探せるようになる。
