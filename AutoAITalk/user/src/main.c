#include "main.h"

// ========================================
// 音频参数配置
// ========================================

#define SAMPLE_RATE 16000  // 采样率：16kHz，适用于语音场景
#define CHANNEL 1 // 声道数：单声道（Mono）
#define VOLUME 0.15 // 音频增益（音量）系数，用于调节录音音量大小（0.0 ~ 1.0）

#define PCM_BUFFER_SIZE 960 // PCM 数据缓冲区大小（采样点数），对应 60ms 音频（960/16000 ≈ 0.06s）
#define OPUS_BUFFER_SIZE 960 // PCM 数据缓冲区大小（采样点数），对应 60ms 音频（960/16000 ≈ 0.06s）


// ========================================
// 录音相关缓冲区与变量
// ========================================

// 用于存储处理完成，用于Opus编码的PCM数据
int16_t Real_I2S_MIC_Buffer_4_OPUS[PCM_BUFFER_SIZE];

// 用于存储添加输入增益放大后的原始PCM数据
int32_t Real_I2S_MIC_Data_32;

// 存储编码后的 Opus 音频数据包
uint8_t OPUS_Buffer[OPUS_BUFFER_SIZE];

// 用于双缓冲机制的 I2S 麦克风输入缓冲区
int16_t I2S_MIC_Buffer[PCM_BUFFER_SIZE*2*2];


// ========================================
// 播放相关缓冲区与状态控制
// ========================================

// 接收 WebSocket 传输的 Opus 数据包缓冲区
uint8_t WS_OPUS_Buffer[300];

// 双缓冲 PCM 播放缓冲区（两个缓冲区交替填充和播放）
int16_t PLAY_PCM_Buffer[2][PCM_BUFFER_SIZE];

// 保存Opus解码的PCM数据
int16_t SAVE_PLAY_PCM_Buffer[PCM_BUFFER_SIZE];

// 当前正在被填充的播放缓冲区索引
uint8_t ActivePlayingBuf = 0;

// 上一个已完成填充的播放缓冲区索引，供 DAC 播放使用
uint8_t LastActivePlayingBuf = 0;


// ========================================
// WebSocket 与通信相关缓冲区
// ========================================

// 每个接收帧的最大字节数（约 1.5KB），用于 WebSocket 数据接收
uint32_t RX_BUFFER_SIZE = 1512;

// 双缓冲接收机制：两个缓冲区交替接收 WebSocket 数据
uint8_t RxBuf[2][1512];

// 临时工作缓冲区，用于组装或解析 WebSocket 数据帧
uint8_t WS_DATA_BUF[1024];

// 当前接收缓冲区中已接收的字节数
uint16_t RxCnt = 0;

// WS_DATA_BUF 中已写入的数据长度
uint16_t WsBufCnt = 0;

// 当前正在写入的接收缓冲区索引（0 或 1）
uint8_t ActiveBuf = 0;

// 接收到的音频帧计数器
uint16_t cnt_frame = 0;


// ========================================
// 设备运行状态机
// ========================================

/*
DeviceState - 设备状态标识（状态机）
0: 初始化中（等待网络、模块就绪）
1: 正在接收音频（从麦克风采集并上传）
2: 正在输出音频（播放云端返回的语音回复）
3: 出现错误（需重试或重启）
*/
uint8_t DeviceState = 0;

// 通用错误码，用于记录最后发生的错误类型
int error;

// 当前 Opus 编码后的数据长度（字节）
int encode_len = 0;

// 当前 Opus 解码后得到的 PCM 数据长度（采样点数）
int decoded_len = 0;


// ========================================
// Opus 编解码器实例指针
// ========================================

OpusEncoder *encoder = NULL;    // Opus 编码器句柄，用于将 PCM 编码为 Opus 流
OpusDecoder *decoder = NULL;    // Opus 解码器句柄，用于将 Opus 流解码为 PCM


// ========================================
// AT 指令执行状态标志（用于控制模组通信流程）
// ========================================

