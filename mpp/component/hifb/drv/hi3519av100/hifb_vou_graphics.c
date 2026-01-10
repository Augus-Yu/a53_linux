/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description:
 * Author: Hisilicon multimedia software group
 * Create: 2012-09-19
 */

#include "hi_osal.h"
#include "proc_ext.h"

#include "hi_errno.h"
#include "hi_debug.h"
#include "hifb_vou_graphics.h"

typedef struct hiVOU_DEV_S {
    HI_BOOL bVoEnable; /* Device enable flag */
    VO_INTF_TYPE_E enIntfType; /* Device interface type */
    HI_U32 u32MaxWidth; /* Device resolution maximum width */
    HI_U32 u32MaxHeight; /* Device resolution maximum height */

} VOU_DEV_S;

unsigned int g_fifb_irq = VOU1_IRQ_NR;

VOU_DEV_S g_astVoDev[VO_MAX_DEV_NUM];

VO_GFXLAYER_CONTEXT_S g_astGfxLayerCtx[VO_MAX_GRAPHICS_LAYER_NUM];
HI_U32 g_u32DectectZone = 0; /* Record the DDR detection area used */

HI_VOID VOU_GetGfxDcmpPixel(VO_DISP_PIXEL_FORMAT_E enPixelFmt, HAL_DISP_PIXEL_FORMAT_E *penPixelFmt)
{
    switch (enPixelFmt) {
        case VO_INPUTFMT_ARGB_4444:
            *penPixelFmt = HAL_INPUTFMT_ARGB_4444;
            break;
        case VO_INPUTFMT_ARGB_1555:
            *penPixelFmt = HAL_INPUTFMT_ARGB_1555;
            break;
        case VO_INPUTFMT_ARGB_8888:
            *penPixelFmt = HAL_INPUTFMT_ARGB_8888;
            break;
        default:
            GRAPHICS_DRV_TRACE(HI_DBG_ERR, "pixel format(%d) is invalid!\n", enPixelFmt);
            break;
    }
}

HI_VOID VOU_GraphicsInitMDDRDetect(HI_VOID)
{
    return;
}

HI_BOOL VOU_GraphicsSetGfxKeyMode(GRAPHIC_LAYER u32Layer, HI_U32 u32KeyOut)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetGfxKeyMode(enLayer, u32KeyOut);
}

HI_BOOL VOU_GraphicsSetGfxExt(GRAPHIC_LAYER u32Layer,
                              VO_GFX_BITEXTEND_E enMode)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetGfxExt(enLayer, enMode);
}

HI_BOOL VOU_GraphicsSetGfxPalpha(GRAPHIC_LAYER u32Layer,
                                 HI_U32 bAlphaEn, HI_U32 bArange,
                                 HI_U8 u8Alpha0, HI_U8 u8Alpha1)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetGfxPalpha(enLayer, bAlphaEn, bArange,
                                    u8Alpha0, u8Alpha1);
}

HI_BOOL VOU_GraphicsSetLayerGalpha(GRAPHIC_LAYER u32Layer,
                                   HI_U8 u8Alpha0)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_LAYER_SetLayerGalpha(enLayer, u8Alpha0);
}

HI_BOOL VOU_GraphicsSetCscEn(GRAPHIC_LAYER u32Layer, HI_BOOL bCscEn)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_LAYER_SetCscEn(enLayer, bCscEn);
}

HI_BOOL VOU_GraphicsSetGfxAddr(GRAPHIC_LAYER u32Layer, HI_U64 u64LAddr)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetLayerAddr(enLayer, u64LAddr);
}

HI_BOOL VOU_GraphicsSetGfxStride(GRAPHIC_LAYER u32Layer, HI_U16 u16pitch)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetGfxStride(enLayer, u16pitch);
}

HI_BOOL VOU_GraphicsGetGfxPreMult(GRAPHIC_LAYER u32Layer, HI_U32 *pbEnable)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_GetGfxPreMult(enLayer, pbEnable);
}

