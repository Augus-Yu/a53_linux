/*
* Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
* Description: Hisilicon UVC gadget test application.
* Author: Liang Shengjun <liangshengjun@hisilicon.com>
*         Jianyed <yejian9@hisilicon.com>
* Create: 2019-06-08
*/

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <linux/usb/ch9.h>

#include "hicamera.h"
#include "uvc.h"
#include "uvc_venc_glue.h"
#include "histream.h"
#include "hi_ctrl.h"

#undef HI_DEBUG
#ifdef HI_DEBUG
#define DEBUG   printf
#else
#define DEBUG   Nothing
#endif

#define ERR_INFO    printf

static struct HiUvcDev *HiCD = 0;
static frame_node_t *waitedNode[WAITED_NODE_SIZE];

HI_VOID Nothing()
{

}

static const HI_CHAR* ToString(HI_U32 u32Format)
{
    switch (u32Format)
    {
        case VIDEO_IMG_FORMAT_H264:
            return "H264";
            break;
        case VIDEO_IMG_FORMAT_MJPEG:
            return "MJPEG";
            break;
        case VIDEO_IMG_FORMAT_YUYV:
            return "YUYV";
            break;
        case VIDEO_IMG_FORMAT_YUV420:
            return "YUV420";
            break;
        default:
            return "unknown format";
            break;
    }
}

static const HI_CHAR* GetCode(HI_S32 s32C)
{
    if (s32C == 0x01)
    {
        return "SET_CUR";
    }
    else if (s32C == 0x81)
    {
        return "GET_CUR";
    }
    else if (s32C == 0x82)
    {
        return "GET_MIN";
    }
    else if (s32C == 0x83)
    {
        return "GET_MAX";
    }
    else if (s32C == 0x84)
    {
        return "GET_RES";
    }
    else if (s32C == 0x85)
    {
        return "GET_LEN";
    }
    else if (s32C == 0x86)
    {
        return "GET_INFO";
    }
    else if (s32C == 0x87)
    {
        return "GET_DEF";
    }
    else
    {
        return "UNKNOW";
    }
}

static const HI_CHAR *GetIntfCsS(HI_S32 s32C)
{
    if (s32C == 0x01)
    {
        return "PROB_CONTROL";
    }
    else if (s32C == 0x02)
    {
        return "COMMIT_CONTROL";
    }
    else
    {
        return "UNKOWN";
    }
}

static HI_VOID ClearWaitedNode(HI_VOID)
{
    HI_S32 i = 0;
    uvc_cache_t *uvcCache = uvc_cache_get();

    for (i = 0; i < WAITED_NODE_SIZE; i++)
    {
        if ((waitedNode[i] != 0) && uvcCache)
        {
            put_node_to_queue(uvcCache->free_queue, waitedNode[i]);
            waitedNode[i] = 0;
        }
    }
}

static struct HiUvcDev *HiUvcInit(const HI_CHAR* devName)
{
    struct HiUvcDev* hiDev;
    struct VideoAbility cap;
    HI_S32 s32Ret;
    HI_S32 s32Fd;

    s32Fd = open(devName, O_RDWR | O_NONBLOCK);
    if (s32Fd == -1)
    {
        ERR_INFO("v4l2 open failed(%s): %s (%d)\n",devName, strerror(errno), errno);
        return NULL;
    }

    s32Ret = ioctl(s32Fd, VIDEO_IOCTL_QUERY_CAP, &cap);
    if (s32Ret < 0)
    {
        ERR_INFO("unable to query device: %s (%d)\n", strerror(errno),
            errno);
        close(s32Fd);
        return NULL;
    }

    DEBUG("open succeeded(%s:caps=0x%04x)\n", devName, cap.dwCaps);

    if (!(cap.dwCaps & 0x02)) {
        close(s32Fd);
        return NULL;
    }

    DEBUG("device is %s on bus %s\n", cap.aCard, cap.aBusInfo);

    hiDev = (struct HiUvcDev*)malloc(sizeof * hiDev);
    if (hiDev == NULL)
    {
        close(s32Fd);
        return NULL;
    }

    memset(hiDev, 0, sizeof * hiDev);
    hiDev->iFd = s32Fd;

    CLEAR(waitedNode);
    return hiDev;
}

