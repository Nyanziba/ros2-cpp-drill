# このファイルは CMakeLists.txt から include される（${PROJECT_NAME} はそのまま使える）。
#
# plugins.xml を pluginlib に登録する。基底クラスはこのパッケージの中にあるので、
# 第 1 引数は自分のパッケージ名。
pluginlib_export_plugin_description_file(${PROJECT_NAME} plugins.xml)