HI_BOOL VOU_GraphicsSetGfxPreMult(GRAPHIC_LAYER u32Layer, HI_U32 bEnable)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetGfxPreMult(enLayer, bEnable);
}

HI_BOOL VOU_GraphicsSetLayerDataFmt(GRAPHIC_LAYER u32Layer,
                                    VO_DISP_PIXEL_FORMAT_E enDataFmt)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;
    HAL_DISP_PIXEL_FORMAT_E enPixFmt = HAL_INPUTFMT_ARGB_1555;

    VOU_GetGfxDcmpPixel(enDataFmt, &enPixFmt);

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetLayerDataFmt(enLayer, enPixFmt);
}

HI_BOOL VOU_GraphicsSetLayerInRect(GRAPHIC_LAYER u32Layer, HIFB_RECT *pstRect)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetLayerInRect(enLayer, pstRect);
}

HI_BOOL VOU_GraphicsSetLayerSrcImageReso(GRAPHIC_LAYER u32Layer, HIFB_RECT *pstRect)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetSrcImageResolution(enLayer, pstRect);
}

HI_BOOL VOU_GraphicsSetLayerOutRect(GRAPHIC_LAYER u32Layer, HIFB_RECT *pstRect)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetLayerOutRect(enLayer, pstRect);
}

HI_BOOL VOU_GraphicsSetColorKeyValue(GRAPHIC_LAYER u32Layer,
                                     VO_GFX_KEY_MAX_S stVoKeyMax, VO_GFX_KEY_MIN_S stVoKeyMin)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HAL_GFX_KEY_MAX_S stKeyMax;
    HAL_GFX_KEY_MIN_S stKeyMin;
    stKeyMax.u8KeyMax_R = stVoKeyMax.u8KeyMax_R;
    stKeyMax.u8KeyMax_G = stVoKeyMax.u8KeyMax_G;
    stKeyMax.u8KeyMax_B = stVoKeyMax.u8KeyMax_B;
    stKeyMin.u8KeyMin_R = stVoKeyMin.u8KeyMin_R;
    stKeyMin.u8KeyMin_G = stVoKeyMin.u8KeyMin_G;
    stKeyMin.u8KeyMin_B = stVoKeyMin.u8KeyMin_B;
    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetColorKeyValue(enLayer, stKeyMax, stKeyMin);
}

HI_BOOL VOU_GraphicsSetColorKeyMask(GRAPHIC_LAYER u32Layer, VO_GFX_MASK_S stVoMsk)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    HAL_GFX_MASK_S stMsk;
    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    stMsk.u8Mask_r = stVoMsk.u8Mask_r;
    stMsk.u8Mask_g = stVoMsk.u8Mask_g;
    stMsk.u8Mask_b = stVoMsk.u8Mask_b;
    return GRAPHIC_DRV_SetColorKeyMask(enLayer, stMsk);
}

HI_BOOL VOU_GraphicsSetGfxKeyEn(GRAPHIC_LAYER u32Layer, HI_U32 u32KeyEnable)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetGfxKeyEn(enLayer, u32KeyEnable);
}

HI_BOOL VOU_GraphicsSetRegUp(GRAPHIC_LAYER u32Layer)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_SetRegUp(enLayer);
}

HI_S32 VOU_GraphicsInit(HI_VOID)
{
    HI_S32 s32ret;
    VO_DEV VoDev = VO_DEV_DHD0;
    HAL_DISP_SYNCINFO_S stSyncInfo = {0};
    HI_BOOL bRet = HI_FALSE;
    HI_BOOL bVoEnable = HI_FALSE;
    VO_INTF_TYPE_E enIntfType = HAL_DISP_INTF_BT1120;
    HI_S32 i = 0;

    s32ret = GRAPHIC_DRV_Init();

    for (i = 0; i < VO_MAX_PHY_DEV_NUM; i++) {
        VoDev = i;
        bRet = GRAPHIC_DRV_GetDevEnable(VoDev, &bVoEnable);
        if (bRet == HI_FALSE) {
            continue;
        } else {
            g_astVoDev[VoDev].bVoEnable = bVoEnable;
        }

        bRet = GRAPHIC_DRV_GetIntfSync(VoDev, &stSyncInfo);
        if (bRet == HI_FALSE) {
            continue;
        } else {
            g_astVoDev[VoDev].u32MaxWidth = stSyncInfo.u16Hact;
            g_astVoDev[VoDev].u32MaxHeight = (stSyncInfo.bIop) ? stSyncInfo.u16Vact : stSyncInfo.u16Vact * 2;
        }

        bRet = GRAPHIC_DRV_GetIntfMuxSel(VoDev, &enIntfType);
        if (bRet == HI_FALSE) {
            continue;
        } else {
            g_astVoDev[VoDev].enIntfType = enIntfType;
        }
    }

    return s32ret;
}