static HI_VOID HiUvcVideoFillBufferUserptr(struct HiUvcDev* hiDev, struct VideoCache* vCache)
{
    HI_S32 s32RetryCount = 0;
    vCache->dwUsed = 0;

    switch (hiDev->dwFcc)
    {
    case VIDEO_IMG_FORMAT_MJPEG:
    case VIDEO_IMG_FORMAT_H264:
    case VIDEO_IMG_FORMAT_YUYV:
    case VIDEO_IMG_FORMAT_YUV420:
    {
        uvc_cache_t *cUvcCache = uvc_cache_get();
        frame_node_t *cNode = 0;
        frame_queue_t *cQ = 0, *cFq = 0;

retry:
        if (cUvcCache)
        {
            cQ  = cUvcCache->ok_queue;
            cFq = cUvcCache->free_queue;
            get_node_from_queue(cQ, &cNode);
        }

        if ((waitedNode[vCache->dwIndex] != 0) && cUvcCache)
        {
            put_node_to_queue(cFq, waitedNode[vCache->dwIndex]);
            waitedNode[vCache->dwIndex] = 0;
        }

        if (cNode != 0)
        {
            vCache->dwUsed = cNode->used;
            vCache->n.ulUsr = (HI_UL)cNode->mem;
            vCache->dwLen = cNode->length;
            waitedNode[vCache->dwIndex] = cNode;

        }
        else if (s32RetryCount++ < 60)
        {
            /*
             * Notice, the perfect solution is using locker and waiting queue's notify.
             * But here just only simply used usleep method and try again.
             * it works fine now.
             */
            usleep(5000);
            goto retry;
        }
    }
        break;
    default:
        DEBUG("what's up....\n");
        break;
    }
}

static HI_S32 HiUvcVideoProcessUserptr(struct HiUvcDev* hiDev)
{
    struct VideoCache vCache;
    HI_S32 s32Ret;

    DEBUG("#############HiUvcVideoProcessUserptr()#############\n");

    memset(&vCache, 0, sizeof vCache);
    vCache.dwType = VIDEO_CACHE_TYPE_OPT;
    vCache.dwMem = VIDEO_MM_USER;

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DQUEUE_BUF, &vCache);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    HiUvcVideoFillBufferUserptr(hiDev, &vCache);

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_QUEUE_BUF, &vCache);
    if (s32Ret < 0)
    {
        ERR_INFO("Unable to requeue buffer(1): %s (%d).\n", strerror(errno), errno);
        return s32Ret;
    }

    return 0;
}

static HI_S32 HiUvcVideoReqbufsUserptr(struct HiUvcDev* hiDev, HI_S32 s32NBufs)
{
    struct VideoBufRequest vRb;
    HI_S32 s32Ret;

    hiDev->dwNbufs = 0;

    memset(&vRb, 0, sizeof vRb);
    vRb.dwCnt = s32NBufs;
    vRb.dwType = VIDEO_CACHE_TYPE_OPT;
    vRb.dwMem = VIDEO_MM_USER;

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_REQUEST_BUF, &vRb);
    if (s32Ret < 0)
    {
        ERR_INFO("Unable to allocate buffers: %s (%d).\n",
             strerror(errno), errno);
        return s32Ret;
    }

    hiDev->dwNbufs = vRb.dwCnt;

    DEBUG("%u buffers allocated.\n", vRb.dwCnt);
    return 0;
}

static HI_S32 HiUvcVideoStreamUserptr(struct HiUvcDev* hiDev, HI_S32 s32Enable)
{
    struct VideoCache vCache;
    HI_U32 i;
    HI_S32 s32Type = VIDEO_CACHE_TYPE_OPT;
    HI_S32 s32Ret = 0;

    DEBUG("%s:Starting video stream.\n", __func__);

    for (i = 0; i < (hiDev->dwNbufs); ++i)
    {
        memset(&vCache, 0, sizeof vCache);

        vCache.dwIndex = i;
        vCache.dwType   = VIDEO_CACHE_TYPE_OPT;
        vCache.dwMem = VIDEO_MM_USER;

        HiUvcVideoFillBufferUserptr(hiDev, &vCache);

        s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_QUEUE_BUF, &vCache);
        if (s32Ret < 0)
        {
            ERR_INFO("Unable to queue buffer(%d): %s (%d).\n", i,
                 strerror(errno), errno);
            break;
        }
    }

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_STREAM_ON, &s32Type);
    if (s32Ret < 0)
    {
        ERR_INFO("Unable to stream on: %s (%d).\n", strerror(errno), errno);
    }

    hiDev->iStreaming = 1;
    return s32Ret;
}

