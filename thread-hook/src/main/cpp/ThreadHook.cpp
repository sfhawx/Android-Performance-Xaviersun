#include <jni.h>
#include <string>
#include <android/log.h>
#include "bytehook.h"
#include "ThreadStackShink.h"
#include <pthread.h>

#define LOG_TAG            "thread_hook"
#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)

JavaVM *javaVm = NULL;

static struct StacktraceJNI {
    jclass callbackClazz;
    jmethodID method_onPthreadCreated;
    jmethodID method_onPthreadReleased;
    jmethodID method_onPthreadIdGet;
} gJ;

int pthread_create_proxy(pthread_t *_pthread_ptr, pthread_attr_t const *_attr,
                         void *(*_start_routine)(void *), void *args) {
    BYTEHOOK_STACK_SCOPE();

    if (_pthread_ptr == nullptr) {
        LOG("attr is null, skip adjusting.");
        return -1;
    }
    JNIEnv *env = NULL;
    if(JNI_OK != (*javaVm).AttachCurrentThread(&env,NULL)){
        return NULL;
    }
    jstring j_str = (jstring)env->CallStaticObjectMethod(gJ.callbackClazz, gJ.method_onPthreadCreated, *_pthread_ptr);
    int result = BYTEHOOK_CALL_PREV(pthread_create_proxy, _pthread_ptr, _attr, _start_routine,
                                    args);

    env->CallStaticVoidMethod(gJ.callbackClazz, gJ.method_onPthreadIdGet, *_pthread_ptr);

    const char *str = env->GetStringUTFChars(j_str, NULL);

    if(std::strlen(str) > 0) {
        LOG("hook thread created, id=%ld, stack=%s", *_pthread_ptr, str);
    }
    return result;
}

void
thread_hooked_callback(bytehook_stub_t task_stub, int status_code, const char *caller_path_name,
                       const char *sym_name, void *new_func, void *prev_func, void *arg) {
    if (BYTEHOOK_STATUS_CODE_ORIG_ADDR == status_code) {
        LOG(">>>>> save original address: %u", (uintptr_t) prev_func);
    } else {
        LOG(">>>>> hooked. stub: %u status: %d, caller_path_name: %s, sym_name: %s, new_func: %u, prev_func: %u"", arg: %u",
            (uintptr_t) task_stub, status_code, caller_path_name, sym_name, (uintptr_t) new_func,
            (uintptr_t) prev_func, (uintptr_t) arg);
    }
}

int pthread_join_proxy(pthread_t _pthread, void** _return_value_ptr) {
    BYTEHOOK_STACK_SCOPE();

    LOG("hook thread join, id=%ld", _pthread);
    JNIEnv *env = NULL;
    if(JNI_OK != (*javaVm).AttachCurrentThread(&env,NULL)){
        return NULL;
    }
    env->CallStaticVoidMethod(gJ.callbackClazz, gJ.method_onPthreadReleased, _pthread);
    int result = BYTEHOOK_CALL_PREV(pthread_join_proxy, _pthread, _return_value_ptr);
    LOG("hook thread join end, id=%ld", _pthread);
    return result;
}

int pthread_detach_proxy(pthread_t _pthread) {
    BYTEHOOK_STACK_SCOPE();

    LOG("hook thread detach success, id=%ld", _pthread);

    JNIEnv *env = NULL;
    if(JNI_OK != (*javaVm).AttachCurrentThread(&env,NULL)){
        return NULL;
    }
    env->CallStaticVoidMethod(gJ.callbackClazz, gJ.method_onPthreadReleased, _pthread);
    int result = BYTEHOOK_CALL_PREV(pthread_detach_proxy, _pthread);
    LOG("hook thread detach end., id=%ld", _pthread);
    return result;
}

int pthread_exit_proxy(void *value_ptr) {
    BYTEHOOK_STACK_SCOPE();

    pthread_t thread_id = pthread_self();
    LOG("hook thread exit success, id=%ld", thread_id);

    JNIEnv *env = NULL;
    if(JNI_OK != (*javaVm).AttachCurrentThread(&env,NULL)){
        return NULL;
    }
    env->CallStaticVoidMethod(gJ.callbackClazz, gJ.method_onPthreadReleased, thread_id);
    int result = BYTEHOOK_CALL_PREV(pthread_exit_proxy, value_ptr);

    LOG("hook thread exit end, id=%ld", thread_id);


    return result;
}


void hookThread() {
    bytehook_hook_all(nullptr, "pthread_create", (void *) pthread_create_proxy,
                      thread_hooked_callback,
                      nullptr);

    bytehook_hook_all(nullptr, "pthread_join", (void *) pthread_join_proxy,
                      thread_hooked_callback,
                      nullptr);

    bytehook_hook_all(nullptr, "pthread_detach", (void *) pthread_detach_proxy,
                      thread_hooked_callback,
                      nullptr);

    bytehook_hook_all(nullptr, "pthread_exit", (void *) pthread_exit_proxy,
                      thread_hooked_callback,
                      nullptr);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
        return -1;

    // com.zj.android.thread.hook

    jclass callbackCls = env->FindClass("com/zj/android/thread/hook/ThreadMonitor");
    if (!callbackCls)
        return -1;
    gJ.callbackClazz = static_cast<jclass>(env->NewGlobalRef(callbackCls));

    gJ.method_onPthreadCreated = env->GetStaticMethodID(callbackCls, "onPthreadCreated", "(J)Ljava/lang/String;");
    gJ.method_onPthreadReleased = env->GetStaticMethodID(callbackCls, "onPthreadReleased", "(J)V");
    gJ.method_onPthreadIdGet = env->GetStaticMethodID(callbackCls, "onPthreadIdGet", "(J)V");

    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_zj_android_thread_hook_ThreadHookNativeLib_hookThread(
        JNIEnv *env,
        jobject /* this */) {
    hookThread();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_zj_android_thread_hook_ThreadHookNativeLib_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}