HI_S32 VOU_GraphicsDeInit(HI_VOID)
{
    return GRAPHIC_DRV_Exit();
}

HI_BOOL VOU_GraphicsGetLayerGalpha(GRAPHIC_LAYER u32Layer, HI_U8 *pu8Alpha0)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_GetLayerGalpha(enLayer, pu8Alpha0);
}

HI_BOOL VOU_GraphicsGetLayerDataFmt(GRAPHIC_LAYER u32Layer, HI_U32 *pu32Fmt)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_GetLayerDataFmt(enLayer, pu32Fmt);
}

HI_BOOL VOU_GraphicsGetGfxAddr(GRAPHIC_LAYER u32Layer, HI_U64 *pu64GfxAddr)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_GetGfxAddr(enLayer, pu64GfxAddr);
}

HI_BOOL VOU_GraphicsGetGfxStride(GRAPHIC_LAYER u32Layer, HI_U32 *pu32GfxStride)
{
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    return GRAPHIC_DRV_GetGfxStride(enLayer, pu32GfxStride);
}

HI_S32 VOU_GraphicsGetDevMode(HAL_DISP_LAYER_E u32Layer, VOU_SCAN_MODE_E *pScanMode, HI_BOOL *pbFeildUpdate)
{
    VO_DEV VoDev;
    HI_U32 u32LayerIndex;
    GFX_SPIN_LOCK_FLAG lockFlag;
    VO_GFXLAYER_CONTEXT_S *pstVoGfxLayerCtx = NULL;
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    pstVoGfxLayerCtx = &g_astGfxLayerCtx[u32LayerIndex];

    GFX_SPIN_LOCK_IRQSAVE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    if (!pstVoGfxLayerCtx->bBinded) {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d has not been binded!\n", u32LayerIndex);
        return HI_ERR_VO_GFX_NOT_BIND;
    }
    VoDev = pstVoGfxLayerCtx->s32BindedDev;

    if (!g_astVoDev[VoDev].bVoEnable) {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "vodev %d for graphics layer %d has been disable!\n",
                            VoDev, u32LayerIndex);
        return HI_ERR_VO_DEV_NOT_ENABLE;
    }

    if (GRAPHIC_DRV_GetScanMode(VoDev, (HI_BOOL *)pScanMode) == HI_FALSE) {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "get vodev:%d scan mode failed!\n", VoDev);
        return HI_FAILURE;
    }

    if (GRAPHIC_DRV_GetVtThdMode(VoDev, pbFeildUpdate) == HI_FALSE) {
        GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "get vodev:%d scan mode failed!\n", VoDev);
        return HI_FAILURE;
    }
    GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    return HI_SUCCESS;
}

HI_S32 VOU_GraphicsResourceInit(HI_VOID)
{
    HI_S32 s32Ret;

    s32Ret = GRAPHIC_DRV_Resource_Init();
    if (s32Ret != HI_SUCCESS) {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphic drv resource init failed.\n");
        return s32Ret;
    }

    return s32Ret;
}

HI_S32 VOU_GraphicsResourceDeInit(HI_VOID)
{
    HI_S32 s32Ret;
    s32Ret = GRAPHIC_DRV_Resource_Exit();
    return s32Ret;
}