static HI_S32 HiUvcVideoSetFmt(struct HiUvcDev* hiDev)
{
    struct VideoFmt vFmt;
    HI_S32 s32Ret;

    memset(&vFmt, 0, sizeof vFmt);
    vFmt.dwType = VIDEO_CACHE_TYPE_OPT;
    vFmt.fmt.pix.dwWidth  = hiDev->dwWidth;
    vFmt.fmt.pix.dwHeight = hiDev->dwHeight;
    vFmt.fmt.pix.dwFmt = hiDev->dwFcc;
    vFmt.fmt.pix.dwFld = VIDEO_FLD_NOTHING;

    if ((hiDev->dwFcc == VIDEO_IMG_FORMAT_MJPEG) || (hiDev->dwFcc == VIDEO_IMG_FORMAT_H264))
    {
        vFmt.fmt.pix.dwSize = hiDev->dwImgSize;
    }

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_SET_FORMAT, &vFmt);
    if (s32Ret < 0)
    {
        ERR_INFO("Unable to set format: %s (%d).\n",
            strerror(errno), errno);
    }

    return s32Ret;
}

static HI_VOID HiUvcStreamOff(struct HiUvcDev *hiDev)
{
    HI_S32 s32Type = VIDEO_CACHE_TYPE_OPT;
    HI_S32 s32Ret = 0;

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_STREAM_OFF, &s32Type);
    if (s32Ret < 0)
    {
        ERR_INFO("Unable to stream off: %s (%d).\n", strerror(errno), errno);
    }

    histream_shutdown();
    hiDev->iStreaming = 0;
    DEBUG("Stopping video stream.\n");
 }

static HI_VOID HiUvcVideoDisable(struct HiUvcDev* hiDev)
{
    HiUvcStreamOff(hiDev);
    ClearWaitedNode();
}

static HI_VOID HiUvcVideoEnable(struct HiUvcDev* hiDev)
{
    encoder_property eP;

    clear_ok_queue();
    ClearWaitedNode();
    HiUvcVideoDisable(hiDev);

    eP.format = hiDev->dwFcc;
    eP.width = hiDev->dwWidth;
    eP.height = hiDev->dwHeight;
    eP.compsite = 0;

    histream_set_enc_property(&eP);
    histream_shutdown();
    histream_startup();

    HiUvcVideoReqbufsUserptr(hiDev, WAITED_NODE_SIZE);
    HiUvcVideoStreamUserptr(hiDev, 1);
}

static HI_S32 IsContainUdcSubDir(const HI_CHAR *charPath)
{
    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;

    pDir = opendir(charPath);
    if (pDir == NULL)
    {
        ERR_INFO("No such path: %s\n", charPath);
        return -1;
    }

    while ((pDirent = readdir(pDir)) != NULL)
    {
        if (strcmp(pDirent->d_name, "UDC") == 0)
        {
            closedir(pDir);
            return 0;
        }
    }

    closedir(pDir);

    return -1;
}

static HI_S32 ReadFile(const HI_CHAR *charPath, HI_CHAR *charDest, HI_U32 u32Size)
{
    FILE *input = NULL;

    input = fopen(charPath, "rw");
    if (input == NULL)
    {
        ERR_INFO("No such path: %s\n", charPath);
        return -1;
    }

    fscanf(input, "%s", charDest);

    fclose(input);

    return 0;
}

static HI_S32 GetUdcNodeName(HI_CHAR *charNodeName, HI_U32 u32Size)
{
    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;
    const HI_CHAR *PATH_TMP = "/sys/kernel/config/usb_gadget/";
    HI_CHAR s32Tmp[256];

    pDir = opendir(PATH_TMP);
    if (pDir == NULL)
    {
        ERR_INFO("No such path: %s\n", PATH_TMP);
        return -1;
    }

    while ((pDirent = readdir(pDir)) != NULL)
    {
        if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
        {
            continue;
        }

        if (pDirent->d_type == DT_DIR)
        {
            strncpy(s32Tmp, PATH_TMP, 256);
            strncat(s32Tmp, pDirent->d_name, 256);

            if (IsContainUdcSubDir(s32Tmp) == 0)
            {
                closedir(pDir);

                strncat(s32Tmp, "/UDC", 4);
                return ReadFile(s32Tmp, charNodeName, u32Size);
            }
        }
    }

    closedir(pDir);

    return -1;
}