bool AT_INIT_FLAG           = false;    // AT 模块初始化完成标志
bool AT_OK_FLAG             = false;    // 最近一次 AT 指令响应为 "OK"
bool AT_ERROR_FLAG          = false;    // 最近一次 AT 指令响应为 "ERROR"
bool AT_CWSTATE_FLAG        = false;    // Wi-Fi 连接状态设置完成标志
bool AT_WSOPENE_FLAG        = false;    // WebSocket 连接已打开
bool AT_WSCONNECTEDE_FLAG   = false;    // WebSocket 已成功连接到服务器
bool AT_WSSEND_FLAG         = false;    // WebSocket 数据发送成功标志


// ========================================
// 音频接收与解码控制标志
// ========================================

bool RECEIVING_OPUS_FLAG    = false;    // 正在接收 Opus 音频流（来自云端）


// ========================================
// 音频数据队列（用于线程间通信或中断与主循环的数据传递）
// ========================================

Queue audio_queue;           // 音频数据包队列，实现生产者-消费者模型
AudioPacket* packet;         // 指向音频数据包的指针，用于从队列中取出数据进行处理


// ========================================
// 函数声明（前置声明）
// ========================================
void AudioInputProcess(int offset , int size);
void AudioDecodeProcess(void);
void VirtualUSB(void);
void ChangeDeviceState(uint8_t flag);
bool CheckIsConnected(const char* target);
void AT_INIT(void);
bool WsSend_WaitTimeOut(int Cnt4Per100Us);


/**
 * @brief 主函数
 * @details 音频处理系统主程序，实现音频采集、编码、传输、解码和播放的完整流程
 *          同时集成网络通信和LED控制功能
 * @param None
 * @return int 程序退出状态
 */
int main(void)
{	
    /* 系统初始化 */
    log_init();                                     // 初始化日志系统
    QueueInit(&audio_queue);                        // 初始化音频数据队列
    
    /* Opus编解码器初始化 */
    encoder = opus_encoder_create(SAMPLE_RATE, CHANNEL, OPUS_APPLICATION_RESTRICTED_LOWDELAY, &error);
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(0));  // 设置编码器复杂度为0（最低延迟）
    decoder = opus_decoder_create(SAMPLE_RATE, CHANNEL, &error);

    /* WS2812 LED灯带初始化 */
    WS2812_RCC_INIT();      // 初始化WS2812所需的RCC时钟
    WS2812_GPIO_INIT();     // 初始化WS2812的GPIO引脚
    WS2812_PWM();           // 配置WS2812的PWM输出
    WS2812_DMA_INIT();      // 初始化WS2812的DMA传输

    /* WebSocket通信初始化 */
    UART7_RCC_INIT();                           // 初始化UART7的RCC时钟
    UART7_GPIO_INIT();                          // 初始化UART7的GPIO引脚
    UART7_INIT();                               // 初始化UART7串口配置
    UART7_NVIC_Configuration();                 // 配置UART7的NVIC中断
    UART7_DMA_Configuration(RxBuf[0], RX_BUFFER_SIZE);  // 配置UART7的DMA接收（双缓冲）
    AT_INIT();                                  // 初始化AT指令模块

    /* 麦克风采集初始化 */
    I2S2_RCC_INIT();                            // 初始化I2S2的RCC时钟
    I2S2_GPIO_INIT();                           // 初始化I2S2的GPIO引脚
    I2S2_I2S2_INIT();                           // 初始化I2S2接口配置
    I2S2_DMA_INIT(I2S_MIC_Buffer, PCM_BUFFER_SIZE*2*2); // 初始化I2S2的DMA传输（双缓冲）
    Mic_Start_Record();                         // 开始麦克风录音

    /* 扬声器播放初始化 */
    SPEAKER_RCC_INIT();                         // 初始化扬声器I2S的RCC时钟
    SPEAKER_GPIO_INIT();                        // 初始化扬声器的GPIO引脚
    SPEAKER_I2S_INIT();                         // 初始化扬声器I2S接口配置
    SPEAKER_DMA_INIT(PLAY_PCM_Buffer[ActivePlayingBuf], PCM_BUFFER_SIZE); // 初始化扬声器DMA传输
    SPEAKER_Start();                            // 启动扬声器播放
    
    /* 主循环前准备 */
    AT_ERROR_FLAG = false;                      // 清除AT指令错误标志
    
    /* 主循环 */
    while (1)
    {	
        /* 处理DMA半传输完成中断 - 前半缓冲区数据就绪 */
        if (DMA_GetFlagStatus(DMA_FLAG_HT4, DMA1))
        {
            DMA_ClearFlag(DMA_FLAG_HT4, DMA1);  // 清除半传输完成标志
            AudioInputProcess(0, 2);            // 处理前半缓冲区音频数据
        }
        
        /* 处理DMA传输完成中断 - 后半缓冲区数据就绪 */
        if (DMA_GetFlagStatus(DMA_FLAG_TC4, DMA1))
        {
            DMA_ClearFlag(DMA_FLAG_TC4, DMA1);  // 清除传输完成标志
            AudioInputProcess(1, 2);            // 处理后半缓冲区音频数据
        }
        
        /* 处理网络通信错误 */
        if (AT_ERROR_FLAG)
        {
            ChangeDeviceState(3);               // 改变设备状态到错误状态
            systick_delay_ms(2000);             // 延时2秒
            AT_INIT();                          // 重新初始化AT指令模块
            AT_ERROR_FLAG = false;              // 清除错误标志
        }		
		
        /* 音频解码处理 */
        AudioDecodeProcess();                   // 解码并播放接收到的音频数据
    }
}

