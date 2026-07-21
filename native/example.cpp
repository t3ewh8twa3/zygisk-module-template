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
                    strncpy(target_board, line, sizeof(target_board) - 1);
                } else if (line_number == 2) {
                    strncpy(target_serial, line, sizeof(target_serial) - 1);
                } else {
                    if (line[0] == '#') {
                        continue; // 跳过注释行
                    }
                    if (strcmp(process_name, line) == 0) {
                        is_target_app = true;
                    }
                }
            }
            fclose(file);
        }

        env->ReleaseStringUTFChars(args->nice_name, process_name);

        // 如果命中了目标包名，则输出提示日志
        if (is_target_app) {
            LOGD("==================================================");
            LOGD("【Zygisk动态匹配成功】命中目标进程: %s", process_name);
            LOGD("读取配置 -> 主板ID: %s, 序列号: %s", 
                 strlen(target_board) > 0 ? target_board : "未设置", 
                 strlen(target_serial) > 0 ? target_serial : "未设置");
            LOGD("==================================================");
            
            // 此处可安全扩展你的 Hook 逻辑
        }
    }

private:
    zygisk::Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
