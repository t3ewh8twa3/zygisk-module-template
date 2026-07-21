#include <jni.h>
#include <sys/system_properties.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include "zygisk.hpp"

#define TAG "MyZygiskModule"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

// 1. 定义原函数指针
static int (*orig___system_property_get)(const char *name, char *value) = nullptr;

// 2. 我们的自定义替身函数（在这里把主板ID和序列号替换掉）
static int my___system_property_get(const char *name, char *value) {
    if (name != nullptr) {
        // 序列号
        if (strcmp(name, "ro.serialno") == 0 || strcmp(name, "ro.boot.serialno") == 0) {
            strcpy(value, "bb17f688");
            return (int)strlen(value);
        }
        // 主板ID
        if (strcmp(name, "ro.product.board") == 0 || strcmp(name, "ro.board.platform") == 0) {
            strcpy(value, "466925547");
            return (int)strlen(value);
        }
    }
    return orig___system_property_get(name, value);
}

class MyPropertyHookModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        // 在这里填入你的目标 App 包名
        const char *target_pkg_name = "com.your.target.package"; 

        bool is_target_app = false;
        if (args->nice_name != nullptr && target_pkg_name != nullptr) {
            if (strcmp(args->nice_name, target_pkg_name) == 0) {
                is_target_app = true;
            }
        }

        // 仅在目标应用进程中生效
        if (is_target_app) {
            LOGD("命中目标包名: %s，开始注入硬件伪装...", args->nice_name);

            // 使用 Zygisk 自带的 plt_hook 拦截 libc 中的 __system_property_get
            zygisk::plt_hook_commit(
                zygisk::plt_hook_by_name("libc.so", "__system_property_get", 
                (void *)my___system_property_get, (void **)&orig___system_property_get)
            );
        }
    }

private:
    zygisk::Api *api;
    JNIEnv *env;
};

// 注册模块
REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
