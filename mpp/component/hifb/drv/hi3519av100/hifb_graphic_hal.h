/*
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description:
 * Author: Hisilicon multimedia software group
 * Create: 2012-09-19
 */

#ifndef __HIFB_HAL_H__
#define __HIFB_HAL_H__

#include "hifb.h"
#include "hifb_reg.h"
#include "hifb_def.h"
#include "hifb_coef_org.h"
#include "hi_comm_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

HI_VOID HAL_VOU_Init(HI_VOID);
HI_VOID HAL_VOU_Exit(HI_VOID);

HI_VOID HAL_WriteReg(HI_U32 *pAddress, HI_U32 Value);
HI_U32 HAL_ReadReg(HI_U32 *pAddress);

/*****************************************************************************
 Prototype       : device Relative
*****************************************************************************/
HI_BOOL HAL_DISP_GetIntfEnable(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL *pbIntfEn);

HI_BOOL HAL_DISP_GetIntfSync(HAL_DISP_OUTPUTCHANNEL_E enChan, HAL_DISP_SYNCINFO_S *pstSyncInfo);
HI_BOOL HAL_DISP_GetDispIoP(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL *pbIop);
HI_BOOL HAL_DISP_GetIntfMuxSel(HAL_DISP_OUTPUTCHANNEL_E enChan, VO_INTF_TYPE_E *pbenIntfType);
HI_BOOL HAL_DISP_GetVtThdMode(HAL_DISP_OUTPUTCHANNEL_E enChan, HI_BOOL *pbFieldMode);

HI_BOOL HAL_DISP_SetIntMask(HI_U32 u32MaskEn);
HI_BOOL HAL_DISP_ClrIntMask(HI_U32 u32MaskEn);
HI_U32 HAL_DISP_GetIntStatus(HI_U32 u32IntMsk);
HI_BOOL HAL_DISP_ClearIntStatus(HI_U32 u32IntMsk);

/*****************************************************************************
 Prototype       : video layer Relative
*****************************************************************************/
HI_BOOL HAL_VIDEO_SetLayerDispRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT *pstRect);
HI_BOOL HAL_VIDEO_SetLayerVideoRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT *pstRect);

/*****************************************************************************
 Prototype       : layer Relative
*****************************************************************************/
/* Video layer CSC relative hal functions. */
HI_VOID HAL_LAYER_CSC_SetEnable(HAL_DISP_LAYER_E enLayer, HI_BOOL csc_en);
HI_VOID HAL_LAYER_CSC_SetCkGtEn(HAL_DISP_LAYER_E enLayer, HI_BOOL ck_gt_en);
HI_VOID HAL_LAYER_CSC_SetCoef(HAL_DISP_LAYER_E enLayer, VDP_CSC_COEF_S *pstCscCoef);
HI_VOID HAL_LAYER_CSC_SetDcCoef(HAL_DISP_LAYER_E enLayer, VDP_CSC_DC_COEF_S *pstCscDcCoef);

HI_VOID HAL_LINK_GetHcLink(HI_U32 *pu32Data);

HI_BOOL HAL_LAYER_EnableLayer(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable);
HI_BOOL HAL_LAYER_SetLayerDataFmt(HAL_DISP_LAYER_E enLayer,
                                  HAL_DISP_PIXEL_FORMAT_E enDataFmt);
HI_BOOL HAL_LAYER_GetLayerDataFmt(HAL_DISP_LAYER_E enLayer, HI_U32 *pu32Fmt);

HI_BOOL HAL_LAYER_SetCscCoef(HAL_DISP_LAYER_E enLayer, CscCoef_S *pstCscCoef);
HI_BOOL HAL_LAYER_SetCscEn(HAL_DISP_LAYER_E enLayer, HI_BOOL bCscEn);

HI_BOOL HAL_LAYER_SetSrcResolution(HAL_DISP_LAYER_E enLayer, HIFB_RECT *pstRect);
HI_BOOL HAL_LAYER_SetLayerInRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT *pstRect);
HI_BOOL HAL_LAYER_SetLayerOutRect(HAL_DISP_LAYER_E enLayer, HIFB_RECT *pstRect);
HI_BOOL HAL_LAYER_SetLayerGAlpha(HAL_DISP_LAYER_E enLayer,
                                 HI_U8 u8Alpha0);
HI_BOOL HAL_LAYER_GetLayerGAlpha(HAL_DISP_LAYER_E enLayer, HI_U8 *pu8Alpha0);

HI_BOOL HAL_LAYER_SetRegUp(HAL_DISP_LAYER_E enLayer);

/*****************************************************************************
 Prototype       : graphic layer Relative
*****************************************************************************/
HI_BOOL HAL_GRAPHIC_SetGfxAddr(HAL_DISP_LAYER_E enLayer, HI_U64 u64LAddr);
HI_BOOL HAL_GRAPHIC_GetGfxAddr(HAL_DISP_LAYER_E enLayer, HI_U64 *pu64GfxAddr);
HI_BOOL HAL_GRAPHIC_SetGfxStride(HAL_DISP_LAYER_E enLayer, HI_U16 u16pitch);
HI_BOOL HAL_GRAPHIC_GetGfxStride(HAL_DISP_LAYER_E enLayer, HI_U32 *pu32GfxStride);
HI_BOOL HAL_GRAPHIC_SetGfxExt(HAL_DISP_LAYER_E enLayer,
                              HAL_GFX_BITEXTEND_E enMode);
HI_BOOL HAL_GRAPHIC_SetGfxPreMult(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable);
HI_BOOL HAL_GRAPHIC_SetGfxPalpha(HAL_DISP_LAYER_E enLayer,
                                 HI_U32 bAlphaEn, HI_U32 bArange,
                                 HI_U8 u8Alpha0, HI_U8 u8Alpha1);
