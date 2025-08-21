#include "websocket.h"
#include "main.h"

#define UART7_TX_PORT  GPIOB
#define UART7_TX_PIN   GPIO_PIN_11
#define UART7_TX_CLOCK RCC_AHB_PERIPHEN_GPIOB

#define UART7_RX_PORT  GPIOB
#define UART7_RX_PIN   GPIO_PIN_10
#define UART7_RX_CLOCK RCC_AHB_PERIPHEN_GPIOB

#define UART7_RX_DMA_CH DMA2_CH6
#define UART7_TX_DMA_CH DMA2_CH7


/**
 * @brief  UART7 时钟初始化函数
 * @note   启用 UART7 模块及其相关外设（GPIO、AFIO、DMA）的时钟
 *         为后续配置引脚、串口和 DMA 提供时钟支持
 * @param  无
 * @retval 无
 */
void UART7_RCC_INIT(void)
{
    // 启用 UART7 的 TX 和 RX 引脚所在 GPIO 端口的 AHB1 总线时钟
    RCC_EnableAHB1PeriphClk(UART7_TX_CLOCK | UART7_RX_CLOCK, ENABLE); 

    // 使能复用功能 I/O (AFIO) 的 APB2 总线时钟，用于配置引脚的复用功能（如 UART 功能）
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);    

    // 使能 UART7 模块自身的 APB2 总线时钟，允许访问其寄存器
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART7, ENABLE);

    // 使能 DMA2 模块的 AHB 总线时钟，为后续使用 DMA 接收数据做准备
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPHEN_DMA2, ENABLE);    
}

/**
 * @brief  UART7 GPIO 引脚初始化函数
 * @note   配置 UART7 的发送（TX）和接收（RX）引脚为复用推挽模式
 *         并设置上拉、快速切换速率等参数
 * @param  无
 * @retval 无
 */
void UART7_GPIO_INIT(void)
{
    GPIO_InitType GPIO_InitStruct;

    // 配置 UART7 TX 引脚
    GPIO_InitStruct.Pin = UART7_TX_PIN;                          // 指定 TX 引脚编号
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;                // 设置为复用推挽输出模式
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_UART7_TX_PB11;      // 映射到 UART7 的 TX 功能
    GPIO_InitStruct.GPIO_Pull = GPIO_PULL_UP;                   // 启用上拉电阻，提高信号稳定性
    GPIO_InitStruct.GPIO_Slew_Rate = GPIO_SLEW_RATE_FAST;       // 设置引脚切换速率为“快速”
    GPIO_InitPeripheral(UART7_TX_PORT, &GPIO_InitStruct);       // 应用配置到指定端口

    // 配置 UART7 RX 引脚
    GPIO_InitStruct.Pin = UART7_RX_PIN;                          // 指定 RX 引脚编号
    GPIO_InitStruct.GPIO_Mode = GPIO_MODE_AF_PP;                // 同样为复用推挽模式（输入由硬件自动管理）
    GPIO_InitStruct.GPIO_Alternate = GPIO_AF_UART7_RX_PB10;      // 映射到 UART7 的 RX 功能
    GPIO_InitStruct.GPIO_Pull = GPIO_PULL_UP;                   // 启用上拉，增强抗干扰能力
    GPIO_InitStruct.GPIO_Slew_Rate = GPIO_SLEW_RATE_FAST;       // 快速切换速率
    GPIO_InitPeripheral(UART7_RX_PORT, &GPIO_InitStruct);       // 应用配置到 RX 端口
}

/**
 * @brief  UART7 串口模块初始化函数
 * @note   配置 UART7 的基本通信参数，包括波特率、数据位、停止位、校验方式等
 *         并启用接收/发送功能、DMA 接收请求和空闲中断
 * @param  无
 * @retval 无
 */
void UART7_INIT(void)
{
    USART_InitType USART_InitStructure;
    USART_StructInit(&USART_InitStructure); // 使用默认值填充结构体，确保未设置字段安全

    // 配置串口通信参数
    USART_InitStructure.BaudRate = 43000;                       // 设置波特率为 43000 bps
    USART_InitStructure.WordLength = USART_WL_8B;               // 数据位长度：8 位
    USART_InitStructure.StopBits = USART_STPB_1;                // 停止位：1 位
    USART_InitStructure.Parity = USART_PE_NO;                   // 无奇偶校验
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;// 不使用硬件流控（RTS/CTS）
    USART_InitStructure.OverSampling = USART_16OVER;            // 采用 16 倍过采样以提高稳定性
    USART_InitStructure.Mode = USART_MODE_RX | USART_MODE_TX;   // 启用接收和发送双工模式

    // 将配置应用到 UART7 外设
    USART_Init(UART7, &USART_InitStructure);

    // 启用 UART7 的 DMA 接收请求，实现数据自动搬运到内存
    USART_EnableDMA(UART7, USART_DMAREQ_RX, ENABLE);

    // 使能空闲线检测中断（IDLE Interrupt），用于判断一帧数据接收完成
    USART_ConfigInt(UART7, USART_INT_IDLEF, ENABLE);

    // 最后使能 UART7 模块，开始工作
    USART_Enable(UART7, ENABLE);
}