static HI_S32 GetMaxPayloadTransferSize(void)
{
    const HI_S32 SUPER_SPEED_SIZE = 1024;
    const HI_S32 HIGH_SPEED_SIZE = 3072;
    const HI_S32 FULL_SPEED_SIZE = 1023;
    const HI_CHAR *PATH_TMP = "/sys/class/udc/";

    HI_CHAR charTmp[128];
    HI_CHAR charTargetPath[256];
    HI_S32 s32Result = HIGH_SPEED_SIZE;

    if (GetUdcNodeName(charTmp, 128) != 0)
    {
        return s32Result;
    }

    strncpy(charTargetPath, PATH_TMP, 256);
    strncat(charTargetPath, charTmp, 128);
    strncat(charTargetPath, "/current_speed", 14);

    if (ReadFile(charTargetPath, charTmp, 128) != 0)
    {
        return s32Result;
    }

    if (strcmp(charTmp, "super-speed") == 0)
    {
        s32Result = SUPER_SPEED_SIZE;
    }
    else if (strcmp(charTmp, "high-speed") == 0)
    {
        s32Result = HIGH_SPEED_SIZE;
    }
    else if (strcmp(charTmp, "full-speed") == 0)
    {
        s32Result = FULL_SPEED_SIZE;
    }
    else
    {
        ERR_INFO("USB cable is not connected yet.\n");
    }

    return s32Result;
}

static HI_VOID HiUvcHandleStreamingControl(struct HiUvcDev* hiDev,
                                       struct HiUvcStreamingControl* uStrCtrl,
                                       HI_S32 u32IFrame, HI_S32 s32IFormat)
{
    const struct HiUvcFormatInfo* hiFmtInfo;
    const struct HiUvcFrameInfo* hiFrmInfo;
    HI_U32 s32NFrm;

    if (s32IFormat < 0)
    {
        s32IFormat = ARRAY_SIZE(hiFmt) + s32IFormat;
    }

    if ((s32IFormat < 0) || (s32IFormat >= (HI_S32)ARRAY_SIZE(hiFmt)))
    {
        return;
    }

    DEBUG("s32IFormat = %d\n", s32IFormat);
    hiFmtInfo = &hiFmt[s32IFormat];

    s32NFrm = 0;

    while (hiFmtInfo->frames[s32NFrm].width != 0)
    {
        ++s32NFrm;
    }

    if (u32IFrame < 0)
    {
        u32IFrame = s32NFrm + u32IFrame;
    }

    if ((u32IFrame < 0) || (u32IFrame >= (HI_S32)s32NFrm))
    {
        return;
    }

    hiFrmInfo = &hiFmtInfo->frames[u32IFrame];

    memset(uStrCtrl, 0, sizeof * uStrCtrl);

    uStrCtrl->bmHint = 1;
    uStrCtrl->bFormatIndex = s32IFormat + 1;    /* Yuv: 1, Mjpeg: 2. */
    uStrCtrl->bFrameIndex = u32IFrame + 1;      /* 360p: 1 720p: 2. */
    uStrCtrl->dwFrameInterval = hiFrmInfo->intervals[0];    /* Corresponding to the number of frame rate. */

    switch (hiFmtInfo->fcc)
    {
    case VIDEO_IMG_FORMAT_YUYV:
    case VIDEO_IMG_FORMAT_YUV420:
        uStrCtrl->dwMaxVideoFrameSize = hiFrmInfo->width * hiFrmInfo->height * 2;
        break;
    case VIDEO_IMG_FORMAT_MJPEG:
    case VIDEO_IMG_FORMAT_H264:
        uStrCtrl->dwMaxVideoFrameSize = hiDev->dwImgSize;
        break;
    }

    if (hiDev->dwBulk) {
        uStrCtrl->dwMaxPayloadTransferSize = hiDev->dwBulkSize;   /* This should be filled by the driver. */
    } else {
        uStrCtrl->dwMaxPayloadTransferSize = GetMaxPayloadTransferSize();
    }
    uStrCtrl->bmFramingInfo = 3;
    uStrCtrl->bPreferedVersion = 1;
    uStrCtrl->bMaxVersion = 1;
}

static HI_VOID HiUvcHandleStandardRequest(struct HiUvcDev* hiDev, struct usb_ctrlrequest* uCtrlReq,
                                        struct uvc_request_data* uReqData)
{
    DEBUG("Hicamera standard request\n");
    (HI_VOID)hiDev;
    (HI_VOID)uCtrlReq;
    (HI_VOID)uReqData;
}


static void HiuvcEveUndefControl(struct HiUvcDev* hiDev, HI_U8 u8Cs,
                                struct uvc_request_data* uReqData)
{
    switch (u8Cs)
    {
        case HIUVC_VC_REQUEST_ERROR_CODE_CONTROL:
            uReqData->length = hiDev->request_error_code.length;
            uReqData->data[0] = hiDev->request_error_code.data[0];
            //printf("hiDev->request_error_code.data[0] = %d\n",hiDev->request_error_code.data[0]);
            break;
        default:
            hiDev->request_error_code.length = 1;
            hiDev->request_error_code.data[0] = 0x06;
            break;
    }
}

