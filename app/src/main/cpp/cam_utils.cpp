//
// Created by Andrea Fiorito on 19/01/2024.
//

// Copyright (c) 2018 Roman Sisik

#include "cam_utils.h"
#include "common.hpp"
#include <media/NdkImageReader.h>
#include <assert.h>

#if defined(__ARM_NEON)
#include "arm_neon.h"
#endif

namespace sixo {

    // Mimicking openCV Camera2Renderer function cacPreviewSize
    bool
    calcPreviewSize(ACameraManager *cameraManager, const char *id, const int32_t req_f, const int req_w,
                   const int req_h, int32_t &out_w, int32_t &out_h) {
        ACameraMetadata *metadataObj;
        ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);
        ACameraMetadata_const_entry entry = {0};

        ACameraMetadata_getConstEntry(metadataObj,
                                      ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

        float aspect = (float) req_w / req_h;
        for (int i = 0; i < entry.count; i += 4) {
            // We are only interested in output streams, so skip input stream
            int32_t input = entry.data.i32[i + 3];
            if (input)
                continue;

            int32_t format = entry.data.i32[i + 0];
            if (format == req_f) {
                int32_t w = entry.data.i32[i + 1];
                int32_t h = entry.data.i32[i + 2];
                if (w <= req_w && h <= req_h &&
                    out_w <= w && out_h <= h &&
                    abs(aspect - (float) w / h) < 0.2) {
                    out_w = w;
                    out_h = h;
                }
            }
        }
        LOGI("best size: %d, %d", out_w, out_h);
        if( out_w == 0 || out_h == 0) return false;
        else return true;
    }

    void printCamProps(ACameraManager *cameraManager, const char *id, const int32_t req_f)
    {
        ACameraMetadata *metadataObj;
        ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);
        ACameraMetadata_const_entry entry = {0};

        // exposure range
        ACameraMetadata_getConstEntry(metadataObj,
                                      ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry);

        int64_t minExposure = entry.data.i64[0];
        int64_t maxExposure = entry.data.i64[1];
        LOGD("camProps: minExposure=%lld vs maxExposure=%lld", minExposure, maxExposure);
        ////////////////////////////////////////////////////////////////

        // sensitivity
        ACameraMetadata_getConstEntry(metadataObj,
                                      ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry);

        int32_t minSensitivity = entry.data.i32[0];
        int32_t maxSensitivity = entry.data.i32[1];

        LOGD("camProps: minSensitivity=%d vs maxSensitivity=%d", minSensitivity, maxSensitivity);
        ////////////////////////////////////////////////////////////////

        ACameraMetadata_getConstEntry(metadataObj, ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, &entry);
        for (int i = 0; i < entry.count; )
        {
            int32_t fps_low = entry.data.i32[i++];
            int32_t fps_high = entry.data.i32[i++];
            LOGI("Frame rate range: [%d,%d] fps", fps_low, fps_high);
        }

        ACameraMetadata_getConstEntry(metadataObj,
                                      ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

        for (int i = 0; i < entry.count; i += 4)
        {
            // We are only interested in output streams, so skip input stream
            int32_t input = entry.data.i32[i + 3];
            if (input)
                continue;

            int32_t format = entry.data.i32[i + 0];
            if (format == req_f)
            {
                int32_t width = entry.data.i32[i + 1];
                int32_t height = entry.data.i32[i + 2];
                LOGD("camProps: maxWidth=%d vs maxHeight=%d", width, height);
            }
        }

        // cam facing
        ACameraMetadata_getConstEntry(metadataObj,
                                      ACAMERA_SENSOR_ORIENTATION, &entry);

        int32_t orientation = entry.data.i32[0];
        LOGD("camProps: %d", orientation);
    }


    std::string getBackFacingCamId(ACameraManager *cameraManager)
    {
        ACameraIdList *cameraIds = nullptr;
        ACameraManager_getCameraIdList(cameraManager, &cameraIds);

        std::string backId;

        LOGD("found camera count %d", cameraIds->numCameras);

        for (int i = 0; i < cameraIds->numCameras; ++i)
        {
            const char *id = cameraIds->cameraIds[i];

            ACameraMetadata *metadataObj;
            ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);

            ACameraMetadata_const_entry lensInfo = {0};
            ACameraMetadata_getConstEntry(metadataObj, ACAMERA_LENS_FACING, &lensInfo);

            auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                    lensInfo.data.u8[0]);

