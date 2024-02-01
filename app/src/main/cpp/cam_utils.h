//
// Created by Andrea Fiorito on 19/01/2024.
//

#ifndef ANDROIDOPENCVCAMERA_CAM_UTILS_H
#define ANDROIDOPENCVCAMERA_CAM_UTILS_H

#include <camera/NdkCameraManager.h>
#include <string>
#include <media/NdkImage.h>

namespace sixo {

    bool
    calcPreviewSize(ACameraManager *cameraManager, const char *id, const int32_t req_f, const int req_w,
                    const int req_h, int32_t &out_w, int32_t &out_h);

    void printCamProps(ACameraManager *cameraManager, const char *id, const int32_t req_f);

    std::string getBackFacingCamId(ACameraManager *cameraManager);

    uint8_t *convert_YUV_420_888_to_YUV_12(const AImage *im);

    // interleave and deinterleave from https://stackoverflow.com/questions/14567786/fastest-de-interleave-operation-in-c
    void interleave(const uint8_t *srcA, const uint8_t *srcB, uint8_t *dstAB, size_t dstABLength);

    void deinterleave(const uint8_t *srcAB, uint8_t *dstA, uint8_t *dstB, size_t srcABLength);

}

#endif //ANDROIDOPENCVCAMERA_CAM_UTILS_H