static HI_VOID HiUvcHandleControlRequest(struct HiUvcDev* hiDev,
                                       HI_U8 u8Req, HI_U8 u8UnitId, HI_U8 u8Cs,
                                       struct uvc_request_data* uReqData)
{
    switch (u8UnitId)
    {
        case HIUVC_VC_DESCRIPTOR_UNDEFINED:
            HiuvcEveUndefControl(hiDev, u8Cs, uReqData);
            break;
        case HIUVC_VC_HEADER:
            histream_event_it_control(hiDev, u8Req, u8UnitId, u8Cs, uReqData);
            break;
        case HIUVC_VC_INPUT_TERMINAL:
            histream_event_pu_control(hiDev, u8Req, u8UnitId, u8Cs, uReqData);
            break;
        case UNIT_XU_H264:
            histream_event_eu_h264_control(hiDev, u8Req, u8UnitId, u8Cs, uReqData);
            break;
        default:
            hiDev->request_error_code.length = 1;
            hiDev->request_error_code.data[0] = 0x06;
    }
}

static HI_VOID HiUvcHandleStreamingRequest(struct HiUvcDev* hiDev, HI_U8 u8Req, HI_U8 u8Cs,
                                         struct uvc_request_data* uReqData)
{
    struct HiUvcStreamingControl* uStrCtrl;

    if ((u8Cs != HIUVC_VS_PROBE_CONTROL) && (u8Cs != HIUVC_VS_COMMIT_CONTROL))
    {
        return;
    }

    uStrCtrl = (struct HiUvcStreamingControl*)&uReqData->data;
    uReqData->length = sizeof * uStrCtrl;

    switch (u8Req)
    {
    case HIUVC_SET_CUR:
        hiDev->iControl = u8Cs;
        uReqData->length = 34;
        break;
    case HIUVC_GET_CUR:
        if (u8Cs == HIUVC_VS_PROBE_CONTROL)
        {
            memcpy(uStrCtrl, &hiDev->probe, sizeof * uStrCtrl);
        }
        else
        {
            memcpy(uStrCtrl, &hiDev->commit, sizeof * uStrCtrl);
        }
        break;
    case HIUVC_GET_MIN:
    case HIUVC_GET_MAX:
        HiUvcHandleStreamingControl(hiDev, uStrCtrl, 0, 0);
        break;
    case HIUVC_GET_DEF:
        HiUvcHandleStreamingControl(hiDev, uStrCtrl, u8Req == HIUVC_GET_MAX ? -1 : 0,
                                   u8Req == HIUVC_GET_MAX ? -1 : 0);
        break;
    case HIUVC_GET_RES:
        memset(uStrCtrl, 0, sizeof * uStrCtrl);
        break;
    case HIUVC_GET_LEN:
        uReqData->data[0] = 0x00;
        uReqData->data[1] = 0x22;
        uReqData->length = 2;
        break;
    case HIUVC_GET_INFO:
        uReqData->data[0] = 0x03;
        uReqData->length = 1;
        break;
    }
}

static HI_VOID SetProbeStatus(struct HiUvcDev* hiDev, HI_S32 u32Cs, HI_S32 u32Req)
{
    if (u32Cs == 0x01)
    {
        switch (u32Req)
        {
        case 0x01:
            hiDev->probeStatus.bSet = 1;
            break;
        case 0x81:
            hiDev->probeStatus.bGet = 1;
            break;
        case 0x82:
            hiDev->probeStatus.bMin = 1;
            break;
        case 0x83:
            hiDev->probeStatus.bMax = 1;
            break;
        case 0x84:
            break;
        case 0x85:
            break;
        case 0x86:
            break;
        }
    }
}

static HI_S32 CheckProbeStatus(struct HiUvcDev* hiDev)
{
    if ((hiDev->probeStatus.bGet == 1) && (hiDev->probeStatus.bSet == 1) &&
        (hiDev->probeStatus.bMin == 1) && (hiDev->probeStatus.bMax == 1))
    {
        return 1;
    }

    DEBUG("the probe status is not correct...\n");

    return 0;
}

static HI_VOID HiUvcHandleClassRequest(struct HiUvcDev* hiDev, struct usb_ctrlrequest* uCtrlReq,
                                     struct uvc_request_data* uReqData)
{
    HI_U8 u8ProbeStatus = 1;

