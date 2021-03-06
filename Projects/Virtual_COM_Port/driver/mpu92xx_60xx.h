#ifndef _MPU92XX_60XX_H
#define _MPU92XX_60XX_H

#include "mltypes.h"
#include "inv_mpu.h"

/* Starting sampling rate. */
//#define DEFAULT_MPU_HZ  (1000)
#define DEFAULT_MPU_HZ  (50)
#define COMPASS_READ_MS (20)


int inv_mpu_init(void);
void set_exit_int(void);
unsigned char get_inv_mpu(void);
void collect_proc(void);


#endif

