#include "i2c.h"
#include "stm32f10x_i2c.h"
#include "log.h"

#define I2C_TRACE(...)

I2C_TypeDef *I2Cx;
DMA_Channel_TypeDef *I2C_Tx_Channelx;
DMA_Channel_TypeDef *I2C_Rx_Channelx;
uint32_t DMA_Tx_FLAG_TC;
uint32_t DMA_Rx_FLAG_TC;
uint32_t I2C_DMA_ADDR;


static unsigned int TimeOut=I2C_TimeOut;
unsigned char I2C_State=0x00;


#ifdef IIC_DEBUG
static void dump_i2c_register(I2C_TypeDef* I2Cx)
{
    if(I2Cx == I2C1 )
        I2C_TRACE("======I2C1======\n");
    else
        I2C_TRACE("======I2C2======\n");
    I2C_TRACE("CR1: 0x%x\tCR2: 0x%x\n", I2Cx->CR1, I2Cx->CR2);
    I2C_TRACE("SR1: 0x%x\tSR2: 0x%x\n", I2Cx->SR1, I2Cx->SR2);
}
#endif

void I2C_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    I2C_InitTypeDef   I2C_InitStructure;
//    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(I2C1_CLK | I2C2_CLK,ENABLE);
    RCC_APB2PeriphClockCmd(I2C1_GPIO_CLK | I2C2_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = I2C1_SDA_PIN | I2C1_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C1_GPIO_PORT,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = I2C2_SDA_PIN | I2C2_SCL_PIN;
    GPIO_Init(I2C2_GPIO_PORT,&GPIO_InitStructure);

    /* I2C init */
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed=200000;
    // I2C1
    I2C_Init(I2C1,&I2C_InitStructure);
    I2C_Cmd(I2C1,ENABLE);
    I2C_DMACmd(I2C1, ENABLE);
    // I2C2
    I2C_Init(I2C2,&I2C_InitStructure);
    I2C_Cmd(I2C2,ENABLE);
    I2C_DMACmd(I2C2, ENABLE);

//    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

//    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;
//    NVIC_Init(&NVIC_InitStructure);

    Set_IIC_bus(1);
}

static void I2C_Dma_Config(IIC_RT_Typedef Dir,uint8_t *pbuffer,uint16_t NumData)
{
    DMA_InitTypeDef DMA_InitStructure;
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_DMA1, ENABLE);

    /* Initialize the DMA_PeripheralBaseAddr member */
    DMA_InitStructure.DMA_PeripheralBaseAddr = I2C_DMA_ADDR;//I2C1->DR;
    /* Initialize the DMA_MemoryBaseAddr member */
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pbuffer;
    /* Initialize the DMA_PeripheralInc member */
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    /* Initialize the DMA_emoryInc member */
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    /* Initialize the DMA_PeripheralDataSize member */
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    /* Initialize the DMA_MemoryDataSize member */
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    /* Initialize the DMA_Mode member */
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    /* Initialize the DMA_Priority member */
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    /* Initialize the DMA_M2M member */
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    /* If using DMA for Reception */
    if (Dir == DMA_RX)
    {
        /* Initialize the DMA_DIR member */
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;

        /* Initialize the DMA_BufferSize member */
        DMA_InitStructure.DMA_BufferSize = NumData;

        DMA_DeInit(I2C_Rx_Channelx);

        DMA_Init(I2C_Rx_Channelx, &DMA_InitStructure);

        I2C_DMALastTransferCmd(I2Cx, ENABLE);
    }
    /* If using DMA for Transmission */
    else if (Dir == DMA_TX)
    {
        /* Initialize the DMA_DIR member */
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;

        /* Initialize the DMA_BufferSize member */
        DMA_InitStructure.DMA_BufferSize = NumData;

        DMA_DeInit(I2C_Tx_Channelx);

        DMA_Init(I2C_Tx_Channelx, &DMA_InitStructure);
    }

}