HI_S32 VOU_GraphicsEnableLayer(GRAPHIC_LAYER u32Layer, HI_BOOL bEnable)
{
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);
    if ((gfxLayer < LAYER_GFX_START) || (gfxLayer > LAYER_GFX_END)) {
        return HI_ERR_VO_GFX_INVALID_ID;
    }
    return GRAPHIC_DRV_EnableLayer(gfxLayer, bEnable);
}

HI_S32 VOU_GraphicsSetCallback(GRAPHIC_LAYER u32Layer, VO_FB_INT_TYPE_E enType,
                               VO_FB_IntCallBack pCallBack, HI_VOID *pArg)
{
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    GFX_SPIN_LOCK_FLAG lockFlag;
    HI_U32 u32LayerIndex;
    HI_S32 s32Ret = HI_SUCCESS;
    VO_GFXLAYER_CONTEXT_S *pstVoGfxLayerCtx = NULL;

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    pstVoGfxLayerCtx = &g_astGfxLayerCtx[u32LayerIndex];

    GFX_SPIN_LOCK_IRQSAVE(&pstVoGfxLayerCtx->spinLock, &lockFlag);
    switch (enType) {
        case HIFB_INTTYPE_VO: {
            pstVoGfxLayerCtx->pfVoCallBack = (FB_IntCallBack)pCallBack;
            pstVoGfxLayerCtx->pVoCallBackArg = pArg;
            break;
        }
        case HIFB_INTTYPE_WBC: {
            pstVoGfxLayerCtx->pfWbcCallBack = (FB_IntCallBack)pCallBack;
            pstVoGfxLayerCtx->pWbcCallBackArg = pArg;
            break;
        }
        default:
        {
            s32Ret = HI_FAILURE;
            break;
        }
    }
    GFX_SPIN_UNLOCK_IRQRESTORE(&pstVoGfxLayerCtx->spinLock, &lockFlag);

    return s32Ret;
}

HI_BOOL VOU_GraphicsEnableDcmp(GRAPHIC_LAYER u32Layer, HI_BOOL bEnable)
{
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);
    return GRAPHIC_DRV_EnableDcmp(gfxLayer, bEnable);
}

HI_BOOL VOU_GraphicsGetDcmpEnableState(GRAPHIC_LAYER u32Layer, HI_BOOL *pbEnable)
{
    HI_S32 s32Ret;
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    s32Ret = GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (s32Ret != HI_SUCCESS) {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d is illegal!\n", u32Layer);
        return HI_FAILURE;
    }
    return GRAPHIC_DRV_GetDcmpEnableState(gfxLayer, pbEnable);
}

HI_BOOL VOU_GraphicsSetDcmpInfo(GRAPHIC_LAYER u32Layer, VO_GRAPHIC_DCMP_INFO_S *pstDcmpInfo)
{
    GRAPHIC_DCMP_INFO_S stDcmpInfo = {0};
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);
    VOU_GetGfxDcmpPixel(pstDcmpInfo->enPixelFmt, &stDcmpInfo.enPixelFmt);
    stDcmpInfo.IsLossless = pstDcmpInfo->IsLossless;
    stDcmpInfo.IsLosslessA = pstDcmpInfo->IsLosslessA;
    stDcmpInfo.u64AR_PhyAddr = pstDcmpInfo->u64AR_PhyAddr;
    stDcmpInfo.u32Frame_Size0 = pstDcmpInfo->u32Frame_Size0;  // 19A no use
    stDcmpInfo.u32Frame_Size1 = pstDcmpInfo->u32Frame_Size1;  // 19A no user
    stDcmpInfo.u64GB_PhyAddr = pstDcmpInfo->u64GB_PhyAddr;  // 19A do not need this addr
    stDcmpInfo.u32Height = pstDcmpInfo->u32Height;
    stDcmpInfo.u32Offset = pstDcmpInfo->u32Offset;
    stDcmpInfo.u32Stride = pstDcmpInfo->u32Stride;
    stDcmpInfo.u32Width = pstDcmpInfo->u32Width;
    return GRAPHIC_DRV_SetDcmpInfo(enLayer, &stDcmpInfo);
}

