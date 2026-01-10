/*
* Copyright (C) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
* Description: Hisilicon UVC gadget test application.
* Author: Liang Shengjun <liangshengjun@hisilicon.com>
*         Jianyed <yejian9@hisilicon.com>
* Create: 2019-06-08
*/

#ifndef __LINUX_HISILICON_USB_CAMERA_H
#define __LINUX_HISILICON_USB_CAMERA_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>

#include "hi_type.h"

#define HIUVC_VC_DESCRIPTOR_UNDEFINED 0x00
#define HIUVC_VC_HEADER 0x01
#define HIUVC_VC_INPUT_TERMINAL 0x02
#define HIUVC_VC_OUTPUT_TERMINAL 0x03
#define HIUVC_VC_SELECTOR_UNIT 0x04
#define HIUVC_VC_PROCESSING_UNIT 0x05
#define HIUVC_VC_EXTENSION_UNIT 0x06

#define HIUVC_RC_UNDEFINED 0x00
#define HIUVC_SET_CUR 0x01
#define HIUVC_GET_CUR 0x81
#define HIUVC_GET_MIN 0x82
#define HIUVC_GET_MAX 0x83
#define HIUVC_GET_RES 0x84
#define HIUVC_GET_LEN 0x85
#define HIUVC_GET_INFO 0x86
#define HIUVC_GET_DEF 0x87

#define HIUVC_VC_CONTROL_UNDEFINED 0x00
#define HIUVC_VC_VIDEO_POWER_MODE_CONTROL 0x01
#define HIUVC_VC_REQUEST_ERROR_CODE_CONTROL 0x02

#define HIUVC_CT_CONTROL_UNDEFINED 0x00
#define HIUVC_CT_SCANNING_MODE_CONTROL 0x01
#define HIUVC_CT_AE_MODE_CONTROL 0x02
#define HIUVC_CT_AE_PRIORITY_CONTROL 0x03
#define HIUVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define HIUVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05
#define HIUVC_CT_FOCUS_ABSOLUTE_CONTROL 0x06
#define HIUVC_CT_FOCUS_RELATIVE_CONTROL 0x07
#define HIUVC_CT_FOCUS_AUTO_CONTROL 0x08
#define HIUVC_CT_IRIS_ABSOLUTE_CONTROL 0x09
#define HIUVC_CT_IRIS_RELATIVE_CONTROL 0x0a
#define HIUVC_CT_ZOOM_ABSOLUTE_CONTROL 0x0b
#define HIUVC_CT_ZOOM_RELATIVE_CONTROL 0x0c
#define HIUVC_CT_PANTILT_ABSOLUTE_CONTROL 0x0d
#define HIUVC_CT_PANTILT_RELATIVE_CONTROL 0x0e
#define HIUVC_CT_ROLL_ABSOLUTE_CONTROL 0x0f
#define HIUVC_CT_ROLL_RELATIVE_CONTROL 0x10
#define HIUVC_CT_PRIVACY_CONTROL 0x11

#define HIUVC_PU_CONTROL_UNDEFINED 0x00
#define HIUVC_PU_BACKLIGHT_COMPENSATION_CONTROL 0x01
#define HIUVC_PU_BRIGHTNESS_CONTROL 0x02
#define HIUVC_PU_CONTRAST_CONTROL 0x03
#define HIUVC_PU_GAIN_CONTROL 0x04
#define HIUVC_PU_POWER_LINE_FREQUENCY_CONTROL 0x05
#define HIUVC_PU_HUE_CONTROL 0x06
#define HIUVC_PU_SATURATION_CONTROL 0x07
#define HIUVC_PU_SHARPNESS_CONTROL 0x08
#define HIUVC_PU_GAMMA_CONTROL 0x09
#define HIUVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0a
#define HIUVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0b
#define HIUVC_PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0c
#define HIUVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0d
#define HIUVC_PU_DIGITAL_MULTIPLIER_CONTROL 0x0e
#define HIUVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0f
#define HIUVC_PU_HUE_AUTO_CONTROL 0x10
#define HIUVC_PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11
#define HIUVC_PU_ANALOG_LOCK_STATUS_CONTROL 0x12

#define HIUVC_VS_CONTROL_UNDEFINED 0x00
#define HIUVC_VS_PROBE_CONTROL 0x01
#define HIUVC_VS_COMMIT_CONTROL 0x02
#define HIUVC_VS_STILL_PROBE_CONTROL 0x03
#define HIUVC_VS_STILL_COMMIT_CONTROL 0x04
#define HIUVC_VS_STILL_IMAGE_TRIGGER_CONTROL 0x05
#define HIUVC_VS_STREAM_ERROR_CODE_CONTROL 0x06
#define HIUVC_VS_GENERATE_KEY_FRAME_CONTROL 0x07
#define HIUVC_VS_UPDATE_FRAME_SEGMENT_CONTROL 0x08
#define HIUVC_VS_SYNC_DELAY_CONTROL 0x09

