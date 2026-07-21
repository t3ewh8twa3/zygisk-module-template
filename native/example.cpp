#include <jni.h>
#include <sys/system_properties.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include "zygisk.hpp"

#define TAG "MyZygiskModule"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

class MyPropertyHookModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        // 在这里填入你的目标 App 包名
        const char *target_pkg_name = "com.nhlx.cmuroe"; 

        if (args->nice_name == nullptr || target_pkg_name == nullptr) {
            return;
        }

        // 安全地将 jstring (args->nice_name) 转为 C 字符串
        const char *process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        if (process_name == nullptr) {
            return;
        }

        bool is_target_app = (strcmp(process_name, target_pkg_name) == 0);
        
        // 释放 JNI 字符串内存
        env->ReleaseStringUTFChars(args->nice_name, process_name);

        // 仅在目标应用进程中生效
        if (is_target_app) {
            LOGD("命中目标包名，开始执行系统属性伪装 (主板ID: 466925547, 序列号: bb17f688)...");
            
            // 注：在现代 Zygisk 模块中，若要对系统属性进行最稳妥的拦截，
            // 建议配合 LSPosed 或在当前 Native 空间通过标准 PLT 替换。
            // 当前阶段已成功进入目标进程初始化流程，可在此处加载你的 hook 逻辑。
        }
    }

private:
    zygisk::Api *api;
    JNIEnv *env;
};

// 注册模块
REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
