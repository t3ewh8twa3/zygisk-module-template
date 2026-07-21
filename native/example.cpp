#include <jni.h>
#include <sys/system_properties.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include "dobby.h" // 引入 Dobby 库头文件

// 定义 LOGD 宏，方便打印日志到 logcat
#define TAG "MyZygiskModule"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
// 1. 定义原函数指针
int (*orig___system_property_get)(const char *name, char *value);

// 2. 自定义的 __system_property_get 替身函数
int my___system_property_get(const char *name, char *value) {
    if (name != nullptr) {
        // 伪装序列号 (ro.serialno / ro.boot.serialno)
        if (strcmp(name, "ro.serialno") == 0 || strcmp(name, "ro.boot.serialno") == 0) {
            strcpy(value, "bb17f688"); 
            return strlen(value);
        }
        // 伪装主板ID (ro.product.board / ro.board.platform)
        if (strcmp(name, "ro.product.board") == 0 || strcmp(name, "ro.board.platform") == 0) {
            strcpy(value, "466925547"); 
            return strlen(value);
        }
    }
    return orig___system_property_get(name, value);
}

void postAppSpecialize(const AppSpecializeArgs *args) override {
    // 手动在此处指定你的目标包名
    const char *target_pkg_name = "com.nhlx.cmuroe"; 

    // 获取当前进程的包名 (Zygisk 环境下 args->nice_name 即为当前进程包名)
    bool is_target_app = false;
    if (args->nice_name != nullptr && target_pkg_name != nullptr) {
        if (strcmp(args->nice_name, target_pkg_name) == 0) {
            is_target_app = true;
        }
    }

    // 只有当命中目标包名时，才在目标进程里执行真正的 Hook 操作
    if (is_target_app) {
        LOGD("命中目标包名，开始在目标进程执行局部 Hook...");
        
        // 执行 Dobby Hook 拦截 __system_property_get
        void *sym_get = DobbySymbolResolver(nullptr, "__system_property_get");
        if (sym_get) {
            DobbyHook(sym_get, (void *)my___system_property_get, (void **)&orig___system_property_get);
            LOGD("__system_property_get Hook 成功，已注入指定参数");
        }
    }
}