    if (u8ProbeStatus)
    {
        HI_U8 u8Type = uCtrlReq->bRequestType & USB_RECIP_MASK;
        switch (u8Type)
        {
        case USB_RECIP_INTERFACE:
            DEBUG("reqeust u8Type :HI_S32ERFACE\n");
            DEBUG("HI_S32erface : %d\n", (uCtrlReq->wIndex &0xff));
            DEBUG("unit id : %d\n", ((uCtrlReq->wIndex & 0xff00)>>8));
            DEBUG("cs code : 0x%02x(%s)\n", (uCtrlReq->wValue >> 8), (HI_CHAR*)GetIntfCsS((uCtrlReq->wValue >> 8)));
            DEBUG("req code: 0x%02x(%s)\n", uCtrlReq->bRequest, (HI_CHAR*)GetCode(uCtrlReq->bRequest));

            SetProbeStatus(hiDev, (uCtrlReq->wValue >> 8), uCtrlReq->bRequest);
            break;
        case USB_RECIP_DEVICE:
            DEBUG("request type :DEVICE\n");
            break;
        case USB_RECIP_ENDPOINT:
            DEBUG("request type :ENDPOINT\n");
            break;
        case USB_RECIP_OTHER:
            DEBUG("request type :OTHER\n");
            break;
        }
    }