/**
 * @brief 音频输入处理函数
 * @details 处理从麦克风采集的音频数据，进行编码并通过WebSocket发送
 * @param offset 缓冲区偏移量（0=前半缓冲区，1=后半缓冲区）
 * @param size 缓冲区大小参数
 */
void AudioInputProcess(int offset, int size)
{
    /* 检查队列状态：队列为空且未处于接收Opus数据状态时才进行编码发送 */
    if (QueueEmpty(&audio_queue) && !RECEIVING_OPUS_FLAG)
    {
        int cnt = 0;
        /* 处理指定偏移量的PCM数据缓冲区 */
        for (int i = PCM_BUFFER_SIZE * offset * size; i < PCM_BUFFER_SIZE * (offset + 1) * size; i += 2)
        {
            /* 数据转换和限幅处理 */
            Real_I2S_MIC_Data_32 = I2S_MIC_Buffer[i] * 15;
            if (Real_I2S_MIC_Data_32 > 32767) { Real_I2S_MIC_Data_32 = 32767; }
            if (Real_I2S_MIC_Data_32 < -32767) { Real_I2S_MIC_Data_32 = -32767; }
            Real_I2S_MIC_Buffer_4_OPUS[cnt] = Real_I2S_MIC_Data_32;
            cnt++;
        }
        
        /* Opus编码处理 */
        encode_len = opus_encode(encoder, Real_I2S_MIC_Buffer_4_OPUS, PCM_BUFFER_SIZE, OPUS_Buffer, sizeof(OPUS_Buffer));
        
        if (encode_len > 0)
        {
            /* 构造WebSocket发送AT指令 */
            char at_command[256];  
            sprintf(at_command, "AT+WSSEND=0,%d,2\r\n", encode_len);
            AT_WSSEND_FLAG = false;
            UART7_SendATCommand(at_command);	
            
            /* 等待发送准备就绪，超时5秒 */
            if (WsSend_WaitTimeOut(5000)) 
            { 
                systick_delay_ms(2);
                USART7_SendBuffer(OPUS_Buffer, encode_len);
            }
            else 
            { 
                AT_ERROR_FLAG = true;  /* 发送超时，设置错误标志 */
            }
        }
    }
    else    
    { 
        ChangeDeviceState(3);  /* 队列非空或正在接收数据，改变设备状态为错误 */
    }
}

/**
 * @brief 音频解码处理函数
 * @details 从队列中取出Opus数据包进行解码，并通过扬声器播放
 */