HI_BOOL VOU_GraphicsEnableGHDR(GRAPHIC_LAYER u32Layer, HI_BOOL bEnable)
{
    return HI_FALSE;
}

HI_BOOL VOU_GraphicsEnableZME(GRAPHIC_LAYER u32Layer, RECT_S *stInRect, RECT_S *stOutRect, HI_BOOL bEnable)
{
    GF_G0_ZME_CFG_S st_zme_cfg;
    HAL_DISP_LAYER_E enLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &enLayer);

    st_zme_cfg.in_width = stInRect->u32Width;
    st_zme_cfg.in_height = stInRect->u32Height;
    st_zme_cfg.out_width = stOutRect->u32Width;
    st_zme_cfg.out_height = stOutRect->u32Height;

    return GRAPHIC_DRV_EnableZME(enLayer, &st_zme_cfg, bEnable);
}

HI_S32 VOU_GraphicsGetIntfSize(GRAPHIC_LAYER u32Layer, HI_U32 *pu32Width, HI_U32 *pu32Height)
{
    VO_DEV VoDev;
    HI_U32 u32LayerIndex;
    VO_GFXLAYER_CONTEXT_S *pstVoGfxLayerCtx = NULL;
    VOU_DEV_S *pstVoDev = NULL;
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;
    HI_BOOL bRet;
    HAL_DISP_SYNCINFO_S stSyncInfo = {0};

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);
    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    pstVoGfxLayerCtx = &g_astGfxLayerCtx[u32LayerIndex];

    if (!pstVoGfxLayerCtx->bBinded) {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "Graphics layer %d# has not been binded!\n", u32LayerIndex);
        return HI_FAILURE;
    }

    VoDev = pstVoGfxLayerCtx->s32BindedDev;
    pstVoDev = &g_astVoDev[VoDev];

    if (!pstVoDev->bVoEnable) {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "The vo device (%d) for graphics layer %d has been disable!\n",
                            VoDev, u32LayerIndex);
        return HI_FAILURE;
    }

    /* Be sure to read from the register, otherwise the width and height of
    the hifb context will not be updated when the vo timing changes. */
    bRet = GRAPHIC_DRV_GetIntfSync(VoDev, &stSyncInfo);
    if (bRet == HI_FALSE) {
        // keep the old value.
    } else {
        g_astVoDev[VoDev].u32MaxWidth = stSyncInfo.u16Hact;
        g_astVoDev[VoDev].u32MaxHeight = (stSyncInfo.bIop) ? stSyncInfo.u16Vact : stSyncInfo.u16Vact * 2;
    }

    *pu32Width = pstVoDev->u32MaxWidth;
    *pu32Height = pstVoDev->u32MaxHeight;

    return HI_SUCCESS;
}

HI_S32 VOU_GraphicsGetIntfType(GRAPHIC_LAYER u32Layer, VO_INTF_TYPE_E *penIntfType)
{
    VO_DEV VoDev;
    HI_U32 u32LayerIndex;
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);

    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }
    VoDev = g_astGfxLayerCtx[u32LayerIndex].s32BindedDev;
    *penIntfType = g_astVoDev[VoDev].enIntfType;

    return HI_SUCCESS;
}

HI_S32 VOU_GraphicsSetCscCoef(GRAPHIC_LAYER u32Layer)
{
    HI_U32 u32LayerIndex;
    HAL_DISP_LAYER_E gfxLayer = HAL_DISP_LAYER_BUTT;

    GRAPHIC_DRV_GetLayerID(u32Layer, &gfxLayer);

    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    return GRAPHIC_DRV_SetCscCoef(gfxLayer, &g_astGfxLayerCtx[u32LayerIndex].stGfxCsc,
                                  &g_astGfxLayerCtx[u32LayerIndex].stCscCoefParam);
}

