# STM32H7R/S bootflash MCU + OSPI + MCE example

We will use a already existing STM32H7R/S OSPI example from 

[this link](https://github.com/ST-TOMAS-Examples-ExtMem)

In tihs example we will use MCE protect our data by using encryption/decription. 

The MCE will encrypt the data in external loader.
And decription will be used in bootloader/application.

## STM32CubeMX 

### MCE1

1. Select `MCE1` for **Bootloader** and **ExtMem Loader**
2. In `Mode` set checkbox **Activated**

![alt text](img/24_06_06_481.png)


### HDMA

For encryption to work the memory must be in memory mapped mode. But we need to guarantee the correct acccesses which will be only write. For this reason the DMA must be used for write data to OSPI. 

1. Select **HDMA**
2. Set `Channel15`  to **Standard request mode**

![hdma](img/24_06_06_482.png)

3. Go to `SECURIT` tab
4. Set `Enable Channel as Priviledged` to **PRIVILEDGED**

![security](img/24_06_06_484.png)

5. Got to `CH15` tab
6. Set `Source Address Increment After Transfer` to **ENABLE**
7. Set `Source Burst Length` to **16** (needed by MCE!)
8. Set `Destination Address Increment After Transfer` to **ENABLE**
9. Set `Destination Burst Length` to **16** (needed by MCE!)

![hdma config](img/24_06_06_486.png)

Now we can **generate code**

### CubeIDE

**External loader** 

1. Got to ExtMemLoader to file **extmemloader_init.c** in **Core/Src** folder
2. We will ad our key for encryption/decription
   
```c
/* USER CODE BEGIN PV */
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
/* USER CODE END PV */
```

For copy:

```c
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
```

3. Configure MCE enable it

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
    pConfig.Mode=MCE_BLOCK_CIPHER;
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
    pConfig.Mode=MCE_BLOCK_CIPHER;
    pConfig.AccessMode=MCE_REGION_READONLY;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  ```
Now we need to modify the write into memory to use HDMA instead of normal byte write. 
We can do it with redefining weak `EXTMEM_MemCopy` function. 
And we put HDMA there.

4. Add HDMA configuration to **extmemloader_init.c** file

```c
/* USER CODE BEGIN 0 */
void EXTMEM_MemCopy(uint32_t* destination_Address, const uint8_t* ptrData, uint32_t DataSize){

  memcpy(buffer,local_Data,size_write);
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

  memcpy(buffer,local_Data,size_write);
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

5. Add temporary buffer to be sure all dma reads will be aligned


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

To use EXTMEM_MemCopy we need also modify another function which is deciding way how to write this function is `memory_write` in `memory_wrapper.c` in **Middleware/ST/STM32ExtMem_Loader/core**

6. We will redefine `memory_write` in our **extmemloader_init.c**

```c
MEM_STATUS memory_write(uint32_t Address, uint32_t Size, uint8_t* buffer){
  MEM_STATUS retr = MEM_OK; /* No error returned */

  if((Size%MAX_PAGE_WRITE) ==0){
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

  if((Size%MAX_PAGE_WRITE) ==0){
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


**Bootloader**

1. Got to Bootloader **main.c** file
2. We will ad our key for decription
   
```c
/* USER CODE BEGIN PV */
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
/* USER CODE END PV */
```

For copy:

```c
const uint32_t key[4] =     { 0x12345678, 0x0, 0x0, 0x0 };
```

3. Configure MCE enable it
   
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
    pConfig.Mode=MCE_BLOCK_CIPHER;
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
    pConfig.Mode=MCE_BLOCK_CIPHER;
    pConfig.AccessMode=MCE_REGION_READONLY;
    pConfig.PrivilegedAccess=MCE_REGION_PRIV;
    HAL_MCE_ConfigRegion(&hmce1,MCE_REGION1,&pConfig);
    HAL_MCE_SetRegionAESContext(&hmce1,MCE_CONTEXT1,MCE_REGION1);
    HAL_MCE_EnableRegion(&hmce1,MCE_REGION1);
  }
  ```

Now we can **compile** code and run it from **Application**.