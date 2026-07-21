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
            LOGD("==================================================");
            is_target_hit = true;
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) {
        if (is_target_hit && env != nullptr) {
            // 在 App 进程初始化完成后，通过 JNI 安全在屏幕弹出一个短暂的 Toast 提示
            showToast(env, "【Zygisk】注入成功，参数已生效！");
        }
    }

private:
    bool is_target_hit = false;
    zygisk::Api *api;
    JNIEnv *env;

    void showToast(JNIEnv *env, const char *message) {
        if (env == nullptr) return;

        // 通过反射调用 Android 的 Toast.makeText 并 show
        jclass activity_thread_cls = env->FindClass("android/app/ActivityThread");
        if (activity_thread_cls == nullptr) return;

        jmethodID current_activity_thread = env->GetStaticMethodID(activity_thread_cls, "currentActivityThread", "()Landroid/app/ActivityThread;");
        if (current_activity_thread == nullptr) return;

        jobject at = env->CallStaticObjectMethod(activity_thread_cls, current_activity_thread);
        if (at == nullptr) return;

        jmethodID get_application = env->GetMethodID(activity_thread_cls, "getApplication", "()Landroid/app/Application;");
        if (get_application == nullptr) return;

        jobject context = env->CallObjectMethod(at, get_application);
        if (context == nullptr) return;

        jclass toast_cls = env->FindClass("android/widget/Toast");
        if (toast_cls == nullptr) return;

        jmethodID make_text = env->GetStaticMethodID(toast_cls, "makeText", "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
        if (make_text == nullptr) return;

        jstring msg_str = env->NewStringUTF(message);
        // LENGTH_SHORT = 0
        jobject toast_obj = env->CallStaticObjectMethod(toast_cls, make_text, context, msg_str, 0);
        if (toast_obj == nullptr) return;

        jmethodID show_method = env->GetMethodID(toast_cls, "show", "()V");
        if (show_method != nullptr) {
            env->CallVoidMethod(toast_obj, show_method);
            LOGD("屏幕 Toast 提示已成功发出！");
        }

        env->DeleteLocalRef(msg_str);
    }
};

REGISTER_ZYGISK_MODULE(MyPropertyHookModule)
