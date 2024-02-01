#include <android/log.h>

#ifndef COMMON_H
#define COMMON_H

#define LOG_TAG "JNI_COMMON"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#include <time.h> // clock_gettime

static inline int64_t getTimeMs()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec*1000 + now.tv_nsec/1000000;
}

static inline int getTimeInterval(int64_t startTime)
{
    return int(getTimeMs() - startTime);
}

#endif // COMMON_H