void AudioDecodeProcess(void)
{
    /* 检查解码器是否有效 */
    if (decoder == NULL) { AT_ERROR_FLAG = true; return; }
    
    /* 检查队列中是否有足够的数据需要解码（队列大小>18或有未处理帧） */
    if (QueueSize(&audio_queue) > 18 || (cnt_frame > 0 && !QueueEmpty(&audio_queue) && !RECEIVING_OPUS_FLAG))
    {
        /* 解码前两帧数据填充双缓冲区 */
        for (int k = 0; k < 2; k++)
        {
            packet = QueueFront(&audio_queue);
            decoded_len = opus_decode(decoder, packet->data, packet->length, SAVE_PLAY_PCM_Buffer, PCM_BUFFER_SIZE, 0);
            QueuePop(&audio_queue);
            if (decoded_len > 0) 
            { 
                /* 解码成功，应用音量控制并填充播放缓冲区 */
                for (int i = 0; i < PCM_BUFFER_SIZE; i++) 
                { 
                    PLAY_PCM_Buffer[k][i] = (int16_t)(SAVE_PLAY_PCM_Buffer[i] * VOLUME); 
                }
            }
        }
        
        /* 启动DMA播放 */
        ActivePlayingBuf = 0;
        DMA_SetCurrDataCounter(DMA2_CH2, PCM_BUFFER_SIZE);
        DMA_EnableChannel(DMA2_CH2, ENABLE);
        
        /* 处理队列中剩余的所有数据包 */
        while (!QueueEmpty(&audio_queue))
        {
            /* 等待当前缓冲区播放完成 */
            while (!DMA_GetFlagStatus(DMA_FLAG_TC2, DMA2));
            DMA_ClearFlag(DMA_FLAG_TC2, DMA2);
            DMA_EnableChannel(DMA2_CH2, DISABLE);
            
            /* 切换活动缓冲区 */
            LastActivePlayingBuf = ActivePlayingBuf;
            if (ActivePlayingBuf == 0) { ActivePlayingBuf = 1; }
            else { ActivePlayingBuf = 0; }
            
            /* 重新配置DMA使用新的缓冲区 */
            SPEAKER_DMA_INIT(PLAY_PCM_Buffer[ActivePlayingBuf], PCM_BUFFER_SIZE);
            DMA_SetCurrDataCounter(DMA2_CH2, PCM_BUFFER_SIZE);
            DMA_EnableChannel(DMA2_CH2, ENABLE);
            
            /* 解码下一帧数据 */
            packet = QueueFront(&audio_queue);
            decoded_len = opus_decode(decoder, packet->data, packet->length, SAVE_PLAY_PCM_Buffer, PCM_BUFFER_SIZE, 0);
            QueuePop(&audio_queue);
            
            if (decoded_len > 0) 
            { 
                /* 解码成功，填充刚刚播放完成的缓冲区 */
                for (int i = 0; i < PCM_BUFFER_SIZE; i++) 
                { 
                    PLAY_PCM_Buffer[LastActivePlayingBuf][i] = (int16_t)(SAVE_PLAY_PCM_Buffer[i] * VOLUME); 
                }
            }			
        }
    }
}

/**
 * @brief 虚拟USB初始化函数
 * @details 配置和初始化USB功能
 */
void VirtualUSB(void)
{ 
    uint32_t system_clock = 0;
    
    Set_System();
    USBFS_IO_Configure();
    USB_Interrupts_Config();
    
    /* 根据芯片型号设置系统时钟 */
#if defined (N32H473) || defined (N32H474)
    system_clock = SYSCLK_VALUE_192MHz;
#elif defined (N32H482) || defined (N32H487)
    system_clock = SYSCLK_VALUE_240MHz;
#endif
    
    /* USB配置和初始化 */
    if (USB_Config(system_clock) == SUCCESS)
    {
        USB_Init();
        /* 等待USB设备配置完成 */
        while (bDeviceState != CONFIGURED)
        {
            
        }
    }	
}

/**
 * @brief 改变设备状态函数
 * @details 根据状态标志改变WS2812 LED的颜色显示
 * @param flag 设备状态标志（0-4）
 */
void ChangeDeviceState(uint8_t flag)
{
    if (DeviceState != flag)
    {
        DeviceState = flag;
        /* 根据状态设置不同颜色 */
        if (flag == 0) { WS2812_SetColor(255, 255, 255); }      /* 白色 - 初始化状态 */
        else if (flag == 1) { WS2812_SetColor(0, 0, 255); }     /* 蓝色 - 连接成功 */
        else if (flag == 2) { WS2812_SetColor(0, 255, 0); }     /* 绿色 - 正常工作 */
        else if (flag == 3) { WS2812_SetColor(255, 0, 0); }     /* 红色 - 错误状态 */
        else if (flag == 4) { WS2812_SetColor(0, 0, 0); }       /* 黑色 - 关闭 */
        
        WS2812_Send();      /* 发送颜色数据到LED */
    }
}