    if ((uCtrlReq->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
    {
        return;
    }

    hiDev->iControl = (uCtrlReq->wValue >> 8);
    hiDev->iUnitId = ((uCtrlReq->wIndex & 0xff00) >> 8);
    hiDev->iIntfId = (uCtrlReq->wIndex & 0xff);

    switch (uCtrlReq->wIndex & 0xff)
    {
    case HIUVC_INTF_CONTROL:
        HiUvcHandleControlRequest(hiDev, uCtrlReq->bRequest, uCtrlReq->wIndex >> 8, uCtrlReq->wValue >> 8, uReqData);
        break;
    case HIUVC_INTF_STREAMING:
        HiUvcHandleStreamingRequest(hiDev, uCtrlReq->bRequest, uCtrlReq->wValue >> 8, uReqData);
        break;
    default:
        break;
    }
}

static HI_VOID DoHiUvcSetupEvent(struct HiUvcDev* hiDev, struct usb_ctrlrequest* uCtrlReq,
                                     struct uvc_request_data* uReqData)
{
    hiDev->iControl = 0;
    hiDev->iUnitId = 0;
    hiDev->iIntfId = 0;

    switch (uCtrlReq->bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        HiUvcHandleStandardRequest(hiDev, uCtrlReq, uReqData);
        break;
    case USB_TYPE_CLASS:
        HiUvcHandleClassRequest(hiDev, uCtrlReq, uReqData);
        break;
    default:
        break;
    }
}

static HI_VOID HandleControlInterfaceData(struct HiUvcDev *hiDev, struct uvc_request_data *uReqData)
{
    switch (hiDev->iUnitId)
    {
        case HIUVC_VC_HEADER:
            histream_event_it_data(hiDev, hiDev->iUnitId, hiDev->iControl, uReqData);
            break;
        case HIUVC_VC_INPUT_TERMINAL:
            histream_event_pu_data(hiDev, hiDev->iUnitId, hiDev->iControl, uReqData);
            break;
        case UNIT_XU_H264:
            histream_event_eu_h264_data(hiDev, hiDev->iUnitId, hiDev->iControl, uReqData);
            break;
        default:
            break;
    }
}

static HI_VOID DoHiUvcDataEvent(struct HiUvcDev* hiDev, struct uvc_request_data* uReqData)
{
    struct HiUvcStreamingControl* uStrTarget;
    struct HiUvcStreamingControl* uStrCtrl;
    const struct HiUvcFormatInfo* hiFmtInfo;
    const struct HiUvcFrameInfo* hiFrmInfo;
    const HI_U32* u32Intval;
    HI_U32 u32IFmt, u32IFrm;
    HI_U32 u32NFrm;

    if ((hiDev->iUnitId != 0) && (hiDev->iIntfId == HIUVC_INTF_CONTROL))
    {
        return HandleControlInterfaceData(hiDev, uReqData);
    }

    switch (hiDev->iControl)
    {
        case HIUVC_VS_PROBE_CONTROL:
            DEBUG("setting probe control, length = %d\n", uReqData->length);
            uStrTarget = &hiDev->probe;
            break;
        case HIUVC_VS_COMMIT_CONTROL:
            DEBUG("setting commit control, length = %d\n", uReqData->length);
            uStrTarget = &hiDev->commit;
            break;
        default:
            DEBUG("setting unknown control, length = %d\n", uReqData->length);
            return;
    }

    uStrCtrl = (struct HiUvcStreamingControl*)&uReqData->data;

    DEBUG("uStrCtrl->bFormatIndex = %d\n", (HI_U32)uStrCtrl->bFormatIndex);

    u32IFmt = clamp((HI_U32)uStrCtrl->bFormatIndex, 1U,
                    (HI_U32)ARRAY_SIZE(hiFmt));

    DEBUG("set iformat = %d \n", u32IFmt);

    hiFmtInfo = &hiFmt[u32IFmt - 1];
    u32NFrm = 0;

    DEBUG("hiFmtInfo->frames[u32NFrm].width: %d\n", hiFmtInfo->frames[u32NFrm].width);
    DEBUG("hiFmtInfo->frames[u32NFrm].height: %d\n", hiFmtInfo->frames[u32NFrm].height);

    while (hiFmtInfo->frames[u32NFrm].width != 0)
    {
        ++u32NFrm;
    }

    u32IFrm = clamp((HI_U32)uStrCtrl->bFrameIndex, 1U, u32NFrm);
    hiFrmInfo = &hiFmtInfo->frames[u32IFrm - 1];
    u32Intval = hiFrmInfo->intervals;

    while ((u32Intval[0] < uStrCtrl->dwFrameInterval) && u32Intval[1])
    {
        ++u32Intval;
    }

    uStrTarget->bFormatIndex = u32IFmt;
    uStrTarget->bFrameIndex = u32IFrm;

    switch (hiFmtInfo->fcc)
    {
        case VIDEO_IMG_FORMAT_YUYV:
            uStrTarget->dwMaxVideoFrameSize = hiFrmInfo->width * hiFrmInfo->height * 2;
            break;
        case VIDEO_IMG_FORMAT_YUV420:
            uStrTarget->dwMaxVideoFrameSize = hiFrmInfo ->width * hiFrmInfo ->height * 1.5;
            break;
        case VIDEO_IMG_FORMAT_MJPEG:
        case VIDEO_IMG_FORMAT_H264:
            if (hiDev->dwImgSize == 0)
            {
                DEBUG("WARNING: MJPEG requested and no image loaded.\n");
            }

            uStrTarget->dwMaxVideoFrameSize = hiDev->dwImgSize;
            break;
    }

    uStrTarget->dwFrameInterval = *u32Intval;

    DEBUG("set u32Intval=%d hiFmtInfo=%d hiFrmInfo=%d\n",uStrTarget->dwFrameInterval,
         uStrTarget->bFormatIndex, uStrTarget->bFrameIndex);

    if ((hiDev->iControl == HIUVC_VS_COMMIT_CONTROL) && CheckProbeStatus(hiDev))
    {
        hiDev->dwFcc    = hiFmtInfo->fcc;
        hiDev->dwWidth  = hiFrmInfo->width;
        hiDev->dwHeight = hiFrmInfo->height;

        DEBUG("set device format=%s width=%d height=%d\n", ToString(hiDev->dwFcc), hiDev->dwWidth, hiDev->dwHeight);

        HiUvcVideoSetFmt(hiDev);

        if (hiDev->dwBulk != 0)
        {
            HiUvcVideoDisable(hiDev);
            HiUvcVideoEnable(hiDev);
        }
    }

    if (hiDev->iControl == HIUVC_VS_COMMIT_CONTROL)
    {
        memset(&hiDev->probeStatus, 0, sizeof (hiDev->probeStatus));
    }
}

static HI_VOID DoHiUvcEvent(struct HiUvcDev* hiDev)
{
    struct VideoEvent vEvent;
    struct HiUvcEvent* hiEve = (struct HiUvcEvent*)(HI_VOID*)&vEvent.u.aData;
    struct uvc_request_data uReqData;
    HI_S32 s32Ret;

    DEBUG("#############DoHiUvcEvent()#############\n");

    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DQUEUE_EVENT, &vEvent);
    if (s32Ret < 0)
    {
        ERR_INFO("VIDEO_IOCTL_DQUEUE_EVENT failed: %s (%d)\n", strerror(errno),
            errno);
        return;
    }

    memset(&uReqData, 0, sizeof uReqData);
    uReqData.length = 32;

    switch (vEvent.dwType)
    {
    case HIUVC_EVE_CON:
        DEBUG("handle connect event\n");
         HiUvcHandleStreamingControl(hiDev, &hiDev->probe, 0, 0);
         HiUvcHandleStreamingControl(hiDev, &hiDev->commit, 0, 0);
    case HIUVC_EVE_DISCON:
        DEBUG("handle disconnect event\n");
        return;
    case HIUVC_EVE_SETTING:
        DEBUG("handle setup event\n");
        DoHiUvcSetupEvent(hiDev, &hiEve->req, &uReqData);
        break;
    case HIUVC_EVE_DATA:
        DEBUG("handle data event\n");
        DoHiUvcDataEvent(hiDev, &hiEve->data);
        return;
    case HIUVC_EVE_STRON:
        DEBUG("HIUVC_EVE_STRON\n");
        if (!hiDev->dwBulk)
        {
            HiUvcVideoEnable(hiDev);
        }
        return;
    case HIUVC_EVE_STROFF:
        DEBUG("HIUVC_EVE_STROFF\n");
        if (!hiDev->dwBulk)
        {
            HiUvcVideoDisable(hiDev);
        }
        return;
    }

    s32Ret = ioctl(hiDev->iFd, HIUVC_IOC_SEND_RESPONSE, &uReqData);
    if (s32Ret < 0)
    {
        ERR_INFO("HIUVC_IOC_S_EVENT failed: %s (%d)\n", strerror(errno),
            errno);
        return;
    }
}

static HI_VOID HiUvcEventRegister(struct HiUvcDev* hiDev)
{
    struct VideoEventDescriptor vEventDesc;
    HI_S32 s32Ret;

    memset(&vEventDesc, 0, sizeof vEventDesc);

    vEventDesc.dwType = HIUVC_EVE_CON;
    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DESC_EVENT, &vEventDesc);
    if (s32Ret < 0)
    {
        ERR_INFO("Connect event failed: %s (%d)\n", strerror(errno),errno);
    }

