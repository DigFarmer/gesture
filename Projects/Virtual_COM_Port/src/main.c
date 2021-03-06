/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Virtual Com Port Demo main file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "hw_timer.h"
#include <string.h>
#include <stdio.h>
#include "i2c.h"
#include "log.h"
#include "mpu92xx_60xx.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
extern uint8_t usb_flg;

st_timer usb_timer;
void usb_send_handle(void *parameter) {
    //    uint8_t car[200];
    static uint32_t i = 0;
    static uint32_t j = 0;
    //    uint8_t iic_buf[20];
    short accel_data[3];
    short gyro_data[3];
//    memset(iic_buf,0,18);
//    memset(car,0,sizeof(car));
//    IIC_Read(0xD0,0x75,1,iic_buf);
//    log_printf("%d:0x%X\r\n",i++,iic_buf[0]);
// 
    //    MPL_LOGI("%d+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++======================================================************",i++);

    //    run_self_test();
//    mpu_get_gyro_reg(gyro_data,&i);
//    mpu_get_accel_reg(accel_data,&i);
//    log_printf("accel %d,%d,%d\r\n",accel_data[0],accel_data[1],accel_data[2]);
//    log_printf("gyro %d,%d,%d\r\n",gyro_data[0],gyro_data[1],gyro_data[2]);

    usb_printf("%dusb",i++);
}

/*******************************************************************************
* Function Name  : main.
* Description    : Main routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
int main(void)
{
    Set_System();
    Set_USBClock();
    USB_Interrupts_Config();
    USB_Init();
    sf_timer_init();
    serial_init();
    log_printf("hello world\r\n");

    mpu_dev_init();

//   usb_timer.func = usb_send_handle;
//   usb_timer.timeout_tick = 1000;
//   cre_sf_timer(&usb_timer,0);
    while (1)
    {
        collect_proc();
        sf_timer_proc();
    }
}
#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