unsigned char IIC_Write(uint8_t PartAddr,uint8_t WriteAddr,uint16_t NumByteToWrite,uint8_t *pBuffer)
{
    __disable_irq();
    TimeOut = I2C_TimeOut *100;
    while (I2C_GetFlagStatus(I2Cx,I2C_FLAG_BUSY)) {
        if ((TimeOut --) == 0) {
            log_printf("w1\r\n");
            goto out;
        }
    }
    /*send Start condition,test on EV5 and clear it*/
    TimeOut=I2C_TimeOut;
    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) {
        if ((TimeOut --) == 0) {
            log_printf("w2\r\n");
            goto out;
        }
    }

    /*send Part Address for write,test on EV6 and clear it*/
    TimeOut = I2C_TimeOut;
    I2C_Send7bitAddress(I2Cx, PartAddr, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if ((TimeOut --) == 0) {
            log_printf("w3\r\n");
            goto out;
        }
    }

    TimeOut=I2C_TimeOut;
    I2C_SendData(I2Cx, (uint8_t)WriteAddr);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if ((TimeOut --) == 0) {
            log_printf("w4\r\n");
            goto out;
        }
    }

    if (NumByteToWrite > 1) {
        /* Configure DMA Peripheral */
        I2C_Dma_Config(DMA_TX, pBuffer, NumByteToWrite);

        DMA_Cmd(I2C_Tx_Channelx, ENABLE);
        I2C_AcknowledgeConfig(I2Cx, DISABLE);

        TimeOut = I2C_TimeOut * NumByteToWrite;
        while (!DMA_GetFlagStatus(DMA_Tx_FLAG_TC)) {
            if ((TimeOut --) == 0) {
                log_printf("w5\r\n");
                goto out;
            }
        }
        while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if ((TimeOut --) == 0) {
                log_printf("w6\r\n");
                goto out;
            }
        }
        I2C_GenerateSTOP(I2Cx, ENABLE);
        DMA_Cmd(I2C_Tx_Channelx, DISABLE);
        DMA_ClearFlag(DMA_Tx_FLAG_TC);

    } else {
        while(NumByteToWrite --) {
            I2C_SendData(I2Cx, *pBuffer);
            TimeOut = I2C_TimeOut;
            while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
                if ((TimeOut --) == 0) {
                    log_printf("w7\r\n");
                    goto out;
                }
            }
            pBuffer ++;
        }

        /*send STOP condition*/
        I2C_GenerateSTOP(I2Cx, ENABLE);
    }
    __enable_irq();
    return  I2C_NOTimeout;

out:
    __enable_irq();
    I2C_State = I2C_RTimeOut;
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return  I2C_WTimeOut;
}

unsigned char IIC_Read(uint8_t PartAddr,uint8_t WriteAddr,uint16_t NumByteToRead,uint8_t *pBuffer)
{
    __disable_irq();
    TimeOut = I2C_TimeOut;
    while (I2C_GetFlagStatus(I2Cx,I2C_FLAG_BUSY)) {
        if ((TimeOut --) == 0) {
            log_printf("r1\r\n");
            goto out;
        }
    }
    //    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    /*send Start condition,test on EV5 and clear it*/
    TimeOut = I2C_TimeOut;
    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) {
        if ((TimeOut --) == 0) {
            log_printf("r2\r\n");
            goto out;
        }
    }

    /*send Part Address for write,test on EV6 and clear it*/
    TimeOut = I2C_TimeOut;
    I2C_Send7bitAddress(I2Cx, PartAddr, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if ((TimeOut --) == 0) {
            log_printf("r3\r\n");
            goto out;
        }
    }

    TimeOut = I2C_TimeOut;
    /**send the  internal address,test on EV8 and clear it**/
    I2C_SendData(I2Cx, (uint8_t)WriteAddr);
    while(!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if ((TimeOut --) == 0) {
            log_printf("r4\r\n");
            goto out;
        }
    }
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    /*send Start condition,test on EV5 and clear it*/
    TimeOut = I2C_TimeOut;
    I2C_GenerateSTART(I2Cx, ENABLE);
    while(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) {
        if((TimeOut --) == 0) {
            log_printf("r5\r\n");
            goto out;
        }
    }

    TimeOut = I2C_TimeOut;
    I2C_Send7bitAddress(I2Cx, PartAddr, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        if ((TimeOut --) == 0) {
            log_printf("r6\r\n");
            goto out;
        }
    }

    /* read data*/
    if(NumByteToRead > 2) {
        I2C_Dma_Config(DMA_RX,pBuffer, NumByteToRead);
        DMA_Cmd(I2C_Rx_Channelx, ENABLE);

        TimeOut = I2C_TimeOut * NumByteToRead;
        while (!DMA_GetFlagStatus(DMA_Rx_FLAG_TC)) {
            if ((TimeOut --) == 0) {
                log_printf("r7\r\n");
                goto out;
            }
        }
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        I2C_GenerateSTOP(I2Cx, ENABLE);

        DMA_Cmd(I2C_Rx_Channelx, DISABLE);
        DMA_ClearFlag(DMA_Rx_FLAG_TC);
        I2C_GenerateSTOP(I2Cx, ENABLE);
    } else {
        while (NumByteToRead) {
            if (NumByteToRead == 1) {
                I2C_NACKPositionConfig(I2Cx, I2C_NACKPosition_Current);
                I2C_AcknowledgeConfig(I2Cx, DISABLE);
                I2C_GenerateSTOP(I2Cx, ENABLE);
            }
            if (I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
                *pBuffer = I2C_ReceiveData(I2Cx);
                pBuffer ++;
                NumByteToRead --;
                TimeOut = I2C_TimeOut;
            } else {
                if ((TimeOut --) == 0) {
                    log_printf("r8\r\n");
                    goto out;
                }
            }
        }
    }

    __enable_irq();
    return  I2C_NOTimeout;