    vEventDesc.dwType = HIUVC_EVE_SETTING;
    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DESC_EVENT, &vEventDesc);
    if (s32Ret < 0)
    {
        ERR_INFO("Setup event failed: %s (%d)\n", strerror(errno),errno);
    }

    vEventDesc.dwType = HIUVC_EVE_DATA;
    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DESC_EVENT, &vEventDesc);
    if (s32Ret < 0)
    {
        ERR_INFO("Data event failed: %s (%d)\n", strerror(errno),errno);
    }

    vEventDesc.dwType = HIUVC_EVE_STRON;
    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DESC_EVENT, &vEventDesc);
    if (s32Ret < 0)
    {
        ERR_INFO("StreamOn event failed: %s (%d)\n", strerror(errno),errno);
    }

    vEventDesc.dwType = HIUVC_EVE_STROFF;
    s32Ret = ioctl(hiDev->iFd, VIDEO_IOCTL_DESC_EVENT, &vEventDesc);
    if (s32Ret < 0)
    {
        ERR_INFO("StreamOff event failed: %s (%d)\n", strerror(errno),errno);
    }
}

HI_S32 open_uvc_device(const HI_CHAR *charDevPath)
{
    struct HiUvcDev* hiDev;

    HI_CHAR* charDev = (HI_CHAR*)charDevPath;

    hiDev = HiUvcInit(charDev);
    if (hiDev == 0)
    {
        return -1;
    }

    hiDev->dwImgSize = MAX_PAYLOAD_IMAGE_SIZE; /* 2160*3840*2 */

    DEBUG("set imagesize = %d, set bulkmode =%d, set bulksize = %d\n",
          hiDev->dwImgSize, hiDev->dwBulk, hiDev->dwBulkSize);

    HiUvcEventRegister(hiDev);
    HiCD = hiDev;

    return 0;
}

HI_S32 close_uvc_device(HI_VOID)
{
    if (HiCD != 0)
    {
        HiUvcVideoDisable(HiCD);
        close(HiCD->iFd);
        free(HiCD);
    }

    HiCD = 0;
    return 0;
}

HI_S32 run_uvc_data(HI_VOID)
{
    fd_set wFds;
    HI_S32 s32Ret;
    struct timeval tVal;

    if (!HiCD)
    {
        return -1;
    }

    tVal.tv_sec  = 1;
    tVal.tv_usec = 0;

    FD_ZERO(&wFds);

    if (HiCD->iStreaming == 1)
    {
        FD_SET(HiCD->iFd, &wFds);
    }

    s32Ret = select(HiCD->iFd + 1, NULL, &wFds, NULL, &tVal);
    if (s32Ret > 0)
    {
        if (FD_ISSET(HiCD->iFd, &wFds))
        {
            HiUvcVideoProcessUserptr(HiCD);
        }
    }

    return s32Ret;
}

HI_S32 run_uvc_device(HI_VOID)
{
    fd_set eFds;
    HI_S32 s32Ret;
    struct timeval tVal;

    if (!HiCD)
    {
        return -1;
    }

    tVal.tv_sec  = 1;
    tVal.tv_usec = 0;

    FD_ZERO(&eFds);
    FD_SET(HiCD->iFd, &eFds);

    s32Ret = select(HiCD->iFd + 1, NULL, NULL, &eFds, &tVal);
    if (s32Ret > 0)
    {
        if (FD_ISSET(HiCD->iFd, &eFds))
        {
            DoHiUvcEvent(HiCD);
        }

    }

    return s32Ret;
}