#define VIDEO_EVE_SECRET_BEGIN 0x08000000
#define HIUVC_EVE_BEGIN (VIDEO_EVE_SECRET_BEGIN + 0)
#define HIUVC_EVE_CON (VIDEO_EVE_SECRET_BEGIN + 0)
#define HIUVC_EVE_DISCON (VIDEO_EVE_SECRET_BEGIN + 1)
#define HIUVC_EVE_STRON (VIDEO_EVE_SECRET_BEGIN + 2)
#define HIUVC_EVE_STROFF (VIDEO_EVE_SECRET_BEGIN + 3)
#define HIUVC_EVE_SETTING (VIDEO_EVE_SECRET_BEGIN + 4)
#define HIUVC_EVE_DATA (VIDEO_EVE_SECRET_BEGIN + 5)
#define HIUVC_EVE_END (VIDEO_EVE_SECRET_BEGIN + 5)

#define VideoFourCharacterCode(a, b, c, d) \
   ((HI_U32)(a) | ((HI_U32)(b) << 8) | ((HI_U32)(c) << 16) | ((HI_U32)(d) << 24))

#define HIUVC_IOC_SEND_RESPONSE     _IOW('U', 1, struct uvc_request_data)

#define VIDEO_IOCTL_SET_FORMAT          _IOWR('V', 5, struct VideoFmt)
#define VIDEO_IOCTL_REQUEST_BUF         _IOWR('V', 8, struct VideoBufRequest)
#define VIDEO_IOCTL_QUERY_BUF           _IOWR('V', 9, struct VideoCache)
#define VIDEO_IOCTL_QUERY_CAP           _IOR('V', 0, struct VideoAbility)
#define VIDEO_IOCTL_STREAM_OFF          _IOW('V', 19, HI_S32)
#define VIDEO_IOCTL_QUEUE_BUF           _IOWR('V', 15, struct VideoCache)
#define VIDEO_IOCTL_SET_CMD             _IOWR('V', 28, struct video_control)
#define VIDEO_IOCTL_SET_DV_TIMINGS      _IOWR('V', 87, struct video_dv_timings)
#define VIDEO_IOCTL_GET_DV_TIMINGS      _IOWR('V', 88, struct video_dv_timings)
#define VIDEO_IOCTL_DQUEUE_EVENT        _IOR('V', 89, struct VideoEvent)
#define VIDEO_IOCTL_DESC_EVENT          _IOW('V', 90, struct VideoEventDescriptor)
#define VIDEO_IOCTL_UNDESC_EVENT        _IOW('V', 91, struct VideoEventDescriptor)
#define VIDEO_IOCTL_STREAM_ON           _IOW('V', 18, HI_S32)
#define VIDEO_IOCTL_DQUEUE_BUF          _IOWR('V', 17, struct VideoCache)