/**
 * @brief 检查连接状态函数
 * @details 在接收缓冲区中查找特定目标字符串
 * @param target 要查找的目标字符串
 * @return bool 是否找到目标字符串
 */
bool CheckIsConnected(const char* target)
{
    return (strstr((char*)RxBuf, target) != NULL);
}

/**
 * @brief AT指令初始化函数（SoftAP版本）
 * @details 初始化WiFi模块，配置为SoftAP模式并连接到WebSocket服务器
 */
void AT_INIT(void)
{
    AT_INIT_FLAG = true;
    ChangeDeviceState(0);   /* 设置为初始化状态（白色） */
    
    /* 1. 测试AT指令响应 */
    UART7_SendATCommand("AT\r\n");	
    AT_OK_FLAG = false;
    while (!AT_OK_FLAG)
    {
        systick_delay_ms(500);
        if (AT_OK_FLAG) { break; }
        UART7_SendATCommand("AT\r\n");
    }
    
    /* 2. 配网过程 */
    systick_delay_ms(5000);
    AT_CWSTATE_FLAG = false;
    AT_OK_FLAG = false;
    UART7_SendATCommand("AT+CWSTATE?\r\n");	
    while (!AT_CWSTATE_FLAG)
    {
        systick_delay_ms(500);
        if (AT_OK_FLAG) { break; }
        UART7_SendATCommand("AT+CWSTATE?\r\n");
    }
    
    /* 如果未配置，进行完整配置 */
    if (!AT_CWSTATE_FLAG)
    {
        /* 恢复出厂设置 */
        UART7_SendATCommand("AT+RESTORE\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+RESTORE\r\n");
        }	
        
        /* 设置WiFi模式为AP+Station */
        UART7_SendATCommand("AT+CWMODE=3\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+CWMODE=3\r\n");
        }		
        
        /* 配置SoftAP参数 */
        UART7_SendATCommand("AT+CWSAP=\"AITalk_SoftAP\",\"PULAN\",11,0,3\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+CWSAP=\"AITalk_SoftAP\",\"PULAN\",11,0,3\r\n");	
        }			
        
        /* 启用多连接 */
        UART7_SendATCommand("AT+CIPMUX=1\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+CIPMUX=1\r\n");
        }		
        
        /* 启动Web服务器 */
        UART7_SendATCommand("AT+WEBSERVER=1,80,60\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+WEBSERVER=1,80,60\r\n");
        }		
        
        /* 再次检查WiFi状态 */
        AT_CWSTATE_FLAG = false;
        UART7_SendATCommand("AT+CWSTATE?\r\n");	
        while (!AT_CWSTATE_FLAG)
        {
            systick_delay_ms(500);
            if (AT_CWSTATE_FLAG) { break; }
            UART7_SendATCommand("AT+CWSTATE?\r\n");
        }		
    }
    
    /* 3. 和服务端WebSocket握手 */
    AT_OK_FLAG = false;
    AT_WSOPENE_FLAG = false;
    UART7_SendATCommand("AT+WSOPEN?\r\n");
    while (!AT_OK_FLAG)
    {
        systick_delay_ms(500);
        if (AT_OK_FLAG) { break; }
        UART7_SendATCommand("AT+WSOPEN?\r\n");
    }
    
    /* 如果WebSocket未打开，进行配置和连接 */
    if (!AT_WSOPENE_FLAG)
    {
        /* 配置WebSocket参数 */
        AT_OK_FLAG = false;
        UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
        }
        
        /* 连接到WebSocket服务器 */
        AT_WSCONNECTEDE_FLAG = false;
        systick_delay_ms(500);
        UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
        while (!AT_WSCONNECTEDE_FLAG) // 等待连接成功响应
        {
            systick_delay_ms(500);
            if (AT_WSCONNECTEDE_FLAG) { break; }
            UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
        }	
    }
    
    AT_INIT_FLAG = false;
    ChangeDeviceState(1);   /* 设置为连接成功状态（蓝色） */
    systick_delay_ms(1000);
}


