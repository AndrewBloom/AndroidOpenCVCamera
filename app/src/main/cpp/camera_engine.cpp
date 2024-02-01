//
// Created by Andrea Fiorito on 31/01/2024.
//
#include <string>
#include <fstream>

#include <camera/NdkCameraMetadata.h>
#include <android/native_window_jni.h>
#include <assert.h>

#include "cam_utils.h"
#include "common.hpp"
#include "ImagePipe.h"
#include "camera_engine.h"


using namespace sixo;

/**
 * Device listeners
 */

static void onDisconnected(void* context, ACameraDevice* device)
{
    LOGD("onDisconnected");
}

static void onError(void* context, ACameraDevice* device, int error)
{
    LOGD("error %d", error);
}

static ACameraDevice_stateCallbacks cameraDeviceCallbacks = {
        .context = nullptr,
        .onDisconnected = onDisconnected,
        .onError = onError,
};


/**
 * Session state callbacks
 */

static void onSessionActive(void* context, ACameraCaptureSession *session)
{
    LOGD("onSessionActive()");
}

static void onSessionReady(void* context, ACameraCaptureSession *session)
{
    LOGD("onSessionReady()");
}

static void onSessionClosed(void* context, ACameraCaptureSession *session)
{
    LOGD("onSessionClosed()");
}

static ACameraCaptureSession_stateCallbacks sessionStateCallbacks {
        .context = nullptr,
        .onActive = onSessionActive,
        .onReady = onSessionReady,
        .onClosed = onSessionClosed
};

static void imageCallback(void* context, AImageReader* reader)
{
    CameraEngine *cameraEngine = reinterpret_cast<CameraEngine*>(context);
    cameraEngine->wThread.pushImage(reader);
}

static void processImage(AImage *image, ImageProcessor *imgProc)
{
    uint8_t *data = nullptr;

    // TODO data could be allocated only once, passing it as arg to convert function.
    data = convert_YUV_420_888_to_YUV_12(image);

    imgProc->buffer = data;
    imgProc->exec();

    delete[] data;
}

AImageReader* CameraEngine::createJpegReader(ImageProcessor &imgProc)
{
    AImageReader* reader = nullptr;
    media_status_t status = AImageReader_new(imgProc.w, imgProc.h, AIMAGE_FORMAT_YUV_420_888,
                                             4, &reader);
    if (status != AMEDIA_OK) {
        LOGE("status is not AMEDIA OK, %d", status);
        assert(status == AMEDIA_OK);
    }

    wThread.start(processImage, &imgProc);

    AImageReader_ImageListener listener{
            .context = (void *) this,
            .onImageAvailable = imageCallback,
    };

    AImageReader_setImageListener(reader, &listener);

    return reader;
}

ANativeWindow* createSurface(AImageReader* reader)
{
    ANativeWindow *nativeWindow;
    AImageReader_getWindow(reader, &nativeWindow);

    return nativeWindow;
}


/**
 * Capture callbacks
 */

void onCaptureFailed(void* context, ACameraCaptureSession* session,
                     ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    LOGE("onCaptureFailed ");
}

void onCaptureSequenceCompleted(void* context, ACameraCaptureSession* session,
                                int sequenceId, int64_t frameNumber)
{}

void onCaptureSequenceAborted(void* context, ACameraCaptureSession* session,
                              int sequenceId)
{}

void onCaptureCompleted (
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result)
{
    LOGD("Capture completed");
}

static ACameraCaptureSession_captureCallbacks captureCallbacks {
        .context = nullptr,
        .onCaptureStarted = nullptr,
        .onCaptureProgressed = nullptr,
        .onCaptureCompleted = onCaptureCompleted,
        .onCaptureFailed = onCaptureFailed,
        .onCaptureSequenceCompleted = onCaptureSequenceCompleted,
        .onCaptureSequenceAborted = onCaptureSequenceAborted,
        .onCaptureBufferLost = nullptr,
};


/**
 * Functions used to set-up the camera and draw
 * camera frames into SurfaceTexture
 */

void CameraEngine::initCam()
{
    cameraManager = ACameraManager_create();

    auto id = getBackFacingCamId(cameraManager);
    ACameraManager_openCamera(cameraManager, id.c_str(), &cameraDeviceCallbacks, &cameraDevice);

    printCamProps(cameraManager, id.c_str());
}

void CameraEngine::exitCam()
{
    if (cameraManager)
    {
        // Stop recording to SurfaceTexture and do some cleanup
        ACameraCaptureSession_stopRepeating(textureSession);
        ACameraCaptureSession_close(textureSession);
        ACaptureSessionOutputContainer_free(outputs);
        ACaptureSessionOutput_free(output);

        ACameraDevice_close(cameraDevice);
        ACameraManager_delete(cameraManager);
        cameraManager = nullptr;

        AImageReader_delete(imageReader);
        imageReader = nullptr;

        // Capture request for SurfaceTexture
        ACaptureRequest_free(request);
    }
}

void CameraEngine::initCamSession(ImageProcessor &imgProc)
{
    // TODO set values from Camera2ndk api, for now use hardcoded values of createJpegReader()
    imgProc.w = 1920;
    imgProc.h = 1080;
    imgProc.pipe.initImagePipe(1920, 1080, 4);

    // Prepare request for texture target
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &request);

    // Prepare outputs for session
    ACaptureSessionOutputContainer_create(&outputs);

// Enable ImageReader example in CMakeLists.txt. This will additionally
// make image data available in imageCallback().
    imageReader = createJpegReader(imgProc);
    imageWindow = createSurface(imageReader);
    ANativeWindow_acquire(imageWindow);
    ACameraOutputTarget_create(imageWindow, &imageTarget);
    ACaptureRequest_addTarget(request, imageTarget);
    ACaptureSessionOutput_create(imageWindow, &imageOutput);
    ACaptureSessionOutputContainer_add(outputs, imageOutput);

    // Create the session
    ACameraDevice_createCaptureSession(cameraDevice, outputs, &sessionStateCallbacks, &textureSession);

    // Start capturing continuously
    ACameraCaptureSession_setRepeatingRequest(textureSession, &captureCallbacks, 1, &request, nullptr);
}

void CameraEngine::setSize(int w, int h)
{
    width = w;
    height = h;
}