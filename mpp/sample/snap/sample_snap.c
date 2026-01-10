
#include "hi_type.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <linux/input.h>   // 新增：从内核input子系统读按键

#include "sample_comm.h"
#include "mpi_snap.h"

#define SAMPLE_SNAP_KEY_DEV      "/dev/input/event0"  // 按键所在的event设备
#define SAMPLE_SNAP_KEY_SNAP     KEY_F1               // 拍照键：按下拍一次
#define SAMPLE_SNAP_KEY_ENC_TOG  KEY_F2               // 编码开关键：按下切换开始/停止

// FPN 矫正全局状态，用于 Ctrl+C 信号清理
static HI_BOOL                         g_bSnapFpnEnable          = HI_FALSE;
static VI_PIPE                         g_SnapFpnPipe             = 0;
static SAMPLE_VI_FPN_CORRECTION_INFO_S g_stSnapFpnCorrectionInfo;
/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_SNAP_Usage(char* sPrgNm)
{
    printf("Usage : %s <index> \n", sPrgNm);
    printf("index:\n");
    printf("0) ov_9734.\n");
    printf("1) ov_6946.\n");

    return;
}


/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_SNAP_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        if (g_bSnapFpnEnable)
        {
            SAMPLE_COMM_VI_DisableFpnCorrection(g_SnapFpnPipe, &g_stSnapFpnCorrectionInfo);
            g_bSnapFpnEnable = HI_FALSE;
        }
        SAMPLE_COMM_All_ISP_Stop();
        SAMPLE_COMM_VO_HdmiStop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_S32 SAMPLE_SNAP_DoublePipeOffline_ov9734(HI_VOID)
{
    HI_S32                  s32Ret              = HI_SUCCESS;
    VI_DEV                  ViDev0              = 0;
    VI_PIPE                 VideoPipe           = 0;
    VI_PIPE                 SnapPipe            = 1;
    VI_CHN                  ViChn               = 0;
    HI_S32                  s32ViCnt            = 1;
    VPSS_GRP                VpssGrp0            = VideoPipe;
    VPSS_GRP                VpssGrp1            = SnapPipe;
    HI_S32                  s32WorkViIndex      = 0; 
    VPSS_CHN                VpssChn[4]          = {VPSS_CHN0, VPSS_CHN1, VPSS_CHN2, VPSS_CHN3};
    VPSS_GRP_ATTR_S         stVpssGrpAttr       = {0};
    VPSS_CHN_ATTR_S         stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM] = {0};
    HI_BOOL                 abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VO_CHN                  VoChn               = 0;
    PIC_SIZE_E              enPicSize           = PIC_3840x2160;
    WDR_MODE_E              enWDRMode           = WDR_MODE_NONE;
    DYNAMIC_RANGE_E         enDynamicRange      = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E          enPixFormat         = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E          enVideoFormat       = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E         enCompressMode      = COMPRESS_MODE_NONE;
    VI_VPSS_MODE_E          enVideoPipeMode     = VI_OFFLINE_VPSS_OFFLINE;
    VI_VPSS_MODE_E          enSnapPipeMode      = VI_OFFLINE_VPSS_OFFLINE;
    SIZE_S                  stSize;
    HI_U32                  u32BlkSize;
    VB_CONFIG_S             stVbConf;
    SAMPLE_VI_CONFIG_S      stViConfig;
    SAMPLE_VO_CONFIG_S      stVoConfig;
    HI_U32 u32SupplementConfig = VB_SUPPLEMENT_JPEG_MASK;
    VENC_CHN VencChn[2] = {0, 1};
    VPSS_GRP_NRX_PARAM_S stNrxParam;
    
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    /************************************************
    step 1:  Get all sensors information, need two vi
        ,and need two mipi --
    *************************************************/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);
    stViConfig.s32WorkingViNum                           = s32ViCnt;

    stViConfig.as32WorkingViId[s32WorkViIndex]           = 0;
    stViConfig.astViInfo[0].stSnsInfo.MipiDev            = SAMPLE_COMM_VI_GetComboDevBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, 0);

    stViConfig.astViInfo[0].stDevInfo.ViDev              = ViDev0;
    stViConfig.astViInfo[0].stDevInfo.enWDRMode          = enWDRMode;

    stViConfig.astViInfo[0].stPipeInfo.enMastPipeMode    = enVideoPipeMode;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[0]          = VideoPipe;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[1]          = SnapPipe;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[2]          = -1;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[3]          = -1;

    stViConfig.astViInfo[0].stChnInfo.ViChn              = ViChn;
    stViConfig.astViInfo[0].stChnInfo.enPixFormat        = enPixFormat;
    stViConfig.astViInfo[0].stChnInfo.enDynamicRange     = enDynamicRange;
    stViConfig.astViInfo[0].stChnInfo.enVideoFormat      = enVideoFormat;
    stViConfig.astViInfo[0].stChnInfo.enCompressMode     = enCompressMode;

    stViConfig.astViInfo[0].stSnapInfo.bSnap = HI_TRUE;
    stViConfig.astViInfo[0].stSnapInfo.bDoublePipe = HI_TRUE;
    stViConfig.astViInfo[0].stSnapInfo.VideoPipe = VideoPipe;
    stViConfig.astViInfo[0].stSnapInfo.SnapPipe = SnapPipe;
    stViConfig.astViInfo[0].stSnapInfo.enVideoPipeMode = enVideoPipeMode;
    stViConfig.astViInfo[0].stSnapInfo.enSnapPipeMode = enSnapPipeMode;

    /************************************************
    step 2:  Get  input size
    *************************************************/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed with %d!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /************************************************
    step3:  Init SYS and common VB
    *************************************************/
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 15;

    s32Ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&stVbConf, u32SupplementConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto EXIT;
    }

    s32Ret = SAMPLE_COMM_VI_SetParam(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed with %d!\n", s32Ret);
        goto EXIT;
    }


    /************************************************
    step 4: start VI
    *************************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        goto EXIT1;
    }

    /************************************************
    step 5: start VPSS, need two grp
    *************************************************/
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.enDynamicRange                 = enDynamicRange;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.bNrEn                          = HI_FALSE;
    stVpssGrpAttr.stNrAttr.enNrType              = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    abChnEnable[0]                               = HI_TRUE;
    stVpssChnAttr[0].u32Width                    = stSize.u32Width;
    stVpssChnAttr[0].u32Height                   = stSize.u32Height;
    stVpssChnAttr[0].enChnMode                   = VPSS_CHN_MODE_USER;
    stVpssChnAttr[0].enCompressMode              = enCompressMode;
    stVpssChnAttr[0].enDynamicRange              = enDynamicRange;
    stVpssChnAttr[0].enPixelFormat               = enPixFormat;
    stVpssChnAttr[0].enVideoFormat               = enVideoFormat;
    stVpssChnAttr[0].stFrameRate.s32SrcFrameRate = -1;
    stVpssChnAttr[0].stFrameRate.s32DstFrameRate = -1;
    stVpssChnAttr[0].u32Depth                    = 1;
    stVpssChnAttr[0].bMirror                     = HI_FALSE;
    stVpssChnAttr[0].bFlip                       = HI_FALSE;
    stVpssChnAttr[0].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp0, abChnEnable, &stVpssGrpAttr, stVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Start Grp0 failed with %d!\n", s32Ret);
        goto EXIT1;
    }

    stVpssGrpAttr.stNrAttr.enNrType = VPSS_NR_TYPE_SNAP;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp1, abChnEnable, &stVpssGrpAttr, stVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Start Grp1 failed with %d!\n", s32Ret);
        goto EXIT2;
    }

    memset(&stNrxParam, 0, sizeof(VPSS_GRP_NRX_PARAM_S));
    stNrxParam.enNRVer = VPSS_NR_V1;
    s32Ret = HI_MPI_VPSS_GetGrpNRXParam(VpssGrp0, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_GetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    stNrxParam.stNRXParam_V1.enOptMode = OPERATION_MODE_MANUAL;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[0].MATH = 900;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[1].MATH = 900;
    s32Ret = HI_MPI_VPSS_SetGrpNRXParam(VpssGrp0, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    memset(&stNrxParam, 0, sizeof(VPSS_GRP_NRX_PARAM_S));
    stNrxParam.enNRVer = VPSS_NR_V1;
    s32Ret = HI_MPI_VPSS_GetGrpNRXParam(VpssGrp1, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_GetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    stNrxParam.stNRXParam_V1.enOptMode = OPERATION_MODE_MANUAL;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[0].MATH = 900;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[1].MATH = 900;
    s32Ret = HI_MPI_VPSS_SetGrpNRXParam(VpssGrp1, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(VideoPipe, ViChn, VpssGrp0);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Bind_VPSS failed with %d!\n", s32Ret);
        goto EXIT3;
    }
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(SnapPipe, ViChn, VpssGrp1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Bind_VPSS failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    /************************************************
    step 6:  start VO
    *************************************************/
    s32Ret = SAMPLE_OV9734_COMM_VO_GetDefConfig(&stVoConfig); 
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_OV9734_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp0, VpssChn[0], stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VO Grp0 failed with %d!\n", s32Ret);
        goto EXIT4;
    }

    /************************************************
    step 7:  start VENC for VideoPipe
    *************************************************/
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_Start VideoPipe failed with %d!\n", s32Ret);
        goto EXIT4;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp0, VpssChn[0], VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VENC VideoPipe failed with %d!\n", s32Ret);
        goto EXIT5;
    }

    /************************************************
    step 8:  start VENC for SnapPipe
    *************************************************/
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn[1], &stSize, HI_TRUE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_SnapStart failed witfh %d\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp1, VpssChn[0], VencChn[1]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VENC failed with %d!\n", s32Ret);
        goto EXIT7;
    }

    /************************************************
    step 9:  配置 SNAP，进入按键循环
    *************************************************/
    SNAP_ATTR_S stSnapAttr;
    int keyFd;
    struct input_event stInputEvent;
    HI_BOOL bEncStart = HI_FALSE;

    stSnapAttr.enSnapType = SNAP_TYPE_NORMAL;
    stSnapAttr.bLoadCCM = HI_TRUE;
    stSnapAttr.stNormalAttr.u32FrameCnt = 1;
    stSnapAttr.stNormalAttr.u32RepeatSendTimes = 1;
    stSnapAttr.stNormalAttr.bZSL = 0;
    s32Ret = HI_MPI_SNAP_SetPipeAttr(SnapPipe, &stSnapAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_SNAP_SetPipeAttr failed with %#x!\n", s32Ret);
        goto EXIT9;
    }

    s32Ret = HI_MPI_SNAP_EnablePipe(SnapPipe);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_SNAP_EnablePipe failed with %#x!\n", s32Ret);
        goto EXIT9;
    }

    keyFd = open(SAMPLE_SNAP_KEY_DEV, O_RDONLY);
    if (keyFd < 0)
    {
        perror("open key input device failed");
        printf("please check SAMPLE_SNAP_KEY_DEV (%s)\n", SAMPLE_SNAP_KEY_DEV);
        goto EXIT10;
    }

    printf("======= use key(code=%d) to SNAP, key(code=%d) to TOGGLE ENCODE on %s =======\n",
           SAMPLE_SNAP_KEY_SNAP, SAMPLE_SNAP_KEY_ENC_TOG, SAMPLE_SNAP_KEY_DEV);
    printf("press Ctrl+C to exit program.\n");

    while (1)
    {
        ssize_t n = read(keyFd, &stInputEvent, sizeof(stInputEvent));
        if (n != (ssize_t)sizeof(stInputEvent))
        {
            continue;
        }

        if (stInputEvent.type != EV_KEY)
        {
            continue;
        }

        if (stInputEvent.value != 1)
        {
            continue;
        }

        if (stInputEvent.code == SAMPLE_SNAP_KEY_SNAP)
        {
            printf("snap key pressed, trigger snapshot...\n");

            s32Ret = HI_MPI_SNAP_TriggerPipe(SnapPipe);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_SNAP_TriggerPipe failed with %#x!\n", s32Ret);
                continue;
            }

            s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencChn[1],
                                                  stSnapAttr.stNormalAttr.u32FrameCnt,
                                                  HI_TRUE, HI_TRUE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: snap process failed!\n", __FUNCTION__);
                continue;
            }

            printf("snap success!\n");
        }
        else if (stInputEvent.code == SAMPLE_SNAP_KEY_ENC_TOG)
        {
            if (!bEncStart)
            {
                s32Ret = SAMPLE_COMM_VENC_StartGetStream(&VencChn[0], 1);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SAMPLE_COMM_VENC_StartGetStream VideoPipe failed with %d!\n", s32Ret);
                    continue;
                }
                bEncStart = HI_TRUE;
                printf("start video encoding stream.\n");
            }
            else
            {
                SAMPLE_COMM_VENC_StopGetStream();
                bEncStart = HI_FALSE;
                printf("stop video encoding stream.\n");
            }
        }
    }

