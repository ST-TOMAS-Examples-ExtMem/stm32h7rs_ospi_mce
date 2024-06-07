/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Loader_Src.c
  * @author  MCD Application Team
  * @brief   This file defines the operations of the external memory loader.
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "extmemloader_init.h"
#include <string.h>
#include <stdio.h>
#include "extmem_manager.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "memory_wrapper.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
MCE_HandleTypeDef hmce1;

XSPI_HandleTypeDef hxspi1;

/* USER CODE BEGIN PV */
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
#define MAX_PAGE_WRITE 0x100
uint32_t buffer[MAX_PAGE_WRITE]  __attribute__ ((aligned (1024))) ;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

static void MX_GPIO_Init(void);
static void MX_SBS_Init(void);
static void MX_XSPI1_Init(void);
static void MX_MCE1_Init(void);
static void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void EXTMEM_MemCopy(uint32_t* destination_Address, const uint8_t* ptrData, uint32_t DataSize){
  if(DataSize<MAX_PAGE_WRITE){
    memset(&buffer[DataSize],0xff,MAX_PAGE_WRITE-DataSize);
  }
  memcpy(buffer,ptrData,DataSize);
  if(DataSize<MAX_PAGE_WRITE){
    DataSize=MAX_PAGE_WRITE;
  }
  DMA_HandleTypeDef handle_HPDMA1_Channel15;
  HAL_Delay(2);
  __HAL_RCC_HPDMA1_CLK_ENABLE();
  handle_HPDMA1_Channel15.Instance = HPDMA1_Channel15;
  handle_HPDMA1_Channel15.Init.Request = DMA_REQUEST_SW;
  handle_HPDMA1_Channel15.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
  handle_HPDMA1_Channel15.Init.Direction = DMA_MEMORY_TO_MEMORY;
  handle_HPDMA1_Channel15.Init.SrcInc = DMA_SINC_INCREMENTED;
  handle_HPDMA1_Channel15.Init.DestInc = DMA_DINC_INCREMENTED;
  handle_HPDMA1_Channel15.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
  handle_HPDMA1_Channel15.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
  handle_HPDMA1_Channel15.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
  handle_HPDMA1_Channel15.Init.SrcBurstLength = 16;
  handle_HPDMA1_Channel15.Init.DestBurstLength = 16;
  handle_HPDMA1_Channel15.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
  handle_HPDMA1_Channel15.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
  handle_HPDMA1_Channel15.Init.Mode = DMA_NORMAL;
  if (HAL_DMA_Init(&handle_HPDMA1_Channel15) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA_ConfigChannelAttributes(&handle_HPDMA1_Channel15, DMA_CHANNEL_PRIV) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_DMA_Start(&handle_HPDMA1_Channel15,(uint32_t)buffer,destination_Address,DataSize);
  HAL_DMA_PollForTransfer(&handle_HPDMA1_Channel15,HAL_DMA_FULL_TRANSFER,0xFFFFFFFF);
  HAL_Delay(2);
}


MEM_STATUS memory_write(uint32_t Address, uint32_t Size, uint8_t* buffer){
  MEM_STATUS retr = MEM_OK; /* No error returned */

  if(Size>=MAX_PAGE_WRITE){
    /* memory mapped write for 256B*/
    if (EXTMEM_WriteInMappedMode(STM32EXTLOADER_DEVICE_MEMORY_ID, Address, buffer, Size) != EXTMEM_OK)
    {
      retr = MEM_FAIL;
    }
  }else{
    /* normal byte write*/
    if (EXTMEM_Write(STM32EXTLOADER_DEVICE_MEMORY_ID, Address & 0x0FFFFFFFUL, buffer, Size) != EXTMEM_OK)
    {
      retr = MEM_FAIL;
    }
  }

  return retr;
}
/* USER CODE END 0 */

/**
  * @brief  main entry of the external flash loader initialization.
  * @param  None
  * @retval  0 : operation is ok else operation is failed

  */
uint32_t extmemloader_Init()
{
  uint32_t retr = 0;
  /* USER CODE BEGIN 1 */
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x90000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256MB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
  /* USER CODE END 1 */

  /* Init system */
  SystemInit();

  /* disable all the IRQ */
  __disable_irq();

  /* MCU Configuration--------------------------------------------------------*/
  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  //SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  //SCB_EnableDCache();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* MPU Configuration--------------------------------------------------------*/
  //HAL_MPU_Disable();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock  */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  MX_GPIO_Init();

  MX_SBS_Init();

  MX_XSPI1_Init();

  MX_MCE1_Init();

  MX_EXTMEM_MANAGER_Init();

  /* USER CODE BEGIN 2 */
  {
    MCE_AESConfigTypeDef  AESConfig;
    MCE_RegionConfigTypeDef pConfig;
    MCE_NoekeonConfigTypeDef pConfigNeo;
    AESConfig.Nonce[0]=0x0;
    AESConfig.Nonce[1]=0x0;
    AESConfig.Version=0x0;
    AESConfig.pKey=key;
    HAL_MCE_ConfigAESContext(&hmce1,&AESConfig,MCE_CONTEXT1);
    HAL_MCE_EnableAESContext(&hmce1,MCE_CONTEXT1);

    pConfig.ContextID=MCE_CONTEXT1;
    pConfig.StartAddress=0x90000000;
    pConfig.EndAddress=0x92000000;
    pConfig.Mode=MCE_BLOCK_CIPHER;
    pConfig.AccessMode=MCE_REGION_READWRITE;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  /* USER CODE END 2 */

  return retr;
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL1.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL1.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL1.PLLM = 16;
  RCC_OscInitStruct.PLL1.PLLN = 300;
  RCC_OscInitStruct.PLL1.PLLP = 2;
  RCC_OscInitStruct.PLL1.PLLQ = 2;
  RCC_OscInitStruct.PLL1.PLLR = 2;
  RCC_OscInitStruct.PLL1.PLLS = 2;
  RCC_OscInitStruct.PLL1.PLLT = 2;
  RCC_OscInitStruct.PLL1.PLLFractional = 0;
  RCC_OscInitStruct.PLL2.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL2.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL2.PLLM = 4;
  RCC_OscInitStruct.PLL2.PLLN = 50;
  RCC_OscInitStruct.PLL2.PLLP = 2;
  RCC_OscInitStruct.PLL2.PLLQ = 2;
  RCC_OscInitStruct.PLL2.PLLR = 2;
  RCC_OscInitStruct.PLL2.PLLS = 4;
  RCC_OscInitStruct.PLL2.PLLT = 2;
  RCC_OscInitStruct.PLL2.PLLFractional = 0;
  RCC_OscInitStruct.PLL3.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK4|RCC_CLOCKTYPE_PCLK5;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief MCE1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_MCE1_Init(void)
{

  /* USER CODE BEGIN MCE1_Init 0 */

  /* USER CODE END MCE1_Init 0 */

  /* USER CODE BEGIN MCE1_Init 1 */

  /* USER CODE END MCE1_Init 1 */
  hmce1.Instance = MCE1;
  if (HAL_MCE_Init(&hmce1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN MCE1_Init 2 */

  /* USER CODE END MCE1_Init 2 */

}

/**
  * @brief SBS Initialization Function
  * @param None
  * @retval None
  */
static void MX_SBS_Init(void)
{

  /* USER CODE BEGIN SBS_Init 0 */

  /* USER CODE END SBS_Init 0 */

  /* USER CODE BEGIN SBS_Init 1 */

  /* USER CODE END SBS_Init 1 */
  /* USER CODE BEGIN SBS_Init 2 */

  /* USER CODE END SBS_Init 2 */

}

/**
  * @brief XSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_XSPI1_Init(void)
{

  /* USER CODE BEGIN XSPI1_Init 0 */

  /* USER CODE END XSPI1_Init 0 */

  XSPIM_CfgTypeDef sXspiManagerCfg = {0};

  /* USER CODE BEGIN XSPI1_Init 1 */

  /* USER CODE END XSPI1_Init 1 */
  /* XSPI1 parameter configuration*/
  hxspi1.Instance = XSPI1;
  hxspi1.Init.FifoThresholdByte = 1;
  hxspi1.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
  hxspi1.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
  hxspi1.Init.MemorySize = HAL_XSPI_SIZE_32MB;
  hxspi1.Init.ChipSelectHighTimeCycle = 2;
  hxspi1.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
  hxspi1.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
  hxspi1.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
  hxspi1.Init.ClockPrescaler = 0;
  hxspi1.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
  hxspi1.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
  hxspi1.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
  hxspi1.Init.MaxTran = 0;
  hxspi1.Init.Refresh = 0;
  hxspi1.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
  if (HAL_XSPI_Init(&hxspi1) != HAL_OK)
  {
    Error_Handler();
  }
  sXspiManagerCfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
  sXspiManagerCfg.IOPort = HAL_XSPIM_IOPORT_2;
  if (HAL_XSPIM_Config(&hxspi1, &sXspiManagerCfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN XSPI1_Init 2 */

  /* USER CODE END XSPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPION_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