HI_BOOL HAL_GRAPHIC_GetGfxPalpha(HAL_DISP_LAYER_E enLayer, HI_U32 *pbAlphaEn,
                                 HI_U8 *pu8Alpha0, HI_U8 *pu8Alpha1);

HI_BOOL HAL_GRAPHIC_SetGfxKeyEn(HAL_DISP_LAYER_E enLayer, HI_U32 u32KeyEnable);
HI_BOOL HAL_GRAPHIC_SetGfxKeyMode(HAL_DISP_LAYER_E enLayer, HI_U32 u32KeyOut);
HI_BOOL HAL_GRAPHIC_SetColorKeyValue(HAL_DISP_LAYER_E enLayer,
                                     HAL_GFX_KEY_MAX_S stKeyMax, HAL_GFX_KEY_MIN_S stKeyMin);
HI_BOOL HAL_GRAPHIC_SetColorKeyMask(HAL_DISP_LAYER_E enLayer, HAL_GFX_MASK_S stMsk);

HI_BOOL HAL_GRAPHIC_GetGfxPreMult(HAL_DISP_LAYER_E enLayer, HI_U32 *pbEnable);

// for compress
HI_BOOL HAL_GRAPHIC_SetGfxDcmpAddr(HAL_DISP_LAYER_E enLayer, HI_U64 addr_AR);
HI_BOOL HAL_GRAPHIC_SetGfxDcmpEnable(HAL_DISP_LAYER_E enLayer, HI_U32 bEnable);
HI_BOOL HAL_GRAPHIC_GetGfxDcmpEnableState(HAL_DISP_LAYER_E enLayer, HI_BOOL *pbEnable);

HI_VOID HAL_MDDRC_GetStatus(HI_U32 *u32Status);
HI_VOID HAL_MDDRC_ClearStatus(HI_U32 u32Status);

/*****************************************************************************
 Begin           : cbm layer Relative
*****************************************************************************/
HI_BOOL HAL_CBM_SetCbmMixerPrio(HAL_DISP_LAYER_E enLayer, HI_U8 u8Prio, HI_U8 u8MixerId);

/***************************************************************************************************
*  Begin : Parameter Address distribute
***************************************************************************************************/

HI_VOID HAL_PARA_SetParaAddrVhdChn06(HI_U64 para_addr_vhd_chn06);

HI_VOID HAL_PARA_SetParaUpVhdChn(HI_U32 u32ChnNum);

/**********************************************************************************
*  Begin : Graphic layer DCMP relative hal functions.
**********************************************************************************/

HI_VOID HAL_FDR_GFX_SetDcmpEn(HI_U32 u32Data, HI_U32 dcmp_en);

HI_VOID HAL_FDR_GFX_SetSourceMode(HI_U32 u32Data, HI_U32 source_mode);
HI_VOID HAL_FDR_GFX_SetCmpMode(HI_U32 u32Data, HI_U32 cmp_mode);
HI_VOID HAL_FDR_GFX_SetIsLosslessA(HI_U32 u32Data, HI_U32 is_lossless_a);
HI_VOID HAL_FDR_GFX_SetIsLossless(HI_U32 u32Data, HI_U32 is_lossless);

/**********************************************************************************
*  Begin   : Graphic layer ZME relative hal functions.
**********************************************************************************/
HI_VOID HAL_G0_ZME_SetCkGtEn(HI_U32 ck_gt_en);
HI_VOID HAL_G0_ZME_SetOutWidth(HI_U32 out_width);
HI_VOID HAL_G0_ZME_SetHfirEn(HI_U32 hfir_en);
HI_VOID HAL_G0_ZME_SetAhfirMidEn(HI_U32 ahfir_mid_en);
HI_VOID HAL_G0_ZME_SetLhfirMidEn(HI_U32 lhfir_mid_en);
HI_VOID HAL_G0_ZME_SetChfirMidEn(HI_U32 chfir_mid_en);
HI_VOID HAL_G0_ZME_SetLhfirMode(HI_U32 lhfir_mode);
HI_VOID HAL_G0_ZME_SetAhfirMode(HI_U32 ahfir_mode);
HI_VOID HAL_G0_ZME_SetHfirOrder(HI_U32 hfir_order);
HI_VOID HAL_G0_ZME_SetHratio(HI_U32 hratio);
HI_VOID HAL_G0_ZME_SetLhfirOffset(HI_U32 lhfir_offset);
HI_VOID HAL_G0_ZME_SetChfirOffset(HI_U32 chfir_offset);
HI_VOID HAL_G0_ZME_SetOutPro(HI_U32 out_pro);
HI_VOID HAL_G0_ZME_SetOutHeight(HI_U32 out_height);
HI_VOID HAL_G0_ZME_SetVfirEn(HI_U32 vfir_en);
HI_VOID HAL_G0_ZME_SetAvfirMidEn(HI_U32 avfir_mid_en);
HI_VOID HAL_G0_ZME_SetLvfirMidEn(HI_U32 lvfir_mid_en);
HI_VOID HAL_G0_ZME_SetCvfirMidEn(HI_U32 cvfir_mid_en);
HI_VOID HAL_G0_ZME_SetLvfirMode(HI_U32 lvfir_mode);
HI_VOID HAL_G0_ZME_SetVafirMode(HI_U32 vafir_mode);
HI_VOID HAL_G0_ZME_SetVratio(HI_U32 vratio);
HI_VOID HAL_G0_ZME_SetVtpOffset(HI_U32 vtp_offset);
HI_VOID HAL_G0_ZME_SetVbtmOffset(HI_U32 vbtm_offset);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* End of __VOU_HAL_H__ */

