#include "mpu92xx_60xx.h"
#include "mltypes.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "eMPL_outputs.h"
#include "invensense.h"
#include "invensense_adv.h"
#include "i2c.h"
#include "log.h"

void run_self_test(void);

unsigned char *mpl_key = (unsigned char*)"eMPL 5.1";

struct platform_data_s {
    signed char orientation[9];
};

/* The sensors can be mounted onto the board in any orientation. The mounting
 * matrix seen below tells the MPL how to rotate the raw data from the
 * driver(s).
 * TODO: The following matrices refer to the configuration on internal test
 * boards at Invensense. If needed, please modify the matrices to match the
 * chip-to-body matrix for your particular set up.
 */
static struct platform_data_s gyro_pdata = {
    .orientation = { 1, 0, 0,
                     0, 1, 0,
                     0, 0, 1}
};

#if defined MPU9150 || defined MPU9250
static struct platform_data_s compass_pdata = {
    .orientation = { 0, 1, 0,
                     1, 0, 0,
                     0, 0, -1}
};
#define COMPASS_ENABLED 1
#elif defined AK8975_SECONDARY
static struct platform_data_s compass_pdata = {
    .orientation = {-1, 0, 0,
                     0, 1, 0,
                     0, 0,-1}
};
#define COMPASS_ENABLED 1
#elif defined AK8963_SECONDARY
static struct platform_data_s compass_pdata = {
    .orientation = {-1, 0, 0,
                     0,-1, 0,
                     0, 0, 1}
};
#define COMPASS_ENABLED 1
#endif

struct int_param_s int_param;

int inv_mpu_init(void) {
    inv_error_t result;
    unsigned char accel_fsr;
    unsigned short gyro_rate, gyro_fsr;
#ifdef COMPASS_ENABLED
    unsigned short compass_fsr;
#endif

    I2C_init();
    st_hw_msdelay(20);
    result = mpu_init(&int_param);
    if (result) {
        log_printf("Could not initialize gyro.%d\n",result);
        return -1;
    }

    result = inv_init_mpl();
    if (result) {
        log_printf("Could not initialize MPL.\n");
        return -1;
    }

    inv_enable_quaternion();
    inv_enable_9x_sensor_fusion();

    inv_enable_fast_nomot();
    inv_enable_gyro_tc();

#ifdef COMPASS_ENABLED
    /* Compass calibration algorithms. */
    inv_enable_vector_compass_cal();
    inv_enable_magnetic_disturbance();
#endif

    inv_enable_eMPL_outputs();

    result = inv_start_mpl();
    if (result == INV_ERROR_NOT_AUTHORIZED) {
        while (1) {
            log_printf("Not authorized.\n");
        }
    }
    if (result) {
        log_printf("Could not start the MPL.\n");
    }

    /* Get/set hardware configuration. Start gyro. */
    /* Wake up all sensors. */
#ifdef COMPASS_ENABLED
    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS);
#else
    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
#endif
    /* Push both gyro and accel data into the FIFO. */
    mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu_set_sample_rate(DEFAULT_MPU_HZ);
#ifdef COMPASS_ENABLED
    /* The compass sampling rate can be less than the gyro/accel sampling rate.
     * Use this function for proper power management.
     */
    mpu_set_compass_sample_rate(1000 / COMPASS_READ_MS);
#endif
    /* Read back configuration in case it was set improperly. */
    mpu_get_sample_rate(&gyro_rate);
    mpu_get_gyro_fsr(&gyro_fsr);
    mpu_get_accel_fsr(&accel_fsr);
#ifdef COMPASS_ENABLED
    mpu_get_compass_fsr(&compass_fsr);
#endif
    /* Sync driver configuration with MPL. */
    /* Sample rate expected in microseconds. */
