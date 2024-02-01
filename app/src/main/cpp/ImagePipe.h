//
// Created by Andrea Fiorito on 25/01/2024.
//

#ifndef HAND_TRACKING_CLIENT_EXAMPLE_IMAGEPIPE_H
#define HAND_TRACKING_CLIENT_EXAMPLE_IMAGEPIPE_H

#include "algo/TripleBuffer.h"
#include <cstdint>

class ImagePipe {
private:
    uint8_t *internalBuffer;
    uint8_t* *buffers;
public:
    std::atomic_bool init = false;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    TripleBuffer<uint8_t*> *imageTripleBuf = nullptr;
    void initImagePipe(uint32_t w, uint32_t h, uint32_t bpp){
        uint64_t imgBytes = w * h * bpp;
        buffers = new uint8_t*[3];
        internalBuffer = new uint8_t[imgBytes * 3];
        buffers[0] = internalBuffer;
        buffers[1] = internalBuffer + imgBytes;
        buffers[2] = internalBuffer + imgBytes * 2;
        this->width = w;
        this->height = h;
        this->bpp = bpp;
        this->imageTripleBuf = new TripleBuffer<uint8_t *>((uint8_t *(&)[3])*buffers);
        this->init = true;
    }
    ~ImagePipe() {
        if (imageTripleBuf != nullptr) {
            delete imageTripleBuf;
            delete[] buffers;
            delete[] internalBuffer;
        }
    }
};


#endif //HAND_TRACKING_CLIENT_EXAMPLE_IMAGEPIPE_H