#define VIDEO_IMG_FORMAT_YUYV           VideoFourCharacterCode('Y', 'U', 'Y', 'V')  /* 16  YUV 4:2:2     */
#define VIDEO_IMG_FORMAT_YUV420         VideoFourCharacterCode('Y', 'U', '1', '2')  /* 16  YUV 4:2:0     */
#define VIDEO_IMG_FORMAT_NV12           VideoFourCharacterCode('N', 'V', '1', '2')  /* 16  YUV 4:2:0     */
#define VIDEO_IMG_FORMAT_MJPEG          VideoFourCharacterCode('M', 'J', 'P', 'G')  /* Motion-JPEG   */
#define VIDEO_IMG_FORMAT_JPEG           VideoFourCharacterCode('J', 'P', 'E', 'G')  /* JFIF JPEG     */
#define VIDEO_IMG_FORMAT_DV             VideoFourCharacterCode('d', 'v', 's', 'd')  /* 1394          */
#define VIDEO_IMG_FORMAT_MPEG           VideoFourCharacterCode('M', 'P', 'E', 'G')  /* MPEG-1/2/4 Multiplexed */
#define VIDEO_IMG_FORMAT_H264           VideoFourCharacterCode('H', '2', '6', '4')  /* H264 with start codes */
#define VIDEO_IMG_FORMAT_H264_NO_SC     VideoFourCharacterCode('A', 'V', 'C', '1')  /* H264 without start codes */
#define VIDEO_IMG_FORMAT_H264_MVC       VideoFourCharacterCode('M', '2', '6', '4')  /* H264 MVC */
#define VIDEO_IMG_FORMAT_H263           VideoFourCharacterCode('H', '2', '6', '3')  /* H263          */
#define VIDEO_IMG_FORMAT_MPEG1          VideoFourCharacterCode('M', 'P', 'G', '1')  /* MPEG-1 ES     */
#define VIDEO_IMG_FORMAT_MPEG2          VideoFourCharacterCode('M', 'P', 'G', '2')  /* MPEG-2 ES     */
#define VIDEO_IMG_FORMAT_MPEG4          VideoFourCharacterCode('M', 'P', 'G', '4')  /* MPEG-4 ES     */
#define VIDEO_IMG_FORMAT_XVID           VideoFourCharacterCode('X', 'V', 'I', 'D')  /* Xvid           */
#define VIDEO_IMG_FORMAT_VC1_ANNEX_G    VideoFourCharacterCode('V', 'C', '1', 'G')  /* SMPTE 421M Annex G compliant stream */
#define VIDEO_IMG_FORMAT_VC1_ANNEX_L    VideoFourCharacterCode('V', 'C', '1', 'L')  /* SMPTE 421M Annex L compliant stream */
#define VIDEO_IMG_FORMAT_VP8            VideoFourCharacterCode('V', 'P', '8', '0')  /* VP8 */

#define HIUVC_INTF_CONTROL 0
#define HIUVC_INTF_STREAMING 1

#define MAX_PAYLOAD_IMAGE_SIZE 16588800
#define VIDEO_MAX_FLATS 8
#define WAITED_NODE_SIZE (4)
#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(a[0])))
#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define clamp(val, min, max) ({                 \
                                  typeof(val)__val = (val);              \
                                  typeof(min)__min = (min);              \
                                  typeof(max)__max = (max);              \
                                  (void) (&__val == &__min);              \
                                  (void) (&__val == &__max);              \
                                  __val = __val < __min ? __min : __val;   \
                                  __val > __max ? __max : __val; })

#define __user

struct HiUvcStreamingControl
{
    HI_U16 bmHint;
    HI_U8  bFormatIndex;
    HI_U8  bFrameIndex;
    HI_U32 dwFrameInterval;
    HI_U16 wKeyFrameRate;
    HI_U16 wPFrameRate;
    HI_U16 wCompQuality;
    HI_U16 wCompWindowSize;
    HI_U16 wDelay;
    HI_U32 dwMaxVideoFrameSize;
    HI_U32 dwMaxPayloadTransferSize;
    HI_U32 dwClockFrequency;
    HI_U8  bmFramingInfo;
    HI_U8  bPreferedVersion;
    HI_U8  bMinVersion;
    HI_U8  bMaxVersion;
} __attribute__((__packed__));

struct uvc_request_data
{
    HI_S32 length;
    HI_U8  data[60];
};

struct HiUvcEvent
{
    union
    {
        struct usb_ctrlrequest  req;
        struct uvc_request_data data;
        enum usb_device_speed   speed;
    };
};

struct VideoEventDescriptor
{
    HI_U32 dwType;
    HI_U32 dwId;
    HI_U32 dwFlags;
    HI_U32 dwReserved[5];
};

struct VideoEventVsync
{
    HI_U8 byField;
} __attribute__ ((packed));

struct VideoEventCtrl
{
    HI_U32 dwChg;
    HI_U32 dwType;
    union
    {
        HI_S32 iVal;
        HI_S64 lVal64;
    };
    HI_U32 dwFlag;
    HI_S32 iMin;
    HI_S32 iMax;
    HI_S32 iStep;
    HI_S32 iDefVal;
};

struct VideoEventFrameSync
{
    HI_U32 dwIframe_seq;
};

struct VideoEvent
{
    HI_U32 dwType;
    union
    {
        HI_U8                       aData[64];
        struct VideoEventVsync      vSync;
        struct VideoEventFrameSync  frameSync;
        struct VideoEventCtrl       ctrl;
    } u;
    HI_U32           dwSequence;
    HI_U32           dwPending;
    struct timespec  timeStamp;
    HI_U32           dwId;
    HI_U32           dwReserved[8];
};

enum VideoBufKinds
{
    VIDEO_CACHE_TYPE_CAP = 1,
    VIDEO_CACHE_TYPE_OPT  = 2,
    VIDEO_CACHE_TYPE_ORLY = 3,
    VIDEO_CACHE_TYPE_VBI_CAP = 4,
    VIDEO_CACHE_TYPE_VBI_OPT = 5,
    VIDEO_CACHE_TYPE_SLD_VBI_CAP = 6,
    VIDEO_CACHE_TYPE_SLD_VBI_OPT = 7,

