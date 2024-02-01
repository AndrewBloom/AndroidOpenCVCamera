#ifndef CAMERA_ENGINE_H
#define CAMERA_ENGINE_H

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <media/NdkImageReader.h>
#include <thread>
#include <unistd.h>

#include "ImagePipe.h"
/**
 * ImageProcessor encapsulates 2 elements:
 * - the pipe used to pass a processed frame to the OpenGL thread
 * - the function that processes the frame (through the callback processFunc).
 * parameters for the callback are stored as members and are the width and the height of the
 * image, which is stored on the buffer, qnd the output pipe used to send the image further.
 */
class ImageProcessor
{
public:
    ImageProcessor(ImagePipe &p) : pipe(p) {};
    int w;
    int h;
    uint8_t *buffer;
    ImagePipe &pipe;
    void (*processFunc)(int w, int h, uint8_t *buffer, ImagePipe &pipe);
    void exec() { (*processFunc)(w, h, buffer, pipe); };
};

/**
 * Class the wraps AImage and deletes it using RAII. Movable only, to guarantee proper
 * release of the resource only once.
 */
class AImageWrapper
{
public:
    AImage *image = nullptr;
    int64_t  timestamp = 0;
    AImageWrapper(){};
    // TODO TripleBuffer class doesn't support move
    AImageWrapper(AImageWrapper&& src) : image(src.image), timestamp(src.timestamp) {
        src.image = nullptr;
        src.timestamp = 0;
    };
    AImageWrapper &  operator= (AImageWrapper && src) {
        release();
        image = src.image;
        timestamp = src.timestamp;
        src.image = nullptr;
        src.timestamp = 0;
        return *this;
    };
    bool acquire (AImageReader *reader) {
        if (image != nullptr) AImage_delete(image);
        auto res = AImageReader_acquireLatestImage(reader, &image);
        if (res != AMEDIA_OK) return false;
        AImage_getTimestamp(image, &timestamp);
        return true;
    }
    ~AImageWrapper(){
        if (image!= nullptr) AImage_delete(image);
    }
    void release() { if (image!= nullptr) AImage_delete(image); image = nullptr; }
};

/**
 * A worker thread that uses a lockless Triple buffer as queue. Images are extracted from the
 * reader and wrote on the pipe, this is usually triggered by the available image callback of the
 * reader itself. Once started, the thread loops and extracts images from the triple buffer and
 * processes them through the assignable callback  cb.
 */
class WorkerThread
{
private:
    std::thread _thread;
    void (*cb)(AImage* img, ImageProcessor* imgProc) = nullptr;
    TripleBuffer<AImageWrapper> imgTBuf;
    std::atomic_bool stopping = false;
public:
    WorkerThread() :imgTBuf(NO_INIT) { }
    ~WorkerThread() { stopping = true; _thread.join(); }
    void start(void(*callback)(AImage*, ImageProcessor *), ImageProcessor *imgProc) {
        cb = callback;
        _thread = std::thread([this, imgProc]() {
            while (true) {
                if (!imgTBuf.IsDirty()) {
                    usleep(1000); // TODO try some values
                    continue;
                }
                if (stopping) return;
                AImageWrapper imgWrp = (AImageWrapper &&) std::move(imgTBuf.SwapAndRead());
                AImage* img = imgWrp.image;
                (*cb)(img, imgProc);
            }
        });
    }

    void pushImage(AImageReader *reader) {
        AImageWrapper wrapper;
        if (!wrapper.acquire(reader)) return;
        imgTBuf.WriteAndSwap(std::move(wrapper));
    }
};

// defined here so it can be both a static and friend function.
static void imageCallback(void* context, AImageReader* reader);

/**
 * The Camera Engine
 */
class CameraEngine {
private:
    ACameraManager *cameraManager = nullptr;
    ACameraDevice *cameraDevice = nullptr;
    ACaptureRequest *request = nullptr;
    ACameraCaptureSession *textureSession = nullptr;
    ANativeWindow *imageWindow = nullptr;
    ACameraOutputTarget *imageTarget = nullptr;
    AImageReader *imageReader = nullptr;
    ACaptureSessionOutput *imageOutput = nullptr;
    ACaptureSessionOutput *output = nullptr;
    ACaptureSessionOutputContainer *outputs = nullptr;

    int width = 640;
    int height = 480;

    WorkerThread wThread;

    AImageReader *createJpegReader(ImageProcessor &imgProc);
    friend void imageCallback(void* context, AImageReader* reader);
public:

    void exitCam();
    void initCamSession(ImageProcessor &imgProc);
    void setSize(int w, int h);
    void initCam();
};

#endif // CAMERA_ENGINE_H