//    inv_set_gyro_sample_rate(1000000L / gyro_rate);
//    inv_set_accel_sample_rate(1000000L / gyro_rate);
//#ifdef COMPASS_ENABLED
//    /* The compass rate is independent of the gyro and accel rates. As long as
//     * inv_set_compass_sample_rate is called with the correct value, the 9-axis
//     * fusion algorithm's compass correction gain will work properly.
//     */
//    inv_set_compass_sample_rate(COMPASS_READ_MS * 1000L);
//#endif
//    /* Set chip-to-body orientation matrix.
//     * Set hardware units to dps/g's/degrees scaling factor.
//     */
//    inv_set_gyro_orientation_and_scale(
//            inv_orientation_matrix_to_scalar(gyro_pdata.orientation),
//            (long)gyro_fsr<<15);
//    inv_set_accel_orientation_and_scale(
//            inv_orientation_matrix_to_scalar(gyro_pdata.orientation),
//            (long)accel_fsr<<15);
//#ifdef COMPASS_ENABLED
//    inv_set_compass_orientation_and_scale(
//            inv_orientation_matrix_to_scalar(compass_pdata.orientation),
//            (long)compass_fsr<<15);
//#endif
// 
//    dmp_load_motion_driver_firmware();
//    dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_pdata.orientation));
//    
//    dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
//                       DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL |
//                       DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL);
    //    dmp_set_fifo_rate(DEFAULT_MPU_HZ);
    //    dmp_set_fifo_rate(20);

    run_self_test();
    mpu_set_dmp_state(1);
    
    return 0;
}

void run_self_test(void)
{
    int result;
    long gyro[3], accel[3];

#if defined (MPU6500) || defined (MPU9250)
    result = mpu_run_6500_self_test(gyro, accel, 1);
#elif defined (MPU6050) || defined (MPU9150)
    result = mpu_run_self_test(gyro, accel);
#endif
    if (result == 0x7) {
	MPL_LOGI("Passed!\n");
        MPL_LOGI("accel: %7.4f %7.4f %7.4f\n",
                    accel[0]/65536.f,
                    accel[1]/65536.f,
                    accel[2]/65536.f);
        MPL_LOGI("gyro: %7.4f %7.4f %7.4f\n",
                    gyro[0]/65536.f,
                    gyro[1]/65536.f,
                    gyro[2]/65536.f);
        /* Test passed. We can trust the gyro data here, so now we need to update calibrated data*/

#ifdef USE_CAL_HW_REGISTERS
        /*
         * This portion of the code uses the HW offset registers that are in the MPUxxxx devices
         * instead of pushing the cal data to the MPL software library
         */
        unsigned char i = 0;

        for(i = 0; i<3; i++) {
        	gyro[i] = (long)(gyro[i] * 32.8f); //convert to +-1000dps
        	accel[i] *= 2048.f; //convert to +-16G
        	accel[i] = accel[i] >> 16;
        	gyro[i] = (long)(gyro[i] >> 16);
        }

        mpu_set_gyro_bias_reg(gyro);

#if defined (MPU6500) || defined (MPU9250)
        mpu_set_accel_bias_6500_reg(accel);
#elif defined (MPU6050) || defined (MPU9150)
        mpu_set_accel_bias_6050_reg(accel);
#endif
#else
        /* Push the calibrated data to the MPL library.
         *
         * MPL expects biases in hardware units << 16, but self test returns
		 * biases in g's << 16.
		 */
    	unsigned short accel_sens;
    	float gyro_sens;

		mpu_get_accel_sens(&accel_sens);
		accel[0] *= accel_sens;
		accel[1] *= accel_sens;
		accel[2] *= accel_sens;
		inv_set_accel_bias(accel, 3);
		mpu_get_gyro_sens(&gyro_sens);
		gyro[0] = (long) (gyro[0] * gyro_sens);
		gyro[1] = (long) (gyro[1] * gyro_sens);
		gyro[2] = (long) (gyro[2] * gyro_sens);
		inv_set_gyro_bias(gyro, 3);
#endif
    }
    else {
            if (!(result & 0x1))
                log_printf("Gyro failed.\n");
            if (!(result & 0x2))
                log_printf("Accel failed.\n");
            if (!(result & 0x4))
                log_printf("Compass failed.\n");
     }

}