            // Found a back-facing camera?
            if (facing == ACAMERA_LENS_FACING_BACK)
            {
                backId = id;
                break;
            }
        }

        ACameraManager_deleteCameraIdList(cameraIds);

        return backId;
    }

    uint8_t * convert_YUV_420_888_to_YUV_12(const AImage *im)
    {
        int w, h;
        uint8_t* outBuf;
        int obSize;
        int Yrs, UVrs,UVps; // Yps = 1, Urs == Vrs, Ups == Vrs. API guaranteed.
        int Ylen, Ulen, Vlen;
        uint8_t *Ydata, *Udata, *Vdata;

        AImage_getWidth(im, &w);
        AImage_getHeight(im, &h);
        assert(((3*w*h) & 0x1) == 0);
        obSize = 3 * w * h / 2; // Y is w*h, U is w*h/4, V is w*h/4
        outBuf = new (std::align_val_t(64)) uint8_t[obSize];

        AImage_getPlaneRowStride(im, 0, &Yrs);
        AImage_getPlaneRowStride(im, 1, &UVrs);
        AImage_getPlanePixelStride(im, 1, &UVps);
        AImage_getPlaneData(im, 0, &Ydata, &Ylen);
        AImage_getPlaneData(im, 1, &Udata, &Ulen);
        AImage_getPlaneData(im, 2, &Vdata, &Vlen);

        assert(w == Yrs);
        if (Yrs == w) memcpy(outBuf, Ydata, Ylen);
        else for (int i=0; i<h; i++) memcpy(outBuf+i*w, Ydata+i*Yrs, w);

        if (UVps == 1) {
            // YUV is planar, similar to I420
            // TODO this is currently not tested
            LOGD("WARNING! using the interleave function! needs proper testing!");
            interleave(Udata, Vdata, outBuf+w*h, w*h/2);
        } else if (UVps == 2) {
            // YUV is semi-planar, similar to nv12 format
            assert(Udata-Vdata == 1);
            if (UVrs == w) memcpy(outBuf+w*h, Udata, w*h/2); // tested
            else for (int i = 0; i < h / 2; i++) memcpy(outBuf+w*h+i*w, Udata+i*UVrs, w); // not tested
        } else {
            // YUV is semi-planar
            LOGD("WARNING! using the slowest interleave code! needs proper testing!");
            // TODO this is currently not tested
            int idx = w*h;
            for (int i=0; i<h/2; i++)
                for (int j=0; j<w/2; j++) {
                    outBuf[idx++] = Udata[j * UVps + i * UVrs];
                    outBuf[idx++] = Vdata[j * UVps + i * UVrs];
                }
        }
        return outBuf;
    }

    void interleave(const uint8_t *srcA, const uint8_t *srcB, uint8_t *dstAB, size_t dstABLength)
    {
#if defined(__ARM_NEON)
        LOGD("WARNING! using the interleave function! ARM_NEON optimisation!");
        // attempt to use NEON intrinsics

    // iterate 32-bytes at a time
    div_t dstABLength_32 = div(dstABLength, 32);
    if (dstABLength_32.rem == 0)
    {
        while (dstABLength_32.quot --> 0)
        {
            const uint8x16_t a = vld1q_u8(srcA);
            const uint8x16_t b = vld1q_u8(srcB);
            const uint8x16x2_t ab = { a, b };
            vst2q_u8(dstAB, ab);
            srcA += 16;
            srcB += 16;
            dstAB += 32;
        }
        return;
    }

    // iterate 16-bytes at a time
    div_t dstABLength_16 = div(dstABLength, 16);
    if (dstABLength_16.rem == 0)
    {
        while (dstABLength_16.quot --> 0)
        {
            const uint8x8_t a = vld1_u8(srcA);
            const uint8x8_t b = vld1_u8(srcB);
            const uint8x8x2_t ab = { a, b };
            vst2_u8(dstAB, ab);
            srcA += 8;
            srcB += 8;
            dstAB += 16;
        }
        return;
    }
#endif

        // if the bytes were not aligned properly
        // or NEON is unavailable, fall back to
        // an optimized iteration

        LOGD("WARNING! using the interleave function! standard CPU optimisation!");
        // iterate 8-bytes at a time
        div_t dstABLength_8 = div(dstABLength, 8);
        if (dstABLength_8.rem == 0)
        {
            typedef union
            {
                uint64_t wide;
                struct { uint8_t a1; uint8_t b1; uint8_t a2; uint8_t b2; uint8_t a3; uint8_t b3; uint8_t a4; uint8_t b4; } narrow;
            } ab8x8_t;

            uint64_t *dstAB64 = (uint64_t *)dstAB;
            int j = 0;
            for (int i = 0; i < dstABLength_8.quot; i++)
            {
                ab8x8_t cursor;
                cursor.narrow.a1 = srcA[j  ];
                cursor.narrow.b1 = srcB[j++];
                cursor.narrow.a2 = srcA[j  ];
                cursor.narrow.b2 = srcB[j++];
                cursor.narrow.a3 = srcA[j  ];
                cursor.narrow.b3 = srcB[j++];
                cursor.narrow.a4 = srcA[j  ];
                cursor.narrow.b4 = srcB[j++];
                dstAB64[i] = cursor.wide;
            }
            return;
        }

        // iterate 4-bytes at a time
        div_t dstABLength_4 = div(dstABLength, 4);
        if (dstABLength_4.rem == 0)
        {
            typedef union
            {
                uint32_t wide;
                struct { uint8_t a1; uint8_t b1; uint8_t a2; uint8_t b2; } narrow;
            } ab8x4_t;

            uint32_t *dstAB32 = (uint32_t *)dstAB;
            int j = 0;
            for (int i = 0; i < dstABLength_4.quot; i++)
            {
                ab8x4_t cursor;
                cursor.narrow.a1 = srcA[j  ];
                cursor.narrow.b1 = srcB[j++];
                cursor.narrow.a2 = srcA[j  ];
                cursor.narrow.b2 = srcB[j++];
                dstAB32[i] = cursor.wide;
            }
            return;
        }

        // iterate 2-bytes at a time
        div_t dstABLength_2 = div(dstABLength, 2);
        typedef union
        {
            uint16_t wide;
            struct { uint8_t a; uint8_t b; } narrow;
        } ab8x2_t;

        uint16_t *dstAB16 = (uint16_t *)dstAB;
        for (int i = 0; i < dstABLength_2.quot; i++)
        {
            ab8x2_t cursor;
            cursor.narrow.a = srcA[i];
            cursor.narrow.b = srcB[i];
            dstAB16[i] = cursor.wide;
        }
    }

    void deinterleave(const uint8_t *srcAB, uint8_t *dstA, uint8_t *dstB, size_t srcABLength)
    {
#if defined(__ARM_NEON)
        LOGD("WARNING! using the deinterleave function! ARM_NEON optimisation!");
        // attempt to use NEON intrinsics

    // iterate 32-bytes at a time
    div_t srcABLength_32 = div(srcABLength, 32);
    if (srcABLength_32.rem == 0)
    {
        while (srcABLength_32.quot --> 0)
        {
            const uint8x16x2_t ab = vld2q_u8(srcAB);
            vst1q_u8(dstA, ab.val[0]);
            vst1q_u8(dstB, ab.val[1]);
            srcAB += 32;
            dstA += 16;
            dstB += 16;
        }
        return;
    }

    // iterate 16-bytes at a time
    div_t srcABLength_16 = div(srcABLength, 16);
    if (srcABLength_16.rem == 0)
    {
        while (srcABLength_16.quot --> 0)
        {
            const uint8x8x2_t ab = vld2_u8(srcAB);
            vst1_u8(dstA, ab.val[0]);
            vst1_u8(dstB, ab.val[1]);
            srcAB += 16;
            dstA += 8;
            dstB += 8;
        }
        return;
    }
#endif

        // if the bytes were not aligned properly
        // or NEON is unavailable, fall back to
        // an optimized iteration

        LOGD("WARNING! using the deinterleave function! standard CPU optimisation!");
        // iterate 8-bytes at a time
        div_t srcABLength_8 = div(srcABLength, 8);
        if (srcABLength_8.rem == 0)
        {
            typedef union
            {
                uint64_t wide;
                struct { uint8_t a1; uint8_t b1; uint8_t a2; uint8_t b2; uint8_t a3; uint8_t b3; uint8_t a4; uint8_t b4; } narrow;
            } ab8x8_t;

            uint64_t *srcAB64 = (uint64_t *)srcAB;
            int j = 0;
            for (int i = 0; i < srcABLength_8.quot; i++)
            {
                ab8x8_t cursor;
                cursor.wide = srcAB64[i];
                dstA[j  ] = cursor.narrow.a1;
                dstB[j++] = cursor.narrow.b1;
                dstA[j  ] = cursor.narrow.a2;
                dstB[j++] = cursor.narrow.b2;
                dstA[j  ] = cursor.narrow.a3;
                dstB[j++] = cursor.narrow.b3;
                dstA[j  ] = cursor.narrow.a4;
                dstB[j++] = cursor.narrow.b4;
            }
            return;
        }
        LOGD("WARNING! using the deinterleave function! standard CPU optimisation! after 8 block");

        // iterate 4-bytes at a time
        div_t srcABLength_4 = div(srcABLength, 4);
        if (srcABLength_4.rem == 0)
        {
            typedef union
            {
                uint32_t wide;
                struct { uint8_t a1; uint8_t b1; uint8_t a2; uint8_t b2; } narrow;
            } ab8x4_t;

            uint32_t *srcAB32 = (uint32_t *)srcAB;
            int j = 0;
            for (int i = 0; i < srcABLength_4.quot; i++)
            {
                ab8x4_t cursor;
                cursor.wide = srcAB32[i];
                dstA[j  ] = cursor.narrow.a1;
                dstB[j++] = cursor.narrow.b1;
                dstA[j  ] = cursor.narrow.a2;
                dstB[j++] = cursor.narrow.b2;
            }
            return;
        }

        // iterate 2-bytes at a time
        div_t srcABLength_2 = div(srcABLength, 2);
        typedef union
        {
            uint16_t wide;
            struct { uint8_t a; uint8_t b; } narrow;
        } ab8x2_t;

        uint16_t *srcAB16 = (uint16_t *)srcAB;
        for (int i = 0; i < srcABLength_2.quot; i++)
        {
            ab8x2_t cursor;
            cursor.wide = srcAB16[i];
            dstA[i] = cursor.narrow.a;
            dstB[i] = cursor.narrow.b;
        }
    }
}