///**
// * @brief AT指令初始化函数（BluFi版本）
// * @details 初始化WiFi模块，配置为BluFi配网模式并连接到WebSocket服务器
// */
//void AT_INIT(void)
//{
//    AT_INIT_FLAG = true;
//    ChangeDeviceState(0);   /* 设置为初始化状态（白色） */
//    
//    /* 1. 测试AT指令响应 */
//    UART7_SendATCommand("AT\r\n");	
//    AT_OK_FLAG = false;
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT\r\n");
//    }
//    
//    /* 2. BluFi配网过程 */
//    // 注释掉的Station模式配置（保留作为参考）
//    // AT_OK_FLAG = false;
//    // UART7_SendATCommand("AT+CWMODE=0\r\n");	
//    // while (!AT_OK_FLAG)
//    // {
//    //     systick_delay_ms(500);
//    //     if (AT_OK_FLAG) { break; }
//    //     UART7_SendATCommand("AT+CWMODE=0\r\n");	
//    // }
//    
//    /* 禁用BluFi功能（先确保关闭） */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    }
//    
//    /* 设置BluFi设备名称 */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFINAME=\"AutoAITalk BLUFI\"\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFINAME=\"AI BOT BLUFI\"\r\n");
//    }
//    
//    /* 启用BluFi功能 */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFI=1\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFI=1\r\n");
//    }
//    
//    /* 3. 等待WIFI连接完成 */
//    AT_CWSTATE_FLAG = false;
//    UART7_SendATCommand("AT+CWSTATE?\r\n");	
//    while (!AT_CWSTATE_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_CWSTATE_FLAG) { break; }
//        UART7_SendATCommand("AT+CWSTATE?\r\n");
//    }
//    
//    /* 配网完成后禁用BluFi功能 */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    }
//    
//    /* 4. 和服务端WebSocket握手 */
//    AT_OK_FLAG = false;
//    AT_WSOPENE_FLAG = false;
//    UART7_SendATCommand("AT+WSOPEN?\r\n");
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+WSOPEN?\r\n");
//    }
//    
//    /* 如果WebSocket未打开，进行配置和连接 */
//    if (!AT_WSOPENE_FLAG)
//    {
//        /* 配置WebSocket参数 */
//        AT_OK_FLAG = false;
//        UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
//        while (!AT_OK_FLAG)
//        {
//            systick_delay_ms(500);
//            if (AT_OK_FLAG) { break; }
//            UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
//        }
//        
//        /* 连接到WebSocket服务器 */
//        AT_WSCONNECTEDE_FLAG = false;
//        systick_delay_ms(500);
//        // 两个服务器地址选项，根据需求使用
//        //UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
//        UART7_SendATCommand("AT+WSOPEN=0,\"ws://8.148.191.21:23530//\"\r\n");	
//        while (!AT_WSCONNECTEDE_FLAG) // 等待连接成功响应 +WS_CONNECTED
//        {
//            systick_delay_ms(500);
//            if (AT_WSCONNECTEDE_FLAG) { break; }
//            //UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
//            UART7_SendATCommand("AT+WSOPEN=0,\"ws://8.148.191.21:23530//\"\r\n");
//        }	
//    }
//    
//    AT_INIT_FLAG = false;
//    ChangeDeviceState(1);   /* 设置为连接成功状态（蓝色） */
//    systick_delay_ms(1000);
//}

/**
 * @brief WebSocket发送等待超时函数
 * @details 等待WebSocket发送准备就绪，带有超时机制
 * @param Cnt4Per100Us 超时计数（每100微秒计数一次）
 * @return bool 是否在超时前准备就绪
 */
bool WsSend_WaitTimeOut(int Cnt4Per100Us)
{
    int cnt = 0;
    while (cnt < Cnt4Per100Us)
    {
        if (AT_WSSEND_FLAG) { return true; }  /* 发送准备就绪 */
        systick_delay_us(100);
        cnt++;
    }
    return false;  /* 超时未就绪 */
}