    /* Experimental */
    VIDEO_CACHE_TYPE_OPT_ORLY = 8,
    VIDEO_CACHE_TYPE_CAP_MLAN = 9,
    VIDEO_CACHE_TYPE_OTP_MLAN = 10,

    /* Deprecated, do not use */
    VIDEO_CACHE_TYPE_SECRET = 0x80,
};

enum VideoMem
{
    VIDEO_MM_MM = 1,
    VIDEO_MM_USER = 2,
    VIDEO_MM_ORLY = 3,
    VIDEO_MM_DMA = 4,
};

enum VideoDomain
{
    VIDEO_FLD_ALL = 0,              /* driver can choose from none,
                                    top, bottom, HI_S32erlaced
                                    depending on whatever it thinks
                                    is approximate ... */
    VIDEO_FLD_NOTHING = 1,          /* this device has no fields ... */
    VIDEO_FLD_ROOF = 2,             /* top field only */
    VIDEO_FLD_BASE = 3,             /* bottom field only */
    VIDEO_FLD_MULTI = 4,            /* both fields HI_S32erlaced */
    VIDEO_FLD_SEQ_TB = 5,           /* both fields sequential HI_S32o one
                                    buffer, top-bottom order */
    VIDEO_FLD_SEQ_BT = 6,           /* same as above + bottom-top order */
    VIDEO_FLD_ALTNT = 7,            /* both fields alternating HI_S32o
                                    separate buffers */
    VIDEO_FLD_MULTI_TB = 8,         /* both fields HI_S32erlaced, top field
                                    first and the top field is
                                    transmitted first */
    VIDEO_FLD_MULTI_BT = 9,         /* both fields HI_S32erlaced, top field
                                    first and the bottom field is
                                    transmitted first */
};

struct VideoAbility
{
    HI_U8  aDriver[16];
    HI_U8  aCard[32];
    HI_U8  aBusInfo[32];
    HI_U32 dwVer;
    HI_U32 dwCaps;
    HI_U32 dwDevCaps;
    HI_U32 dwReserved[3];
};

struct VideoTimeFmt
{
    HI_U32 dwType;
    HI_U32 dwFlag;
    HI_U8  bFrame;
    HI_U8  bSec;
    HI_U8  bMin;
    HI_U8  bHour;
    HI_U8  bUseu[4];
};

struct VideoCache
{
    HI_U32 dwIndex;
    HI_U32 dwType;
    HI_U32 dwUsed;
    HI_U32 dwFlags;
    HI_U32 dwFld;
    struct timeval      tivs;
    struct VideoTimeFmt fmt;
    HI_U32 dwSeq;

    /* memory location */
    HI_U32 dwMem;
    union
    {
        HI_S32 iFd;
        HI_U32 dwOffset;
        HI_UL ulUsr;
        struct video_flat* flats;
    } n;
    HI_U32 dwLen;
    HI_U32 dwReserved2;
    HI_U32 dwReserved;
};

struct VideoBufRequest
{
    HI_U32 dwCnt;
    HI_U32 dwType;          /* enum VideoBufKinds */
    HI_U32 dwMem;           /* enum VideoMem */
    HI_U32 dwReserved[2];
};

struct VideoImgFmt
{
    HI_U32 dwWidth;
    HI_U32 dwHeight;
    HI_U32 dwFmt;
    HI_U32 dwFld;           /* enum VideoDomain */
    HI_U32 dwEachline;      /* for padding, zero if unused */
    HI_U32 dwSize;
    HI_U32 dwColorspace;    /* enum v4l2_colorspace */
    HI_U32 dwSec;           /* private data, depends on pixelformat */
};

struct VideoFlatImgFmt
{
    HI_U32 dwSize;
    HI_U16 wEachline;
    HI_U16 wReserved[7];
} __attribute__ ((packed));

struct VideoImgFmtMflat
{
    HI_U32 dwWidth;
    HI_U32 dwHeight;
    HI_U32 dwFmt;
    HI_U32 dwFld;
    HI_U32 dwColorspace;

    struct VideoFlatImgFmt flatFmt[VIDEO_MAX_FLATS];
    HI_U8 bNum;
    HI_U8 bReserved[11];
} __attribute__ ((packed));

struct VideoShape
{
    HI_S32 iLeft;
    HI_S32 iTop;
    HI_S32 iWidth;
    HI_S32 iHeight;
};

