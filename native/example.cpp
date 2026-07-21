#include <jni.h>
#include <sys/system_properties.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include <unistd.h>
#include "zygisk.hpp"

#define TAG "ZygiskDebug"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

// 固定的配置文件路径
#define CONFIG_FILE_PATH "/data/adb/modules/BY/by.txt"

// 属性 Hook 的底层替换函数（利用 __system_property_override 动态劫持系统属性）
// 注意：如果原模板中已有自己的属性修改实现，可将下方的具体hook逻辑嵌入其中
void overrideProp(const char* name, const char* value) {
    // 兼容不同安卓版本的属性修改接口
    if (__system_property_override(name, value, strlen(value)) < 0) {
        LOGD("修改属性失败: %s", name);
    } else {
        LOGD("成功伪装属性 -> %s = %s", name, value);
    }
}

class MyPropertyHookModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (args->nice_name == nullptr) {
            return;
        }

        const char *process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process_name == nullptr) {
            return;
        }

        char target_board[128] = "";
        char target_serial[128] = "";
        bool is_target_app = false;

        // 打开配置文件读取参数
        FILE *file = fopen(CONFIG_FILE_PATH, "r");
        if (file != nullptr) {
            char line[256];
            int line_number = 0;

            while (fgets(line, sizeof(line), file) != nullptr) {
                // 去除换行符
                line[strcspn(line, "\r\n")] = 0;
                
                // 跳过空行
                if (line[0] == '\0') {
                    continue;
                }

                line_number++;

                if (line_number == 1) {
                    // 第一行：主板ID
                    strncpy(target_board, line, sizeof(target_board) - 1);
                } else if (line_number == 2) {
                    // 第二行：序列号
                    strncpy(target_serial, line, sizeof(target_serial) - 1);
                } else {
                    // 第三行及以后：目标包名
                    if (line[0] == '#') {
                        continue; // 跳过注释行
                    }
                    if (strcmp(process_name, line) == 0) {
                        is_target_app = true;
                        // 注意：这里先不 break，因为我们要把整个文件读完以拿到前两行的配置参数
                    }
                }
            }
            fclose(file);
        } else {
            LOGD("未能打开配置文件: %s", CONFIG_FILE_PATH);
        }

        env->ReleaseStringUTFChars(args->nice_name, process_name);

        // 如果命中了目标包名，则执行硬件属性篡改
        if (is_target_app) {
            LOGD("==================================================");
            LOGD("【Zygisk动态匹配成功】命中目标进程: %s", process_name);
            
            // 应用从 by.txt 读取到的主板ID和序列号
            if (strlen(target_board) > 0) {
                // 常见主板ID属性（根据你的需要可以增减，例如 ro.board.id 等）
                overrideProp("ro.board.id", target_board);
                overrideProp("ro.product.board", target_board);
            }
            if (strlen(target_serial) > 0) {
                // 常见序列号属性
                overrideProp("ro.serialno", target_serial);
                overrideProp("gsm.serialno", target_serial);
            }
            
            LOGD("==================================================");
        }
    }

private:
    zygisk::Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
