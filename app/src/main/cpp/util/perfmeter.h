/**
 * Created by Andrea Fiorito on 27/01/2021.
 * Copyright (c) 2020, BLOOM ENGINEERING LTD. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
**/

#ifndef A6CLOUDXR_PERFMETER_H
#define A6CLOUDXR_PERFMETER_H

#include <chrono>
#include <android/log.h>

#ifndef LOGI
#define LOGI(...) \
  __android_log_print(ANDROID_LOG_INFO, "hello_ar_example_c", __VA_ARGS__)
#endif  // LOGI

struct TimeInterval{
private:
    void setEndTime() {
        endtime = std::chrono::steady_clock::now();
        duration = endtime - starttime;
    }
public:
    enum State {
        NOT_SET,
        RUNNING,
        END
    } state = NOT_SET;
    int count = { 0 };
    long interval = { 1000000L };
    std::chrono::duration<double, std::micro> duration;
    std::chrono::time_point<std::chrono::steady_clock> starttime;
    std::chrono::time_point<std::chrono::steady_clock> endtime;
    void hit() {
        switch(state) {
            case NOT_SET:
                starttime = std::chrono::steady_clock::now();
                setEndTime();
                state = RUNNING;
                break;
            case RUNNING:
                count++;
                setEndTime();
                if (duration.count()>interval) state = END;
                break;
            case END:
                break;
        }
    }
    double getFPS() { return count*interval/duration.count(); }
    void clear() {
        state = NOT_SET;
        count = 0;
    }
    void moveToNextInterval() {
        state = RUNNING;
        count = 0;
        starttime = endtime;
    }
};

class PerfMeter
{
private:
    std::string name;
TimeInterval interval;
public:
    PerfMeter(std::string name): name(name) { }
    void hit() {
        if (interval.state == TimeInterval::State::END) {
            LOGI("TGF Performance Meter [%s] - framerate is %f", name.c_str(), interval.getFPS());
            interval.moveToNextInterval();
        }
        interval.hit();
    }
};

#endif //A6CLOUDXR_PERFMETER_H
