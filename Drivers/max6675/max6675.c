/*
* max6675.c
*
*/
#include "max6675.h"

extern SPI_HandleTypeDef hspi2;

extern osPoolId  pool_KHandle;
extern osMessageQId Q_KHandle;


uint8_t data_temp[2] = {0, 0};  // [0] ��λ [1]��λ
K_Value k = {0, 0};

void read_k(void)
{  
  // ����
  HAL_GPIO_WritePin(K_PORT, K_CS, GPIO_PIN_RESET);
  
  //osDelay(250);
  // ��ȡ
  if (HAL_SPI_Receive_IT(&hspi2, &data_temp[0], 2) != HAL_OK) {
    k.error = 2;
  }
  else {    
    k.error = 0;
  }
}


void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *  hspi) 
{
  if (hspi->Instance == SPI2) {
    // ����
    HAL_GPIO_WritePin(K_PORT, K_CS, GPIO_PIN_SET);
    
    // D2 low�� ������high�� ��·  
    if (data_temp[1] & 0x4) {
      k.v = 0;
      k.error = 1; // ��·
    }
    else  {  
      k.v = (uint16_t)(( (data_temp[0] << 5) | (data_temp[1] >> 3) ) * 0.25);
      k.error = 0;
    }
    
    if (k.v>1000) {
      k.v = 0;
    }
    
    // ������Ϣ����
    if (pool_KHandle != NULL) {
      K_Value *p_k = (K_Value*)osPoolAlloc(pool_KHandle);
      
      if (p_k != NULL) {
        
        p_k->v = k.v;
        p_k->error = k.error;
        
        osMessagePut(Q_KHandle, (uint32_t)p_k, 0);
        
      } //put
    }
  }
  //
}

