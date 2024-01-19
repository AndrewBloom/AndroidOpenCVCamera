//
// Created by Andrea Fiorito on 19/01/2024.
//

#ifndef ANDROIDOPENCVCAMERA_CAM_UTILS_H
#define ANDROIDOPENCVCAMERA_CAM_UTILS_H

#include <camera/NdkCameraManager.h>
#include <string>

namespace sixo {

    void printCamProps(ACameraManager *cameraManager, const char *id);

    std::string getBackFacingCamId(ACameraManager *cameraManager);

}

#endif //ANDROIDOPENCVCAMERA_CAM_UTILS_H
