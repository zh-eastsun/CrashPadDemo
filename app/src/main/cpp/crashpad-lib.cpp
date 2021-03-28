#include <jni.h>
#include <string>
#include <unistd.h>
#include <cstdio>
#include <android/log.h>
#include <pthread.h>
#include <bitset>

#include "client/crashpad_client.h"
#include "client/crash_report_database.h"
#include "client/settings.h"

#define LOG_TAG "GoogleCrashPad"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using namespace base;
using namespace crashpad;
using namespace std;


static JNIEnv *global_env;
static jobject global_native_crash_handler;

//全局变量
JavaVM *g_jvm = NULL;

void* thread_fun(void* );

bool callback() {
    ALOGD("===============NativeCrashHandler.CRASH.DumpCallback================");
    pthread_t thread;
    pthread_create(&thread, NULL, &thread_fun, nullptr);
    pthread_join(thread, NULL);
    return true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_github_zdy_crashpad_demo_MainActivity_initCrashPad(
        JNIEnv *env,
        jobject object, /* this */
        jstring path,
        jstring nativeLibDir
) {

    global_env = env;
    global_native_crash_handler = env->NewGlobalRef(object);

    string dataDir = env->GetStringUTFChars(path, 0);
    string nativeLibDirPath = env->GetStringUTFChars(nativeLibDir, 0);

    // Crashpad file paths
    FilePath handler(nativeLibDirPath + "/libcrashpad_handler.so");
    FilePath reportsDir(dataDir + "/crashpad");
    FilePath metricsDir(dataDir + "/crashpad");

    // Crashpad upload URL for BugSplat database
    string url = "http://{{database}}.bugsplat.com/post/bp/crash/crashpad.php";

    // Crashpad annotations
    map<string, string> annotations;
    annotations["format"] = "minidump";           // Required: Crashpad setting to save crash as a minidump
    annotations["database"] = "{{database}}";     // Required: BugSplat database
    annotations["product"] = "{{appName}}";       // Required: BugSplat appName
    annotations["version"] = "{{appVersion}}";    // Required: BugSplat appVersion
    annotations["key"] = "Key";                   // Optional: BugSplat key field
    annotations["user"] = "fred@bugsplat.com";    // Optional: BugSplat user email
    annotations["list_annotations"] = "Sample comment"; // Optional: BugSplat crash description

    // Crashpad arguments
    vector<string> arguments;
    arguments.push_back("--no-rate-limit");
    // Crashpad local database
    unique_ptr<CrashReportDatabase> crashReportDatabase = CrashReportDatabase::Initialize(
            reportsDir);
    if (crashReportDatabase == NULL) return false;

    // Enable automated crash uploads
    Settings *settings = crashReportDatabase->GetSettings();
    if (settings == NULL) return false;
    settings->SetUploadsEnabled(true);

    // File paths of attachments to be uploaded with the minidump file at crash time - default bundle limit is 2MB
    vector<FilePath> attachments;
    FilePath attachment(dataDir + "/files/attachment.txt");
    attachments.push_back(attachment);

    // Start Crashpad crash handler
    static CrashpadClient *client = new CrashpadClient();
    bool status = client->StartHandlerAtCrash(handler, reportsDir, metricsDir, url, annotations, arguments, attachments,callback);
    return status;
}

extern "C" JNIEXPORT void JNICALL
Java_com_github_zdy_crashpad_demo_MainActivity_testNativeCrash(JNIEnv *env,
                                                                         jobject /* this */) {
    std::bitset<10> x(std::string("987123"));
}

void *thread_fun(void* ) {
    ALOGD("===============NativeCrashHandler.thread_fun.Start================");
    //Attach主线程
    if (g_jvm->AttachCurrentThread(&global_env, NULL) != JNI_OK) {
        ALOGD("===============AttachCurrentThread.NOT.JNI_OK================");
    }
    jclass class_native_crash_handler = global_env->GetObjectClass(global_native_crash_handler);
    jmethodID methodID = global_env->GetMethodID(class_native_crash_handler, "test",
                                                 "()V");
    global_env->CallVoidMethod(global_native_crash_handler,methodID);
    //Detach主线程
    if (g_jvm->DetachCurrentThread() != JNI_OK) {
        ALOGD("===============DetachCurrentThread.NOT.JNI_OK================");
    }
    ALOGD("===============NativeCrashHandler.thread_fun.End================");
    pthread_exit(0);

}