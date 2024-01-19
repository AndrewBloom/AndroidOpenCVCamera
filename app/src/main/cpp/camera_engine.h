#include <jni.h>
#include <string>
#include <fstream>
#include <vector>
#include <thread>

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraDevice.h>
#include <media/NdkImageReader.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "cam_utils.h"
#include "common.hpp"


using namespace sixo;


/**
 * Variables used to initialize and manage native camera
 */

static ACameraManager* cameraManager = nullptr;

static ACameraDevice* cameraDevice = nullptr;

static ACaptureRequest* request = nullptr;

static ACameraCaptureSession* textureSession = nullptr;

static ANativeWindow* imageWindow = nullptr;

static ACameraOutputTarget* imageTarget = nullptr;

static AImageReader* imageReader = nullptr;

static ACaptureSessionOutput* imageOutput = nullptr;

static ACaptureSessionOutput* output = nullptr;

static ACaptureSessionOutputContainer* outputs = nullptr;


/**
 * GL stuff - mostly used to draw the frames captured
 * by camera into a SurfaceTexture
 */

static GLuint prog;
static GLuint vtxShader;
static GLuint fragShader;

static GLint vtxPosAttrib;
static GLint uvsAttrib;
static GLint mvpMatrix;
static GLint texMatrix;
static GLint texSampler;
static GLint color;
static GLint size;
static GLuint buf[2];
static GLuint textureId;

static int width = 640;
static int height = 480;


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


/**
 * Image reader setup. If you want to use AImageReader, enable this in CMakeLists.txt.
 */

static void imageCallback(void* context, AImageReader* reader)
{
    AImage *image = nullptr;
    auto status = AImageReader_acquireNextImage(reader, &image);
    LOGD("imageCallback()");
    // Check status here ...

    // Try to process data without blocking the callback
    std::thread processor([=](){

        uint8_t *data = nullptr;
        int len = 0;
        AImage_getPlaneData(image, 0, &data, &len);

        // Process data here
        // ...

        AImage_delete(image);
    });
    processor.detach();
}

AImageReader* createJpegReader()
{
    AImageReader* reader = nullptr;
    media_status_t status = AImageReader_new(640, 480, AIMAGE_FORMAT_JPEG,
                     4, &reader);

    //if (status != AMEDIA_OK)
        // Handle errors here

    AImageReader_ImageListener listener{
            .context = nullptr,
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

static void initCam()
{
    cameraManager = ACameraManager_create();

    auto id = getBackFacingCamId(cameraManager);
    ACameraManager_openCamera(cameraManager, id.c_str(), &cameraDeviceCallbacks, &cameraDevice);

    printCamProps(cameraManager, id.c_str());
}

static void exitCam()
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

static void initCam(JNIEnv* env, jobject surface)
{
    // Prepare request for texture target
    ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &request);

    // Prepare outputs for session
    ACaptureSessionOutputContainer_create(&outputs);

// Enable ImageReader example in CMakeLists.txt. This will additionally
// make image data available in imageCallback().
    imageReader = createJpegReader();
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

static void initSurface(JNIEnv* env, jint texId, jobject surface)
{
    // Prepare the surfaces/targets & initialize session
    initCam(env, surface);
}

static void drawFrame(JNIEnv* env, jfloatArray texMatArray)
{

}


/**
 * JNI stuff
 */

extern "C" {

JNIEXPORT void JNICALL
Java_eu_sisik_cam_MainActivity_initCam(JNIEnv *env, jobject)
{
initCam();
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_MainActivity_exitCam(JNIEnv *env, jobject)
{
exitCam();
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_CamRenderer_onSurfaceCreated(JNIEnv *env, jobject, jint texId, jobject surface)
{
LOGD("onSurfaceCreated()");
initSurface(env, texId, surface);
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_CamRenderer_onSurfaceChanged(JNIEnv *env, jobject, jint w, jint h)
{
width = w;
height = h;
}

JNIEXPORT void JNICALL
Java_eu_sisik_cam_CamRenderer_onDrawFrame(JNIEnv *env, jobject, jfloatArray texMatArray)
{
drawFrame(env, texMatArray);
}
} // Extern C