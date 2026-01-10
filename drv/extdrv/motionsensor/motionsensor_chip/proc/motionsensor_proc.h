#ifndef __MOTIONSENSOR_PROC_H__
#define __MOTIONSENSOR_PROC_H__
#include "motionsensor_ext.h"
#include "hi_comm_motionsensor.h"

#include "hi_type.h"

#define MAX_LEN (32)


//extern hi_msensor_param*         MotionSensorStatus;


hi_s32 MPU_PROC_Init(void);
void MPU_PROC_Exit(void);
//extern hi_s32 HI_BMM150_GetProcInfo(void);



#endif