/* check the bind is exist or not when the first open */
HI_S32 VOU_GraphicsOpenLayer(HAL_DISP_LAYER_E gfxLayer)
{
    VO_DEV VoDev;
    GFX_SPIN_LOCK_FLAG lockFlag = 0;
    HI_U32 u32LayerIndex;

    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    GFX_SPIN_LOCK_IRQSAVE(&g_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    if (!g_astGfxLayerCtx[u32LayerIndex].bBinded) {
        GFX_SPIN_UNLOCK_IRQRESTORE(&g_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d has not been binded!\n", u32LayerIndex);
        return HI_ERR_VO_GFX_NOT_BIND;
    }

    VoDev = g_astGfxLayerCtx[u32LayerIndex].s32BindedDev;

    if (!g_astVoDev[VoDev].bVoEnable) {
        GFX_SPIN_UNLOCK_IRQRESTORE(&g_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "vodev %d for graphics layer %d has not been enable!\n",
                            VoDev, u32LayerIndex);
        return HI_ERR_VO_DEV_NOT_ENABLE;
    }
    g_astGfxLayerCtx[u32LayerIndex].bOpened = HI_TRUE;
    GFX_SPIN_UNLOCK_IRQRESTORE(&g_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    return HI_SUCCESS;
}

HI_S32 VOU_GraphicsCloseLayer(HAL_DISP_LAYER_E gfxLayer)
{
    GFX_SPIN_LOCK_FLAG lockFlag;
    HI_U32 u32LayerIndex;

    if (GRAPHIC_DRV_GetLayerIndex(gfxLayer, &u32LayerIndex) != HI_SUCCESS) {
        GRAPHICS_DRV_TRACE(HI_DBG_ERR, "gfxLayer(%u) is invalid!\n", (HI_U32)gfxLayer);
        return HI_ERR_VO_GFX_INVALID_ID;
    }

    GFX_SPIN_LOCK_IRQSAVE(&g_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);
    g_astGfxLayerCtx[u32LayerIndex].bOpened = HI_FALSE;
    GFX_SPIN_UNLOCK_IRQRESTORE(&g_astGfxLayerCtx[u32LayerIndex].spinLock, &lockFlag);

    return HI_SUCCESS;
}

HI_S32 VOU_GraphicsEnableInt(HI_U32 u32LayerIndex, HI_BOOL bEnable)
{
    VO_GFXLAYER_CONTEXT_S *pstVoGfxLayerCtx = NULL;
    VOU_DEV_S *pstVoDev = NULL;
    VO_DEV VoDev;
    pstVoGfxLayerCtx = &g_astGfxLayerCtx[u32LayerIndex];

    if (!pstVoGfxLayerCtx->bBinded) {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "graphics layer %d has not been binded!\n", u32LayerIndex);
        return HI_ERR_VO_GFX_NOT_BIND;
    }
    VoDev = pstVoGfxLayerCtx->s32BindedDev;
    pstVoDev = &g_astVoDev[VoDev];

    if (pstVoDev->bVoEnable == HI_FALSE) {
        HIFB_GRAPHICS_TRACE(HI_DBG_ERR, "Open Int Error:The vo device (%d) for graphics layer %d has been disable!\n",
                            VoDev, u32LayerIndex);
        return HI_FAILURE;
    }

    GRAPHIC_DRV_DevIntEnable(VoDev, bEnable);
    return HI_SUCCESS;
}

HI_BOOL VOU_GraphicsClearInt(HI_U32 u32IntClear, HI_S32 s32Irq)
{
    GRAPHIC_DRV_IntClear(u32IntClear, s32Irq);
    return HI_SUCCESS;
}

HI_BOOL VOU_GraphicsGetInt(HI_U32 *pu32IntStaus)
{
    *pu32IntStaus = GRAPHIC_DRV_IntGetStatus();
    return HI_SUCCESS;
}

HI_BOOL VOU_GraphicsClearIntStatus(HI_U32 u32IntStatus)
{
    GRAPHIC_DRV_ClrIntStatus(u32IntStatus);
    return HI_SUCCESS;
}

HI_S32 VOU_GraphicsGetInterruptDev(HI_U32 IntStatus, VO_DEV *pVoDev)
{
    return GRAPHIC_DRV_GetInterruptDev(IntStatus, pVoDev);
}