EXIT10:
    HI_MPI_SNAP_DisablePipe(SnapPipe);
EXIT9:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT8:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp1, VpssChn[0], VencChn[1]);
EXIT7:
    SAMPLE_COMM_VENC_Stop(VencChn[1]);
EXIT6:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp0, VpssChn[0], VencChn[0]);
EXIT5:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT4:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp0, VpssChn[0], stVoConfig.VoDev, VoChn);
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT3:
    SAMPLE_COMM_VPSS_Stop(VpssGrp1, abChnEnable);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp0, abChnEnable);
    SAMPLE_COMM_VI_UnBind_VPSS(VideoPipe, ViChn, VpssGrp0);
    SAMPLE_COMM_VI_UnBind_VPSS(SnapPipe, ViChn, VpssGrp1);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

HI_S32 SAMPLE_SNAP_DoublePipeOffline_ov6946(HI_VOID)
{
    HI_S32                  s32Ret              = HI_SUCCESS;
    VI_DEV                  ViDev0              = 3;
    VI_PIPE                 VideoPipe           = 0;
    VI_PIPE                 SnapPipe            = 1;
    VI_CHN                  ViChn               = 0;
    HI_S32                  s32ViCnt            = 1;
    VPSS_GRP                VpssGrp0            = VideoPipe;
    VPSS_GRP                VpssGrp1            = SnapPipe;
    // 将工作传感器索引改为0，更直观
    HI_S32                  s32WorkViIndex      = 0;  
    VPSS_CHN                VpssChn[4]          = {VPSS_CHN0, VPSS_CHN1, VPSS_CHN2, VPSS_CHN3};
    VPSS_GRP_ATTR_S         stVpssGrpAttr       = {0};
    VPSS_CHN_ATTR_S         stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM] = {0};
    HI_BOOL                 abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VO_CHN                  VoChn               = 0;
    PIC_SIZE_E              enPicSize           = PIC_400P;  // OV6946应该是400P
    WDR_MODE_E              enWDRMode           = WDR_MODE_NONE;
    DYNAMIC_RANGE_E         enDynamicRange      = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E          enPixFormat         = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E          enVideoFormat       = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E         enCompressMode      = COMPRESS_MODE_NONE;
    VI_VPSS_MODE_E          enVideoPipeMode     = VI_OFFLINE_VPSS_OFFLINE;
    VI_VPSS_MODE_E          enSnapPipeMode      = VI_OFFLINE_VPSS_OFFLINE;
    SIZE_S                  stSize;
    HI_U32                  u32BlkSize;
    VB_CONFIG_S             stVbConf;
    SAMPLE_VI_CONFIG_S      stViConfig;
    SAMPLE_VO_CONFIG_S      stVoConfig;
    HI_U32 u32SupplementConfig = VB_SUPPLEMENT_JPEG_MASK;
    VENC_CHN VencChn[2] = {0, 1}; // 修改为数组，支持两个编码通道
    VPSS_GRP_NRX_PARAM_S stNrxParam;
    
    // 添加VENC相关参数
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    // FPN 校准/校正相关
    SAMPLE_VI_FPN_CALIBRATE_INFO_S   stViFpnCalibrateInfo;
    SAMPLE_VI_FPN_CORRECTION_INFO_S  stViFpnCorrectionInfo;
    HI_BOOL                          bFpnEnable = HI_FALSE;

    /************************************************
    step 1:  Get all sensors information, need two vi
        ,and need two mipi --
    *************************************************/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);
    stViConfig.s32WorkingViNum = s32ViCnt;

    // 设置工作VI索引
    stViConfig.as32WorkingViId[s32WorkViIndex] = 1;  // 使用传感器1
    
    // 设置MipiDev
    stViConfig.astViInfo[1].stSnsInfo.MipiDev = SAMPLE_COMM_VI_GetComboDevBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, 3);

    stViConfig.astViInfo[1].stDevInfo.ViDev              = ViDev0;
    stViConfig.astViInfo[1].stDevInfo.enWDRMode          = enWDRMode;

    stViConfig.astViInfo[1].stPipeInfo.enMastPipeMode    = enVideoPipeMode;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[0]          = VideoPipe;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[1]          = SnapPipe;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[2]          = -1;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[3]          = -1;

    stViConfig.astViInfo[1].stChnInfo.ViChn              = ViChn;
    stViConfig.astViInfo[1].stChnInfo.enPixFormat        = enPixFormat;
    stViConfig.astViInfo[1].stChnInfo.enDynamicRange     = enDynamicRange;
    stViConfig.astViInfo[1].stChnInfo.enVideoFormat      = enVideoFormat;
    stViConfig.astViInfo[1].stChnInfo.enCompressMode     = enCompressMode;

    stViConfig.astViInfo[1].stSnapInfo.bSnap = HI_TRUE;
    stViConfig.astViInfo[1].stSnapInfo.bDoublePipe = HI_TRUE;
    stViConfig.astViInfo[1].stSnapInfo.VideoPipe = VideoPipe;
    stViConfig.astViInfo[1].stSnapInfo.SnapPipe = SnapPipe;
    stViConfig.astViInfo[1].stSnapInfo.enVideoPipeMode = enVideoPipeMode;
    stViConfig.astViInfo[1].stSnapInfo.enSnapPipeMode = enSnapPipeMode;

    /************************************************
    step 2:  Get  input size
    *************************************************/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[1].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed with %d!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /************************************************
    step3:  Init SYS and common VB
    *************************************************/
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 15;

    s32Ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&stVbConf, u32SupplementConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto EXIT;
    }

    s32Ret = SAMPLE_COMM_VI_SetParam(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed with %d!\n", s32Ret);
        goto EXIT;
    }


    /************************************************
    step 4: start VI
    *************************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        goto EXIT1;
    }
/************************************************
    step 4.1: FPN calibrate & correction (VideoPipe)
    *************************************************/
    stViFpnCalibrateInfo.u32Threshold   = 4095;
    stViFpnCalibrateInfo.u32FrameNum    = 16;
    stViFpnCalibrateInfo.enFpnType      = ISP_FPN_TYPE_FRAME;
    stViFpnCalibrateInfo.enPixelFormat  = PIXEL_FORMAT_RGB_BAYER_16BPP;
    stViFpnCalibrateInfo.enCompressMode = COMPRESS_MODE_NONE;

    stViFpnCorrectionInfo.enOpType       = OP_TYPE_AUTO;
    stViFpnCorrectionInfo.enFpnType      = stViFpnCalibrateInfo.enFpnType;
    stViFpnCorrectionInfo.u32Strength    = 0;
    stViFpnCorrectionInfo.enPixelFormat  = stViFpnCalibrateInfo.enPixelFormat;
    stViFpnCorrectionInfo.enCompressMode = stViFpnCalibrateInfo.enCompressMode;

    s32Ret = SAMPLE_COMM_VI_FpnCorrectionConfig(VideoPipe, &stViFpnCorrectionInfo);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_FpnCorrectionConfig failed with %d!\n", s32Ret);
        goto EXIT1;
    }
    bFpnEnable = HI_TRUE;
    // 记录到全局变量，供 Ctrl+C 信号处理时使用
    g_bSnapFpnEnable = HI_TRUE;
    g_SnapFpnPipe    = VideoPipe;
    memcpy(&g_stSnapFpnCorrectionInfo, &stViFpnCorrectionInfo, sizeof(SAMPLE_VI_FPN_CORRECTION_INFO_S));
    /************************************************
    step 5: start VPSS, need two grp
    *************************************************/
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.enDynamicRange                 = enDynamicRange;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.bNrEn                          = HI_FALSE;
    stVpssGrpAttr.stNrAttr.enNrType              = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    abChnEnable[0]                               = HI_TRUE;
    stVpssChnAttr[0].u32Width                    = stSize.u32Width;
    stVpssChnAttr[0].u32Height                   = stSize.u32Height;
    stVpssChnAttr[0].enChnMode                   = VPSS_CHN_MODE_USER;
    stVpssChnAttr[0].enCompressMode              = enCompressMode;
    stVpssChnAttr[0].enDynamicRange              = enDynamicRange;
    stVpssChnAttr[0].enPixelFormat               = enPixFormat;
    stVpssChnAttr[0].enVideoFormat               = enVideoFormat;
    stVpssChnAttr[0].stFrameRate.s32SrcFrameRate = -1;
    stVpssChnAttr[0].stFrameRate.s32DstFrameRate = -1;
    stVpssChnAttr[0].u32Depth                    = 1;
    stVpssChnAttr[0].bMirror                     = HI_FALSE;
    stVpssChnAttr[0].bFlip                       = HI_FALSE;
    stVpssChnAttr[0].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp0, abChnEnable, &stVpssGrpAttr, stVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Start Grp0 failed with %d!\n", s32Ret);
        goto EXIT1;
    }

    stVpssGrpAttr.stNrAttr.enNrType = VPSS_NR_TYPE_SNAP;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp1, abChnEnable, &stVpssGrpAttr, stVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Start Grp1 failed with %d!\n", s32Ret);
        goto EXIT2;
    }

    memset(&stNrxParam, 0, sizeof(VPSS_GRP_NRX_PARAM_S));
    stNrxParam.enNRVer = VPSS_NR_V1;
    s32Ret = HI_MPI_VPSS_GetGrpNRXParam(VpssGrp0, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_GetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    stNrxParam.stNRXParam_V1.enOptMode = OPERATION_MODE_MANUAL;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[0].MATH = 900;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[1].MATH = 900;
    s32Ret = HI_MPI_VPSS_SetGrpNRXParam(VpssGrp0, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    memset(&stNrxParam, 0, sizeof(VPSS_GRP_NRX_PARAM_S));
    stNrxParam.enNRVer = VPSS_NR_V1;
    s32Ret = HI_MPI_VPSS_GetGrpNRXParam(VpssGrp1, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_GetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    stNrxParam.stNRXParam_V1.enOptMode = OPERATION_MODE_MANUAL;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[0].MATH = 900;
    stNrxParam.stNRXParam_V1.stNRXManual.stNRXParam.MDy[1].MATH = 900;
    s32Ret = HI_MPI_VPSS_SetGrpNRXParam(VpssGrp1, &stNrxParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetGrpNRXParam failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(VideoPipe, ViChn, VpssGrp0);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Bind_VPSS failed with %d!\n", s32Ret);
        goto EXIT3;
    }
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(SnapPipe, ViChn, VpssGrp1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Bind_VPSS failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    /************************************************
    step 6:  start VO
    *************************************************/
    s32Ret = SAMPLE_OV6946_COMM_VO_GetDefConfig(&stVoConfig); 
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_OV6946_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed with %d!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp0, VpssChn[0], stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VO Grp0 failed with %d!\n", s32Ret);
        goto EXIT4;
    }

    /************************************************
    step 7:  start VENC for VideoPipe (新增)
    *************************************************/
    // 配置GOP属性
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    
    // 启动VideoPipe的编码通道
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_Start VideoPipe failed with %d!\n", s32Ret);
        goto EXIT4;
    }

    // 绑定VideoPipe的VPSS和VENC
    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp0, VpssChn[0], VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VENC VideoPipe failed with %d!\n", s32Ret);
        goto EXIT5;
    }

    /************************************************
    step 8:  start VENC for SnapPipe (原有)
    *************************************************/
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn[1], &stSize, HI_TRUE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_SnapStart failed witfh %d\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp1, VpssChn[0], VencChn[1]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VPSS_Bind_VENC failed with %d!\n", s32Ret);
        goto EXIT7;
    }

    /************************************************
    step 10: 配置 SNAP，进入按键循环
    *************************************************/
    SNAP_ATTR_S stSnapAttr;
    int keyFd;
    struct input_event stInputEvent;
    HI_BOOL bEncStart = HI_FALSE;  // 编码是否已经开始

    stSnapAttr.enSnapType = SNAP_TYPE_NORMAL;
    stSnapAttr.bLoadCCM = HI_TRUE;
    stSnapAttr.stNormalAttr.u32FrameCnt = 1;
    stSnapAttr.stNormalAttr.u32RepeatSendTimes = 1;
    stSnapAttr.stNormalAttr.bZSL = 0;
    s32Ret = HI_MPI_SNAP_SetPipeAttr(SnapPipe, &stSnapAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_SNAP_SetPipeAttr failed with %#x!\n", s32Ret);
        goto EXIT9;
    }

    s32Ret = HI_MPI_SNAP_EnablePipe(SnapPipe);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_SNAP_EnablePipe failed with %#x!\n", s32Ret);
        goto EXIT9;
    }

    keyFd = open(SAMPLE_SNAP_KEY_DEV, O_RDONLY);
    if (keyFd < 0)
    {
        perror("open key input device failed");
        printf("please check SAMPLE_SNAP_KEY_DEV (%s)\n", SAMPLE_SNAP_KEY_DEV);
        goto EXIT10;
    }

    printf("======= use key(code=%d) to SNAP, key(code=%d) to TOGGLE ENCODE on %s =======\n",
           SAMPLE_SNAP_KEY_SNAP, SAMPLE_SNAP_KEY_ENC_TOG, SAMPLE_SNAP_KEY_DEV);
    printf("press Ctrl+C to exit program.\n");

    while (1)
    {
        ssize_t n = read(keyFd, &stInputEvent, sizeof(stInputEvent));
        if (n != (ssize_t)sizeof(stInputEvent))
        {
            continue;
        }

        if (stInputEvent.type != EV_KEY)
        {
            continue;
        }

        /* value: 1=press, 0=release, 2=autorepeat */
        if (stInputEvent.value != 1)
        {
            continue;
        }

        if (stInputEvent.code == SAMPLE_SNAP_KEY_SNAP)
        {
            /* 拍照键：每按一次，抓拍一次 */
            printf("snap key pressed, trigger snapshot...\n");

            s32Ret = HI_MPI_SNAP_TriggerPipe(SnapPipe);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_SNAP_TriggerPipe failed with %#x!\n", s32Ret);
                continue;
            }

            s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencChn[1],
                                                  stSnapAttr.stNormalAttr.u32FrameCnt,
                                                  HI_TRUE, HI_TRUE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("%s: snap process failed!\n", __FUNCTION__);
                continue;
            }

            printf("snap success!\n");
        }
        else if (stInputEvent.code == SAMPLE_SNAP_KEY_ENC_TOG)
        {
            /* 编码键：第一次按开始编码，再按一次停止编码 */
            if (!bEncStart)
            {
                s32Ret = SAMPLE_COMM_VENC_StartGetStream(&VencChn[0], 1);
                if (HI_SUCCESS != s32Ret)
                {
                    SAMPLE_PRT("SAMPLE_COMM_VENC_StartGetStream VideoPipe failed with %d!\n", s32Ret);
                    continue;
                }
                bEncStart = HI_TRUE;
                printf("start video encoding stream.\n");
            }
            else
            {
                SAMPLE_COMM_VENC_StopGetStream();
                bEncStart = HI_FALSE;
                printf("stop video encoding stream.\n");
            }
        }
    }

