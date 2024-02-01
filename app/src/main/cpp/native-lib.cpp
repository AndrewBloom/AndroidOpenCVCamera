#include <jni.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>

#include "common.hpp"
#include "camera_engine.h"

#include "Model3d.h"
#include "ImagePipe.h"

void processFrame(int w, int h, uint8_t *buffer, ImagePipe &pipe) {
    // This could be heavily simplified using luminance channel as grey image, it would then
    // skip the conversion from N12 to RGBA and RGBA to grayscale. It gives 50% gain, left as it is
    // just to document some conversions...
    cv::UMat yuv21 = cv::Mat(h + h / 2, w, CV_8UC1, buffer).getUMat(cv::ACCESS_READ);

    cv::UMat m = cv::UMat(h, w, CV_8UC4);
    LOGD("Processing on CPU");
    int64_t t;

    t = getTimeMs();
    cvtColor(yuv21, m, CV_YUV2RGBA_NV12);
    // Check if we should flip image due to frontFacing
    // I don't think this should be required, but I can't find
    // a way to get the OpenCV Android SDK to do this properly
    // (also, time taken to flip image is negligible)
    //if(front_facing){
    // back camera, flip it vertically
        flip(m, m, 0);
    //}
    LOGD("flip() costs %d ms", getTimeInterval(t));

    // modify
    t = getTimeMs();
    cvtColor(m, m, CV_RGB2GRAY);
    Laplacian(m, m, CV_8U);
    multiply(m, 10, m);
    cvtColor(m, m, CV_GRAY2RGBA);
    LOGD("Laplacian() costs %d ms", getTimeInterval(t));

    // write back
    t = getTimeMs();

    cv::Mat out(h,w, CV_8UC4, pipe.imageTripleBuf->GetWriteBuffer());
    m.copyTo(out);
    /*int rowSize = pipe.width*4;
        for (int i = 0; i < pipe.height; i+=8)
            for (int j = 0; j < 8; j++)
                memset(pipe.imageTripleBuf->GetWriteBuffer() + (i * rowSize) + (j  * rowSize) , (j<4) ? 255 : 0, rowSize);
*/
    pipe.imageTripleBuf->SwapWriteBuffers();
    LOGD("Copying to pipe costs %d ms", getTimeInterval(t));
}

CameraEngine cameraEngine;

ImagePipe imgPipe;
ImageProcessor imgProc(imgPipe);

const float QUAD[4 * 4] = { // vertex , tex_coord
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
};

ShaderManager *shaderManager;
Model3d *textureQuad;

// ----------------------------------------------------------------------------

extern "C" {
void resize(int w, int h);
void render();
}

void resize(int w, int h) {
    glViewport(0, 0, w, h);
}

void render() {
    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //checkGlError("Renderer::render");
}

// ----------------------------------------------------------------------------
enum {
    UNINIT,
    SET
} renderer_state = UNINIT;

extern "C"
JNIEXPORT void JNICALL
Java_com_bloomengineeringltd_androidopencvcamera_GLES3JNILib_init(JNIEnv *env, jobject thiz) {
    LOGD("onSurfaceCreated()");
    imgProc.processFunc = processFrame;
    cameraEngine.initCamSession(imgProc);
    shaderManager = new ShaderManager();
    textureQuad = new Model3d(4, ShaderManager::TEXTURE2D, shaderManager);
    textureQuad->loadBuffers((void *) QUAD);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bloomengineeringltd_androidopencvcamera_GLES3JNILib_resize(JNIEnv *env, jobject thiz, jint width, jint height) {
    cameraEngine.setSize(width, height);
    resize(width, height);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bloomengineeringltd_androidopencvcamera_GLES3JNILib_step(JNIEnv *env, jobject thiz) {
    if (imgPipe.init && renderer_state == UNINIT) {
        FShaderParams *f_sh_par = shaderManager->getFShaderParams(ShaderManager::TEXTURE2D);
        f_sh_par->getWidthArray()[0] = imgPipe.width;
        f_sh_par->getHeightArray()[0] = imgPipe.height;
        textureQuad->makeRenderer(*f_sh_par);
        delete f_sh_par;
        renderer_state = SET;
    }
    if (renderer_state == SET) {
        uint8_t * texture = imgPipe.imageTripleBuf->SwapAndRead();
        textureQuad->loadTexture(texture);
        render();
        textureQuad->draw();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_bloomengineeringltd_androidopencvcamera_CameraActivity_initCam(JNIEnv *env, jobject thiz) {
    cameraEngine.initCam();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_bloomengineeringltd_androidopencvcamera_CameraActivity_exitCam(JNIEnv *env, jobject thiz) {
    cameraEngine.exitCam();
}