struct VideoClip
{
    struct VideoShape h;
    struct VideoClip __user* next;
};

struct VideoWin
{
    struct VideoShape p;
    HI_U32 dwFld;          /* enum VideoDomain */
    HI_U32 dwChromakey;
    struct VideoClip __user* clips;
    HI_U32 dwClipicnt;
    void __user* bmp;
    HI_U8 bGlobalAlpha;
};

struct VideoVbiFmt
{
    HI_U32 dwSam_rate;          /* in 1 Hz */
    HI_U32 dwOffset;
    HI_U32 dwSamEachLine;
    HI_U32 dwSamFmt;            /* VIDEO_IMG_FORMAT_* */
    HI_S32 dwStart[2];
    HI_U32 dwCnt[2];
    HI_U32 dwFlag;              /* V4L2_VBI_* */
    HI_U32 dwReserved[2];       /* must be zero */
};

struct VideoPartVbiFmt
{
    HI_U16 wService;

    /* service_lines[0][...] specifies lines 0-23 (1-23 used) of the first field
       service_lines[1][...] specifies lines 0-23 (1-23 used) of the second field
       (equals frame lines 313-336 for 625 line video
       standards, 263-286 for 525 line standards) */
    HI_U16 wSrv_line[2][24];
    HI_U32 dwSize;
    HI_U32 dwReserved[2];            /* must be zero */
};

struct VideoFmt
{
    HI_U32 dwType;
    union
    {
        struct VideoImgFmt      pix;        /* VIDEO_CACHE_TYPE_CAP */
        struct VideoImgFmtMflat pixMp;      /* VIDEO_CACHE_TYPE_CAP_MLAN */
        struct VideoWin         win;        /* VIDEO_CACHE_TYPE_ORLY */
        struct VideoVbiFmt      vbi;        /* VIDEO_CACHE_TYPE_VBI_CAP */
        struct VideoPartVbiFmt  sliced;     /* VIDEO_CACHE_TYPE_SLD_VBI_CAP */
        HI_U8 bRaw_data[200];               /* user-defined */
    } fmt;
};

struct HiUvcPrb
{
    HI_U8 bSet;
    HI_U8 bGet;
    HI_U8 bMax;
    HI_U8 bMin;
};

struct HiUvcDev
{
    HI_S32 iFd;
    struct HiUvcStreamingControl probe;
    struct HiUvcStreamingControl commit;
    HI_S32 iControl;
    HI_S32 iUnitId;
    HI_S32 iIntfId;
    HI_U32 dwFcc;
    HI_U32 dwWidth;
    HI_U32 dwHeight;
    HI_U32 dwNbufs;
    HI_U32 dwBulk;
    HI_U8 bColor;
    HI_U32 dwImgSize;
    HI_U32 dwBulkSize;
    struct HiUvcPrb probeStatus;
    HI_S32 iStreaming;

    /* USB speed specific */
    HI_S32 mUlt;
    HI_S32 bUrst;
    HI_S32 mAxpkt;
    enum usb_device_speed speed;

    struct uvc_request_data request_error_code;
};

struct HiUvcFrameInfo
{
    HI_U32 width;
    HI_U32 height;
    HI_U32 intervals[8];
};

struct HiUvcFormatInfo
{
    HI_U32                 fcc;
    const struct HiUvcFrameInfo* frames;
};

static const struct HiUvcFrameInfo hiUvcFramesYuyv[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,       0 }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {     0,         }, },
};

static const struct HiUvcFrameInfo hiUvcFramesMjpeg[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,       0 }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {     0,         }, },
};

static const struct HiUvcFrameInfo hiUvcFramesH264[] =
{
    {  640,  360, {333333,       0 }, },
    { 1280,  720, {333333,       0 }, },
    { 1920, 1080, {333333,       0 }, },
    { 3840, 2160, {333333,       0 }, },
    {    0,    0, {     0,         }, },
};

static const struct HiUvcFormatInfo hiFmt[] =
{
    { VIDEO_IMG_FORMAT_YUYV,  hiUvcFramesYuyv  },
    { VIDEO_IMG_FORMAT_MJPEG, hiUvcFramesMjpeg },
    { VIDEO_IMG_FORMAT_H264,  hiUvcFramesH264  },
};

HI_S32 open_uvc_device(const HI_CHAR *charDevPath);
HI_S32 close_uvc_device(HI_VOID);
HI_S32 run_uvc_data(HI_VOID);
HI_S32 run_uvc_device(HI_VOID);
HI_VOID Nothing();

#endif /* __LINUX_HISILICON_USB_CAMERA_H */
