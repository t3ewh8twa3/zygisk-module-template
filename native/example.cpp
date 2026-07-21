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
#define CONFIG_FILE_PATH "/data/adb/modules/BY/by.txt"

class MyPropertyHookModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    // 去掉会导致报错的 override 关键字，保持纯净的函数覆写
    void preSpecialize(zygisk::AppSpecializeArgs *args) {
        if (args->nice_name == nullptr) return;

        FILE *file = fopen(CONFIG_FILE_PATH, "r");
        if (file == nullptr) return;

        if (env == nullptr) {
            fclose(file);
            return;
        }

        const char *process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process_name == nullptr) {
            fclose(file);
            return;
        }

        char target_board[128] = "";
        char target_serial[128] = "";
        bool is_target_app = false;

        char line[256];
        int line_number = 0;

        while (fgets(line, sizeof(line), file) != nullptr) {
            line[strcspn(line, "\r\n")] = 0;
            if (line[0] == '\0') continue;

            line_number++;
            if (line_number == 1) {
                strncpy(target_board, line, sizeof(target_board) - 1);
            } else if (line_number == 2) {
                strncpy(target_serial, line, sizeof(target_serial) - 1);
            } else {
                if (line[0] == '#') continue;
                if (strcmp(process_name, line) == 0) {
                    is_target_app = true;
                }
            }
        }
        fclose(file);
        env->ReleaseStringUTFChars(args->nice_name, process_name);

        if (is_target_app) {
            LOGD("==================================================");
            LOGD("【Zygisk闪电命中】目标进程已成功拦截: %s", process_name);
            LOGD("读取到配置 -> 主板ID: %s | 序列号: %s", 
                 strlen(target_board) > 0 ? target_board : "默认", 
                 strlen(target_serial) > 0 ? target_serial : "默认");
            LOGD("==================================================");
            
            is_target_hit = true;
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) {
        if (is_target_hit) {
            LOGD("【Zygisk】模块注入成功并运行中！");
        }
    }

private:
    bool is_target_hit = false;
    zygisk::Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
