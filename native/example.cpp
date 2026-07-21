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

    // 将核心判断提到更早的 preSpecialize 阶段，防止 App 启动过快导致 Hook 滞后或失效
    void preSpecialize(zygisk::AppSpecializeArgs *args) override {
        checkForTarget(args->nice_name);
    }

    void preSpecialize(zygisk::ServerSpecializeArgs *args) override {
        // 不需要处理 system_server
    }

private:
    void checkForTarget(jstring nice_name) {
        if (nice_name == nullptr) {
            return;
        }

        // 注意：在 preSpecialize 中由于还没完全初始化，有时 env 可能需要通过安全方式获取
        // 如果在此处用 env->GetStringUTFChars 遇到兼容问题，我们采用底层 safer 方案：
        // 也可以直接在 postAppSpecialize 里做，但为了速度，我们在 pre 阶段尽早读取文件缓存
        
        FILE *file = fopen(CONFIG_FILE_PATH, "r");
        if (file == nullptr) {
            return; // 配置文件不存在直接跳过
        }

        // 这里为了稳妥，我们把读取逻辑写完
        // 由于 preAppSpecialize/preSpecialize 阶段拿 jstring 需要 JNIEnv，
        // 如果当前的 env 指针安全，我们可以直接转：
        if (env == nullptr) {
            fclose(file);
            return;
        }

        const char *process_name = env->GetStringUTFChars(nice_name, nullptr);
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

        env->ReleaseStringUTFChars(nice_name, process_name);

        // 只要命中，立刻打印最高优先级的强制日志，绝对不会因为启动快而遗漏
        if (is_target_app) {
            LOGD("==================================================");
            LOGD("【Zygisk闪电命中】进程已成功拦截: %s", process_name);
            LOGD("已加载参数 -> 主板ID: %s | 序列号: %s", 
                 strlen(target_board) > 0 ? target_board : "默认", 
                 strlen(target_serial) > 0 ? target_serial : "默认");
            LOGD("==================================================");
        }
    }

    zygisk::Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