EXIT10:
    HI_MPI_SNAP_DisablePipe(SnapPipe);
EXIT9:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT8:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp1, VpssChn[0], VencChn[1]);
EXIT7:
    SAMPLE_COMM_VENC_Stop(VencChn[1]);
EXIT6:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp0, VpssChn[0], VencChn[0]);
EXIT5:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT4:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp0, VpssChn[0], stVoConfig.VoDev, VoChn);
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT3:
    SAMPLE_COMM_VPSS_Stop(VpssGrp1, abChnEnable);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp0, abChnEnable);
    SAMPLE_COMM_VI_UnBind_VPSS(VideoPipe, ViChn, VpssGrp0);
    SAMPLE_COMM_VI_UnBind_VPSS(SnapPipe, ViChn, VpssGrp1);
EXIT1:
    if (bFpnEnable)
    {
        SAMPLE_COMM_VI_DisableFpnCorrection(VideoPipe, &stViFpnCorrectionInfo);
        bFpnEnable       = HI_FALSE;
        g_bSnapFpnEnable = HI_FALSE;
    }
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char* argv[])
#endif
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s32Index;

    if (argc < 2)
    {
        SAMPLE_SNAP_Usage(argv[0]);
        return HI_FAILURE;
    }

#ifndef __HuaweiLite__
    signal(SIGINT, SAMPLE_SNAP_HandleSig);
    signal(SIGTERM, SAMPLE_SNAP_HandleSig);
#endif

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
        case 0:
            s32Ret = SAMPLE_SNAP_DoublePipeOffline_ov9734();
            break;
        case 1:
            s32Ret = SAMPLE_SNAP_DoublePipeOffline_ov6946();
            break;
        default:
            SAMPLE_PRT("the index %d is invaild!\n",s32Index);
            SAMPLE_SNAP_Usage(argv[0]);
            return HI_FAILURE;
    }

    if (HI_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("program exit normally!\n");
    }
    else
    {
        SAMPLE_PRT("program exit abnormally!\n");
    }

    return (s32Ret);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