/**
 * @brief  UART7 DMA 接收通道配置函数
 * @note   配置 DMA 通道用于从 UART7 数据寄存器自动搬运接收到的数据到指定缓冲区
 *         支持通过空闲中断 + DMA 实现“不定长数据接收”
 * @param  buffer        - [in] 用户提供的数据接收缓冲区首地址
 * @param  BUFFER_SIZE   - [in] 缓冲区总大小（字节）
 * @retval 无
 */
void UART7_DMA_Configuration(uint8_t *buffer, int BUFFER_SIZE)
{
    DMA_InitType DMA_InitStructure;

    // 复位并禁用指定的 DMA 通道，确保初始状态干净
    DMA_DeInit(UART7_RX_DMA_CH);

    // 使用默认值初始化 DMA 配置结构体
    DMA_StructInit(&DMA_InitStructure);

    // 配置 DMA 传输参数
    DMA_InitStructure.PeriphAddr = (uint32_t)&UART7->DAT;           // 外设地址：UART7 数据寄存器
    DMA_InitStructure.MemAddr = (uint32_t)buffer;                   // 内存地址：用户缓冲区
    DMA_InitStructure.Direction = DMA_DIR_PERIPH_SRC;               // 传输方向：外设为源（读取数据）
    DMA_InitStructure.BufSize = BUFFER_SIZE;                        // 传输数据总量（字节）
    DMA_InitStructure.PeriphInc = DMA_PERIPH_INC_DISABLE;           // 外设地址不自增（始终读 DAT 寄存器）
    DMA_InitStructure.MemoryInc = DMA_MEM_INC_ENABLE;               // 内存地址自增，依次存入缓冲区
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_WIDTH_BYTE;  // 外设数据宽度：字节
    DMA_InitStructure.MemDataSize = DMA_MEM_DATA_WIDTH_BYTE;        // 内存数据宽度：字节
    DMA_InitStructure.CircularMode = DMA_MODE_NORMAL;               // 正常模式（非循环），适合一帧接收
    DMA_InitStructure.Priority = DMA_PRIORITY_VERY_HIGH;            // 设置为最高优先级，避免丢数据
    DMA_InitStructure.Mem2Mem = DMA_M2M_DISABLE;                    // 禁用内存到内存传输

    // 初始化 DMA 通道，应用上述配置
    DMA_Init(UART7_RX_DMA_CH, &DMA_InitStructure);

    // 重新映射 DMA 请求通道，确保 UART7_RX 正确连接到指定 DMA 通道
    DMA_RequestRemap(DMA_REMAP_UART7_RX, UART7_RX_DMA_CH, ENABLE);

    // 启用 DMA 通道，开始监听 UART7 的数据接收请求
    DMA_EnableChannel(UART7_RX_DMA_CH, ENABLE);
}

/**
 * @brief  UART7 NVIC 中断优先级配置函数
 * @note   配置 UART7 中断的抢占优先级和子优先级
 *         用于处理空闲中断（IDLE）等事件
 * @param  无
 * @retval 无
 */
void UART7_NVIC_Configuration(void)
{
    NVIC_InitType NVIC_InitStructure;

    // 设置中断优先级分组为 Group 2：2 位抢占优先级，2 位子优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 配置 UART7 中断通道
    NVIC_InitStructure.NVIC_IRQChannel = UART7_IRQn;                  // 指定中断源为 UART7
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;         // 抢占优先级设为 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;                // 子优先级设为 1
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                   // 使能该中断通道

    // 应用配置到 NVIC（嵌套向量中断控制器）
    NVIC_Init(&NVIC_InitStructure);
}

// 发送 AT 指令并等待响应
// AT： 测试模块是否已经上电启动，通讯是否正常
// AT+RST：重启模块
// AT+GMR：查看版本信息
// ATE：开启或关闭AT回显功能
// AT+RESTORE：恢复出厂设置
// AT+CWMODE=1：Station 模式（等同于AT+CWMODE=1,1 自动连接热点），若设置AT+CWMODE=1,0则禁止自动连接热点（连接成功之后，重启开机）
// AT+CWJAP=“热点名”,“密码”
// AT+CWQAP：断开与AP的连接
// AT+CWAUTOCONN=1：上电自动连接AP，默认；0：上电不自动连接AP

// AT+WSCFG=0,30,600,4096 配置WebSocket参数
// AT+WSOPEN=0,"ws://5.181.225.4:23530//" 连接
// AT+WSOPEN? 查询连接
// AT+WSSEND=0,n,2 //发送n字节数据
// AT+WSCLOSE=0 关闭link_id为0的连接
bool UART7_SendATCommand(const char* cmd) 
{
  // 发送指令
  for (uint8_t i = 0; i < strlen(cmd); i++) 
	{
		USART_SendData(UART7, cmd[i]);
		while (USART_GetFlagStatus(UART7, USART_FLAG_TXC) == RESET); // 等待发送完成
  }
	return true;
}

void USART7_SendBuffer(const uint8_t* pData, uint32_t Size)
{
    for (uint32_t i = 0; i < Size; i++)
    {
        USART_SendData(UART7, pData[i]);  // 逐字节发送
        while (USART_GetFlagStatus(UART7, USART_FLAG_TXC) == RESET); // 等待发送完成
    }
}
