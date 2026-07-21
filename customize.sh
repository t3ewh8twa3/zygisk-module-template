#!/bin/sh
MODPATH=${0%/*}

# 自动创建 BY 目录并释放默认的 by.txt 配置文件
CONFIG_DIR="/data/adb/modules/BY"
CONFIG_FILE="$CONFIG_DIR/by.txt"

if [ ! -f "$CONFIG_FILE" ]; then
    mkdir -p "$CONFIG_DIR"
    cat << 'EOF' > "$CONFIG_FILE"
466925547
bb17f688
# 在下面写你的目标包名，一行一个
com.example.app1
com.example.app2
EOF
    ui_print "- 已自动创建默认配置文件: $CONFIG_FILE"
fi