out:
    __enable_irq();
    I2C_State = I2C_RTimeOut;
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return  I2C_RTimeOut;
}

unsigned char CheckIIC_Ack(uint8_t PartAddr)
{
    /*send Start condition,test on EV5 and clear it*/
    TimeOut = 200;
    I2C_GenerateSTART(I2Cx, ENABLE);
    while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) {
        if ((TimeOut --) == 0) {
            I2C_State = I2C_WTimeOut;
            return  I2C_WTimeOut;
        }
    }

    /*send Part Address for write,test on EV6 and clear it*/
    TimeOut = 200;
    I2C_Send7bitAddress(I2Cx, PartAddr,I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if ((TimeOut --) == 0) {
            I2C_State = I2C_WTimeOut;
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return  I2C_WTimeOut;
        }
    }

    TimeOut = 320;
    I2C_SendData(I2Cx, 0X00);
    while (!I2C_CheckEvent(I2Cx,I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if((TimeOut --) == 0) {
            I2C_State = I2C_WTimeOut;
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return  I2C_WTimeOut;
        }
    }

    I2C_GenerateSTOP(I2Cx, ENABLE);
    return  I2C_NOTimeout;
}

void Set_IIC_bus(char dev)
{
    switch(dev) {
        case 1:
            I2Cx = I2C1;
            I2C_DMA_ADDR = I2C1_DMA_ADDR;
            I2C_Tx_Channelx = I2C1_DMA_TX_CHANNEL;
            I2C_Rx_Channelx = I2C1_DMA_RX_CHANNEL;
            DMA_Tx_FLAG_TC = DMA1_FLAG_TC6;
            DMA_Rx_FLAG_TC = DMA1_FLAG_TC7;
            break;
        case 2:
            I2Cx = I2C2;
            I2C_DMA_ADDR = I2C2_DMA_ADDR;
            I2C_Tx_Channelx = I2C2_DMA_TX_CHANNEL;
            I2C_Rx_Channelx = I2C2_DMA_RX_CHANNEL;
            DMA_Tx_FLAG_TC = DMA1_FLAG_TC4;
            DMA_Rx_FLAG_TC = DMA1_FLAG_TC5;
            break;
        default:
            break;
    }
}


/*
void DMA1_Channel6_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC6) != RESET) {
        DMA_Cmd(I2C1_DMA_TX_CHANNEL, DISABLE);
        DMA_ClearFlag(DMA1_FLAG_GL6);

        TimeOut = I2C_TimeOut;
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if ((TimeOut --) == 0) {
                log_printf("w5r\n");
            }else{
                log_printf("w_ok\r\n");
            }
        }
        I2C_GenerateSTOP(I2C1, ENABLE);
        DMA_ClearITPendingBit(DMA1_IT_TC6);
    }

}
*/
