# CrashPad编译教程

## CrashPad vs BreakPad

1. CrashPad修复了Android 10+设备无法捕获信号为SIGABRT的崩溃问题。
2. 更改架构，CrashPad使用C/S架构重构代码。

## 环境准备

要开发Crashpad，必须使用以下工具，并且这些工具必须存在于$ PATH环境变量中：

* 开发工具
  * 在macOS上，安装Xcode。通常建议使用最新版本。
  * 在Windows上，安装具有C ++支持的Visual Studio和Windows SDK。通常建议使用最新版本。某些测试还需要CDB调试器，该调试器与Windows调试工具一起安装。
  * 在Linux上，可以通过任何适当的方式（包括系统的程序包管理器）获得用于C ++开发的适当工具。在基于Debian和Debian的发行版中，build-essential和zlib1g-dev软件包就足够了。
* Chromium的depot_tools。
* GIT，这是由macOS上的Xcode，Windows上的depot_tools以及通过任何适当的方式（包括Linux上的系统的程序包管理器）提供的。
* Python，这是由macOS操作系统，Windows上的depot_tools以及包括Linux上系统的软件包管理器在内的任何适当方式提供的。
* Android NDK，版本21.x。

Google depot_tools仓库地址：[chromium/tools/depot_tools.git - Git at Google (googlesource.com)](https://chromium.googlesource.com/chromium/tools/depot_tools.git)



## 编译步骤

### 初始化仓库

```bash
$ mkdir ~/crashpad
$ cd ~/crashpad
$ fetch crashpad
```

<font color='red'>注意：fetch是depot_tools中的工具，需要将depot_tools添加到环境变量中，不能直接用路径引用fetch，因为该工具会调用其他工具（调用链），所以必须配置到环境变量中。</font>



### 同步仓库

```bash
$ cd ~/crashpad/crashpad
$ git pull -r
$ git checkout -f 7547d0aa874f550fb3d3e8a5365ec51da96cbd86
$ gclient sync
```

<font color='red'>这里需要强制切换节点，Google仓库的HEAD节点代码无法正常编译，所以需要切换到该节点；gclient也是depot_tools中的工具。</font>



### 构建

```bash
$ cd ~/crashpad/crashpad
python build/gyp_crashpad_android.py \
  --ndk ~/android-ndk-r21b --arch arm64 --api-level 21 \
  --generator-output out/android_arm64_api21 \
```

```bash
$ ninja -C out/android_arm64_api21/out/Debug all
```

> --ndk 指定本地NDK的路径，我使用的是NDK 21.x
>
> --arch 指定编译的目标平台，支持的平台可以参考gyp_crashpad_android.py代码
>
> --api-level 指定Android API等级
>
> --generator-output 指定输出路径



## 引入Android工程中

### 添加头文件引用

将CrashPad工程中所有.h文件到 `app/src/main/cpp/crashpad/include`路径下，为了方便筛选所有.h文件，可以复制一份CrashPad工程到任意目录下，然后在CrashPad副本工程中执行如下脚本删除除了.h文件外的其他文件。

```bash
$ find ./ -type f | grep -v '.h$' | xargs rm -f
```

完成上述操作后需要在CMakeLists.txt中配置对头文件的引用。

```cmake
# Crashpad Headers
include_directories(${PROJECT_SOURCE_DIR}/crashpad/include/ ${PROJECT_SOURCE_DIR}/crashpad/include/third_party/mini_chromium/mini_chromium/)
```



### 添加目标文件

从`/crashpad/out/Debug/{{ABI}}`文件夹下复制`client`, `util` 和 `third_party/mini_chromium/mini_chromium/base`到`app/src/main/cpp/crashpad/lib/{{ABI}}`下。

然后在CMakeLists.txt中配置链接关系。

```cmake
# Crashpad Libraries
add_library(crashpad_client STATIC IMPORTED)
set_property(TARGET crashpad_client PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/client/libcrashpad_client.a)

add_library(crashpad_util STATIC IMPORTED)
set_property(TARGET crashpad_util PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/util/libcrashpad_util.a)

add_library(base STATIC IMPORTED)
set_property(TARGET base PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/base/libbase.a)

target_link_libraries(  # Specifies the target library
    native-lib
    crashpad_client
    crashpad_util
    base
)
```



### 添加核心动态库

在我们构建CrashPad这一步骤里，生成了名为crashpad_handler的可执行文件，为了让Android识别并加入打包流程，我们需要将这个可执行文件改名为libcrashpad_handler.so，然后复制到Android工程的`app/src/main/cpp/crashpad/lib/{{ABI}}`路径下，每一个架构都需要该库，所以当支持多架构设备时需要将这个库复制到所有ABI对应的文件夹下。

然后在CMakeLists.txt中添加对该库的引用。

```cmake
# Crashpad Handler
add_library(crashpad_handler SHARED IMPORTED)
set_property(TARGET crashpad_handler PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/libcrashpad_handler.so)
```



### 完整的CMakeLists.txt

```cmake
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        crashpad-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        crashpad-lib.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Crashpad Headers
include_directories(${PROJECT_SOURCE_DIR}/crashpad/include/ ${PROJECT_SOURCE_DIR}/crashpad/include/third_party/mini_chromium/mini_chromium/)

# Crashpad Libraries
add_library(crashpad_client STATIC IMPORTED)
set_property(TARGET crashpad_client PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/client/libcrashpad_client.a)

add_library(crashpad_util STATIC IMPORTED)
set_property(TARGET crashpad_util PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/util/libcrashpad_util.a)

add_library(base STATIC IMPORTED)
set_property(TARGET base PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/base/libbase.a)


# Crashpad Handler
add_library(crashpad_handler SHARED IMPORTED)
set_property(TARGET crashpad_handler PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/crashpad/lib/${ANDROID_ABI}/libcrashpad_handler.so)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        crashpad-lib
        crashpad_client
        crashpad_util
        base
        # Links the target library to the log library
        # included in the NDK.
        ${log-crashpad}
        ${log-lib})
```

<font color='red'>注意：以上配置仅供参考，需要根据自己的项目结构进行路径配置。</font>



## 应用CrashPad

### 初始化

```c++
extern "C" JNIEXPORT jboolean JNICALL
Java_com_github_zdy_crashpad_demo_MainActivity_initCrashPad(
        JNIEnv *env,
        jobject object, /* this */
        jstring path,
        jstring nativeLibDir
) {
    env->GetJavaVM(&g_jvm);
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
```

> 初始化参数
>
> jstring path：CrashPad崩溃信息存储路径
>
> jstring nativeLibDir：CrashPad加载动态库的路径，需要在该路径下加载libcrashpad_handler.so



### 崩溃测试用例

```c++
extern "C" JNIEXPORT void JNICALL
Java_com_github_zdy_crashpad_demo_MainActivity_testNativeCrash(JNIEnv *env,
                                                                         jobject /* this */) {
    std::bitset<10> x(std::string("987123"));
}
```

在Java代码中通过JNI引用测试用例代码触发崩溃，然后在初始化相关目录下查看是否有崩溃文件(.dmp)产生。



### 解析dmp文件

参考BreakPad本地解析dmp文件的方式，CrashPad生成的dmp文件仍然遵从minidump文件标准并且兼容BreakPad的解析工具，CrashPad编译并不会像BreakPad一样生成解析dmp文件的可执行文件，所以需要使用BreakPad的解析工具来进行dmp文件的解析



## 参考链接

[Crashpad - Developing Crashpad (googlesource.com)](https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/doc/developing.md) - CrashPad官方文档

[Android NDK Crash Reporting - Setup Guide - BugSplat Docs](https://www.bugsplat.com/docs/sdk/android-ndk/) - BugSplat对CrashPad的应用，有详细教程

