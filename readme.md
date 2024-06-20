# STM32H7R/S bootflash MCU + OSPI + MCE example

We will utilize an existing OSPI example for the STM32H7R/S series, which can be found under
[this link](https://github.com/ST-TOMAS-Examples-ExtMem)


In this example, the Memory Cipher Engine (MCE) will be employed to protect our code and data through the on-the-fly encryption/decryption. 
The Memory Cipher Engine (MCE) will encrypt the data within the external memory loader, while decryption will be carried out in the bootloader or the application itself.

## STM32CubeMX 

### MCE1

1. Select `MCE1` for **Bootloader** and **ExtMem Loader**.
2. In `Mode` ensure the **Activated** checkbox is checked.

![alt text](img/24_06_06_481.png)


### HPDMA (High-Performance Direct Memory Access)

For encryption to work the memory must be in memory-mapped mode. But we need to guarantee the correct acccesses which will be only write. For this reason the DMA must be used for write data to OSPI. 
To ensure successful encryption, the memory must be operated in memory-mapped mode. Additionally, it is essential to ensure that access is restricted to write-only operations. Therefore, the Direct Memory Access (DMA) should be utilized to write data to the Octal SPI (OSPI).

1. Select **HPDMA1**.
2. Set `Channel15`  to **Standard request mode**.

![hdma](img/24_06_06_482.png)

3. Go to the `SECURITY` tab.
4. Set `Enable Channel as Priviledged` to **PRIVILEDGED**.

![security](img/24_06_06_484.png)

5. Navigate to the `CH15` tab.
6. Set `Source Address Increment After Transfer` to **ENABLE**.
7. Set `Source Burst Length` to **16** (needed by MCE!).
8. Set `Destination Address Increment After Transfer` to **ENABLE**.
9. Set `Destination Burst Length` to **16** (needed by MCE!).

![hdma config](img/24_06_06_486.png)

Now we can **generate code**.

### STM32CubeIDE

**External Memory Loader** 

1. Go to ExtMemLoader to file **extmemloader_init.c** in **Core/Src** folder.
2. Include the encryption/decryption key.
   
```c
/* USER CODE BEGIN PV */
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
/* USER CODE END PV */
```

For copy:

```c
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
```

3. Configure MCE and enable it.

```c
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
    pConfig.Mode=MCE_STREAM_CIPHER;
    pConfig.AccessMode=MCE_REGION_READWRITE;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  /* USER CODE END 2 */
  ```

For copy:

```c
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
    pConfig.Mode=MCE_STREAM_CIPHER;
    pConfig.AccessMode=MCE_REGION_READWRITE;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  ```
We now need to update the memory write process to utilize High-Performance Direct Memory Access controller (HPDMA) rather than the standard byte write method. 
This can be achieved by overriding the weak `EXTMEM_MemCopy` function with our own implementation that incorporates HPDMA.

4. Add HPDMA configuration to **extmemloader_init.c** file.

```c
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
  HAL_DMA_Start(&handle_HPDMA1_Channel15,(uint32_t)buffer,local_Address,size_write);
  HAL_DMA_PollForTransfer(&handle_HPDMA1_Channel15,HAL_DMA_FULL_TRANSFER,0xFFFFFFFF);
  HAL_Delay(2);
}
/* USER CODE END 0 */
```

for copy:

```c
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
  handle_HPDMA1_Channel15.Instance = HPDMA1_Channel12;
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
  HAL_DMA_Start(&handle_HPDMA1_Channel15,(uint32_t)buffer,local_Address,size_write);
  HAL_DMA_PollForTransfer(&handle_HPDMA1_Channel15,HAL_DMA_FULL_TRANSFER,0xFFFFFFFF);
  HAL_Delay(2);
}
```

5. Add a temporary buffer to ensure that all DMA reads are properly aligned.


```c
/* USER CODE BEGIN PV */
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
#define MAX_PAGE_WRITE 0x100
uint32_t buffer[MAX_PAGE_WRITE]  __attribute__ ((aligned (1024))) ;
/* USER CODE END PV */
```

for copy:

```c
#define MAX_PAGE_WRITE 0x100
uint32_t buffer[MAX_PAGE_WRITE]  __attribute__ ((aligned (1024))) ;
```

To utilize EXTMEM_MemCopy, we also need to modify an additional function that determines the method of writing. This function is `memory_write` in `memory_wrapper.c` in **Middleware/ST/STM32ExtMem_Loader/core**.

6. We will redefine `memory_write` in our **extmemloader_init.c**.

```c
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
```

for copy:

```c
MEM_STATUS memory_write(uint32_t Address, uint32_t Size, uint8_t* buffer){
  MEM_STATUS retr = MEM_OK; /* No error returned */
  uint32_t addr = Address & 0x0FFFFFFFUL;

  if(Size>=MAX_PAGE_WRITE){
    /* memory mapped write for 256B*/
    if (EXTMEM_WriteInMappedMode(STM32EXTLOADER_DEVICE_MEMORY_ID, addr, buffer, Size) != EXTMEM_OK)
    {
      retr = MEM_FAIL;
    }
  }else{
    /* normal byte write*/
    if (EXTMEM_Write(STM32EXTLOADER_DEVICE_MEMORY_ID, addr, buffer, Size) != EXTMEM_OK)
    {
      retr = MEM_FAIL;
    }
  }

  return retr;
}
```

7. Disable write access to the OSPI memory from the core by configuring the **Memory Protection Unit (MPU)**.
Insert the relevant code into the `extmemloader_Init` function found in **in extmemloader_init.c**.

```c
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
```

for copy

```c
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
```

   8. Ensure that both Instruction Cache (ICache) and Data Cache (DCache) are disabled in the external memory loader, while maintaining the use of the Memory Protection Unit (MPU).

```c
  /* Enable I-Cache---------------------------------------------------------*/
  //SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  //SCB_EnableDCache();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* MPU Configuration--------------------------------------------------------*/
  //HAL_MPU_Disable();
```

for copy

```c
  //SCB_EnableICache();
```

```c
  //SCB_EnableDCache();
```

```c
  //HAL_MPU_Disable();
```



**Bootloader**

1. Navigate to the Bootloader **main.c** file.
2. Include the encryption/decryption key.
   
```c
/* USER CODE BEGIN PV */
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
/* USER CODE END PV */
```

For copy:

```c
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
```

3. Configure MCE and enable it.
   
```c
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
    pConfig.Mode=MCE_STREAM_CIPHER;
    pConfig.AccessMode=MCE_REGION_READONLY;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  /* USER CODE END 2 */
  ```

For copy:

```c
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
    pConfig.Mode=MCE_STREAM_CIPHER;
    pConfig.AccessMode=MCE_REGION_READONLY;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  ```

4. Add cache invalidation to ensure their contents are valid.

```c 
  /* USER CODE BEGIN 1 */
  SCB_InvalidateDCache();
  SCB_InvalidateICache();
  /* USER CODE END 1 */
```
  for copy:

```c
  SCB_InvalidateDCache();
  SCB_InvalidateICache();
```

Now we can **compile** the code and run it from the **Application**.
