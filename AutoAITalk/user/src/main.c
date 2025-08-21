#include "main.h"

// ========================================
// ��Ƶ��������
// ========================================

#define SAMPLE_RATE 16000  // �����ʣ�16kHz����������������
#define CHANNEL 1 // ����������������Mono��
#define VOLUME 0.15 // ��Ƶ���棨������ϵ�������ڵ���¼��������С��0.0 ~ 1.0��

#define PCM_BUFFER_SIZE 960 // PCM ���ݻ�������С����������������Ӧ 60ms ��Ƶ��960/16000 �� 0.06s��
#define OPUS_BUFFER_SIZE 960 // PCM ���ݻ�������С����������������Ӧ 60ms ��Ƶ��960/16000 �� 0.06s��


// ========================================
// ¼����ػ����������
// ========================================

// ���ڴ洢������ɣ�����Opus�����PCM����
int16_t Real_I2S_MIC_Buffer_4_OPUS[PCM_BUFFER_SIZE];

// ���ڴ洢�����������Ŵ���ԭʼPCM����
int32_t Real_I2S_MIC_Data_32;

// �洢������ Opus ��Ƶ���ݰ�
uint8_t OPUS_Buffer[OPUS_BUFFER_SIZE];

// ����˫������Ƶ� I2S ��˷����뻺����
int16_t I2S_MIC_Buffer[PCM_BUFFER_SIZE*2*2];


// ========================================
// ������ػ�������״̬����
// ========================================

// ���� WebSocket ����� Opus ���ݰ�������
uint8_t WS_OPUS_Buffer[300];

// ˫���� PCM ���Ż������������������������Ͳ��ţ�
int16_t PLAY_PCM_Buffer[2][PCM_BUFFER_SIZE];

// ����Opus�����PCM����
int16_t SAVE_PLAY_PCM_Buffer[PCM_BUFFER_SIZE];

// ��ǰ���ڱ����Ĳ��Ż���������
uint8_t ActivePlayingBuf = 0;

// ��һ����������Ĳ��Ż������������� DAC ����ʹ��
uint8_t LastActivePlayingBuf = 0;


// ========================================
// WebSocket ��ͨ����ػ�����
// ========================================

// ÿ������֡������ֽ�����Լ 1.5KB�������� WebSocket ���ݽ���
uint32_t RX_BUFFER_SIZE = 1512;

// ˫������ջ��ƣ������������������ WebSocket ����
uint8_t RxBuf[2][1512];

// ��ʱ������������������װ����� WebSocket ����֡
uint8_t WS_DATA_BUF[1024];

// ��ǰ���ջ��������ѽ��յ��ֽ���
uint16_t RxCnt = 0;

// WS_DATA_BUF ����д������ݳ���
uint16_t WsBufCnt = 0;

// ��ǰ����д��Ľ��ջ�����������0 �� 1��
uint8_t ActiveBuf = 0;

// ���յ�����Ƶ֡������
uint16_t cnt_frame = 0;


// ========================================
// �豸����״̬��
// ========================================

/*
DeviceState - �豸״̬��ʶ��״̬����
0: ��ʼ���У��ȴ����硢ģ�������
1: ���ڽ�����Ƶ������˷�ɼ����ϴ���
2: ���������Ƶ�������ƶ˷��ص������ظ���
3: ���ִ��������Ի�������
*/
uint8_t DeviceState = 0;

// ͨ�ô����룬���ڼ�¼������Ĵ�������
int error;

// ��ǰ Opus ���������ݳ��ȣ��ֽڣ�
int encode_len = 0;

// ��ǰ Opus �����õ��� PCM ���ݳ��ȣ�����������
int decoded_len = 0;


// ========================================
// Opus �������ʵ��ָ��
// ========================================

OpusEncoder *encoder = NULL;    // Opus ��������������ڽ� PCM ����Ϊ Opus ��
OpusDecoder *decoder = NULL;    // Opus ��������������ڽ� Opus ������Ϊ PCM


// ========================================
// AT ָ��ִ��״̬��־�����ڿ���ģ��ͨ�����̣�
// ========================================

bool AT_INIT_FLAG           = false;    // AT ģ���ʼ����ɱ�־
bool AT_OK_FLAG             = false;    // ���һ�� AT ָ����ӦΪ "OK"
bool AT_ERROR_FLAG          = false;    // ���һ�� AT ָ����ӦΪ "ERROR"
bool AT_CWSTATE_FLAG        = false;    // Wi-Fi ����״̬������ɱ�־
bool AT_WSOPENE_FLAG        = false;    // WebSocket �����Ѵ�
bool AT_WSCONNECTEDE_FLAG   = false;    // WebSocket �ѳɹ����ӵ�������
bool AT_WSSEND_FLAG         = false;    // WebSocket ���ݷ��ͳɹ���־


// ========================================
// ��Ƶ�����������Ʊ�־
// ========================================

bool RECEIVING_OPUS_FLAG    = false;    // ���ڽ��� Opus ��Ƶ���������ƶˣ�


// ========================================
// ��Ƶ���ݶ��У������̼߳�ͨ�Ż��ж�����ѭ�������ݴ��ݣ�
// ========================================

Queue audio_queue;           // ��Ƶ���ݰ����У�ʵ��������-������ģ��
AudioPacket* packet;         // ָ����Ƶ���ݰ���ָ�룬���ڴӶ�����ȡ�����ݽ��д���


// ========================================
// ����������ǰ��������
// ========================================
void AudioInputProcess(int offset , int size);
void AudioDecodeProcess(void);
void VirtualUSB(void);
void ChangeDeviceState(uint8_t flag);
bool CheckIsConnected(const char* target);
void AT_INIT(void);
bool WsSend_WaitTimeOut(int Cnt4Per100Us);


/**
 * @brief ������
 * @details ��Ƶ����ϵͳ������ʵ����Ƶ�ɼ������롢���䡢����Ͳ��ŵ���������
 *          ͬʱ��������ͨ�ź�LED���ƹ���
 * @param None
 * @return int �����˳�״̬
 */
int main(void)
{	
    /* ϵͳ��ʼ�� */
    log_init();                                     // ��ʼ����־ϵͳ
    QueueInit(&audio_queue);                        // ��ʼ����Ƶ���ݶ���
    
    /* Opus���������ʼ�� */
    encoder = opus_encoder_create(SAMPLE_RATE, CHANNEL, OPUS_APPLICATION_RESTRICTED_LOWDELAY, &error);
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(0));  // ���ñ��������Ӷ�Ϊ0������ӳ٣�
    decoder = opus_decoder_create(SAMPLE_RATE, CHANNEL, &error);

    /* WS2812 LED�ƴ���ʼ�� */
    WS2812_RCC_INIT();      // ��ʼ��WS2812�����RCCʱ��
    WS2812_GPIO_INIT();     // ��ʼ��WS2812��GPIO����
    WS2812_PWM();           // ����WS2812��PWM���
    WS2812_DMA_INIT();      // ��ʼ��WS2812��DMA����

    /* WebSocketͨ�ų�ʼ�� */
    UART7_RCC_INIT();                           // ��ʼ��UART7��RCCʱ��
    UART7_GPIO_INIT();                          // ��ʼ��UART7��GPIO����
    UART7_INIT();                               // ��ʼ��UART7��������
    UART7_NVIC_Configuration();                 // ����UART7��NVIC�ж�
    UART7_DMA_Configuration(RxBuf[0], RX_BUFFER_SIZE);  // ����UART7��DMA���գ�˫���壩
    AT_INIT();                                  // ��ʼ��ATָ��ģ��

    /* ��˷�ɼ���ʼ�� */
    I2S2_RCC_INIT();                            // ��ʼ��I2S2��RCCʱ��
    I2S2_GPIO_INIT();                           // ��ʼ��I2S2��GPIO����
    I2S2_I2S2_INIT();                           // ��ʼ��I2S2�ӿ�����
    I2S2_DMA_INIT(I2S_MIC_Buffer, PCM_BUFFER_SIZE*2*2); // ��ʼ��I2S2��DMA���䣨˫���壩
    Mic_Start_Record();                         // ��ʼ��˷�¼��

    /* ���������ų�ʼ�� */
    SPEAKER_RCC_INIT();                         // ��ʼ��������I2S��RCCʱ��
    SPEAKER_GPIO_INIT();                        // ��ʼ����������GPIO����
    SPEAKER_I2S_INIT();                         // ��ʼ��������I2S�ӿ�����
    SPEAKER_DMA_INIT(PLAY_PCM_Buffer[ActivePlayingBuf], PCM_BUFFER_SIZE); // ��ʼ��������DMA����
    SPEAKER_Start();                            // ��������������
    
    /* ��ѭ��ǰ׼�� */
    AT_ERROR_FLAG = false;                      // ���ATָ������־
    
    /* ��ѭ�� */
    while (1)
    {	
        /* ����DMA�봫������ж� - ǰ�뻺�������ݾ��� */
        if (DMA_GetFlagStatus(DMA_FLAG_HT4, DMA1))
        {
            DMA_ClearFlag(DMA_FLAG_HT4, DMA1);  // ����봫����ɱ�־
            AudioInputProcess(0, 2);            // ����ǰ�뻺������Ƶ����
        }
        
        /* ����DMA��������ж� - ��뻺�������ݾ��� */
        if (DMA_GetFlagStatus(DMA_FLAG_TC4, DMA1))
        {
            DMA_ClearFlag(DMA_FLAG_TC4, DMA1);  // ���������ɱ�־
            AudioInputProcess(1, 2);            // �����뻺������Ƶ����
        }
        
        /* ��������ͨ�Ŵ��� */
        if (AT_ERROR_FLAG)
        {
            ChangeDeviceState(3);               // �ı��豸״̬������״̬
            systick_delay_ms(2000);             // ��ʱ2��
            AT_INIT();                          // ���³�ʼ��ATָ��ģ��
            AT_ERROR_FLAG = false;              // ��������־
        }		
		
        /* ��Ƶ���봦�� */
        AudioDecodeProcess();                   // ���벢���Ž��յ�����Ƶ����
    }
}

/**
 * @brief ��Ƶ���봦����
 * @details �������˷�ɼ�����Ƶ���ݣ����б��벢ͨ��WebSocket����
 * @param offset ������ƫ������0=ǰ�뻺������1=��뻺������
 * @param size ��������С����
 */
void AudioInputProcess(int offset, int size)
{
    /* ������״̬������Ϊ����δ���ڽ���Opus����״̬ʱ�Ž��б��뷢�� */
    if (QueueEmpty(&audio_queue) && !RECEIVING_OPUS_FLAG)
    {
        int cnt = 0;
        /* ����ָ��ƫ������PCM���ݻ����� */
        for (int i = PCM_BUFFER_SIZE * offset * size; i < PCM_BUFFER_SIZE * (offset + 1) * size; i += 2)
        {
            /* ����ת�����޷����� */
            Real_I2S_MIC_Data_32 = I2S_MIC_Buffer[i] * 15;
            if (Real_I2S_MIC_Data_32 > 32767) { Real_I2S_MIC_Data_32 = 32767; }
            if (Real_I2S_MIC_Data_32 < -32767) { Real_I2S_MIC_Data_32 = -32767; }
            Real_I2S_MIC_Buffer_4_OPUS[cnt] = Real_I2S_MIC_Data_32;
            cnt++;
        }
        
        /* Opus���봦�� */
        encode_len = opus_encode(encoder, Real_I2S_MIC_Buffer_4_OPUS, PCM_BUFFER_SIZE, OPUS_Buffer, sizeof(OPUS_Buffer));
        
        if (encode_len > 0)
        {
            /* ����WebSocket����ATָ�� */
            char at_command[256];  
            sprintf(at_command, "AT+WSSEND=0,%d,2\r\n", encode_len);
            AT_WSSEND_FLAG = false;
            UART7_SendATCommand(at_command);	
            
            /* �ȴ�����׼����������ʱ5�� */
            if (WsSend_WaitTimeOut(5000)) 
            { 
                systick_delay_ms(2);
                USART7_SendBuffer(OPUS_Buffer, encode_len);
            }
            else 
            { 
                AT_ERROR_FLAG = true;  /* ���ͳ�ʱ�����ô����־ */
            }
        }
    }
    else    
    { 
        ChangeDeviceState(3);  /* ���зǿջ����ڽ������ݣ��ı��豸״̬Ϊ���� */
    }
}

/**
 * @brief ��Ƶ���봦����
 * @details �Ӷ�����ȡ��Opus���ݰ����н��룬��ͨ������������
 */
void AudioDecodeProcess(void)
{
    /* ���������Ƿ���Ч */
    if (decoder == NULL) { AT_ERROR_FLAG = true; return; }
    
    /* ���������Ƿ����㹻��������Ҫ���루���д�С>18����δ����֡�� */
    if (QueueSize(&audio_queue) > 18 || (cnt_frame > 0 && !QueueEmpty(&audio_queue) && !RECEIVING_OPUS_FLAG))
    {
        /* ����ǰ��֡�������˫������ */
        for (int k = 0; k < 2; k++)
        {
            packet = QueueFront(&audio_queue);
            decoded_len = opus_decode(decoder, packet->data, packet->length, SAVE_PLAY_PCM_Buffer, PCM_BUFFER_SIZE, 0);
            QueuePop(&audio_queue);
            if (decoded_len > 0) 
            { 
                /* ����ɹ���Ӧ���������Ʋ���䲥�Ż����� */
                for (int i = 0; i < PCM_BUFFER_SIZE; i++) 
                { 
                    PLAY_PCM_Buffer[k][i] = (int16_t)(SAVE_PLAY_PCM_Buffer[i] * VOLUME); 
                }
            }
        }
        
        /* ����DMA���� */
        ActivePlayingBuf = 0;
        DMA_SetCurrDataCounter(DMA2_CH2, PCM_BUFFER_SIZE);
        DMA_EnableChannel(DMA2_CH2, ENABLE);
        
        /* ���������ʣ����������ݰ� */
        while (!QueueEmpty(&audio_queue))
        {
            /* �ȴ���ǰ������������� */
            while (!DMA_GetFlagStatus(DMA_FLAG_TC2, DMA2));
            DMA_ClearFlag(DMA_FLAG_TC2, DMA2);
            DMA_EnableChannel(DMA2_CH2, DISABLE);
            
            /* �л�������� */
            LastActivePlayingBuf = ActivePlayingBuf;
            if (ActivePlayingBuf == 0) { ActivePlayingBuf = 1; }
            else { ActivePlayingBuf = 0; }
            
            /* ��������DMAʹ���µĻ����� */
            SPEAKER_DMA_INIT(PLAY_PCM_Buffer[ActivePlayingBuf], PCM_BUFFER_SIZE);
            DMA_SetCurrDataCounter(DMA2_CH2, PCM_BUFFER_SIZE);
            DMA_EnableChannel(DMA2_CH2, ENABLE);
            
            /* ������һ֡���� */
            packet = QueueFront(&audio_queue);
            decoded_len = opus_decode(decoder, packet->data, packet->length, SAVE_PLAY_PCM_Buffer, PCM_BUFFER_SIZE, 0);
            QueuePop(&audio_queue);
            
            if (decoded_len > 0) 
            { 
                /* ����ɹ������ող�����ɵĻ����� */
                for (int i = 0; i < PCM_BUFFER_SIZE; i++) 
                { 
                    PLAY_PCM_Buffer[LastActivePlayingBuf][i] = (int16_t)(SAVE_PLAY_PCM_Buffer[i] * VOLUME); 
                }
            }			
        }
    }
}

/**
 * @brief ����USB��ʼ������
 * @details ���úͳ�ʼ��USB����
 */
void VirtualUSB(void)
{ 
    uint32_t system_clock = 0;
    
    Set_System();
    USBFS_IO_Configure();
    USB_Interrupts_Config();
    
    /* ����оƬ�ͺ�����ϵͳʱ�� */
#if defined (N32H473) || defined (N32H474)
    system_clock = SYSCLK_VALUE_192MHz;
#elif defined (N32H482) || defined (N32H487)
    system_clock = SYSCLK_VALUE_240MHz;
#endif
    
    /* USB���úͳ�ʼ�� */
    if (USB_Config(system_clock) == SUCCESS)
    {
        USB_Init();
        /* �ȴ�USB�豸������� */
        while (bDeviceState != CONFIGURED)
        {
            
        }
    }	
}

/**
 * @brief �ı��豸״̬����
 * @details ����״̬��־�ı�WS2812 LED����ɫ��ʾ
 * @param flag �豸״̬��־��0-4��
 */
void ChangeDeviceState(uint8_t flag)
{
    if (DeviceState != flag)
    {
        DeviceState = flag;
        /* ����״̬���ò�ͬ��ɫ */
        if (flag == 0) { WS2812_SetColor(255, 255, 255); }      /* ��ɫ - ��ʼ��״̬ */
        else if (flag == 1) { WS2812_SetColor(0, 0, 255); }     /* ��ɫ - ���ӳɹ� */
        else if (flag == 2) { WS2812_SetColor(0, 255, 0); }     /* ��ɫ - �������� */
        else if (flag == 3) { WS2812_SetColor(255, 0, 0); }     /* ��ɫ - ����״̬ */
        else if (flag == 4) { WS2812_SetColor(0, 0, 0); }       /* ��ɫ - �ر� */
        
        WS2812_Send();      /* ������ɫ���ݵ�LED */
    }
}

/**
 * @brief �������״̬����
 * @details �ڽ��ջ������в����ض�Ŀ���ַ���
 * @param target Ҫ���ҵ�Ŀ���ַ���
 * @return bool �Ƿ��ҵ�Ŀ���ַ���
 */
bool CheckIsConnected(const char* target)
{
    return (strstr((char*)RxBuf, target) != NULL);
}

/**
 * @brief ATָ���ʼ��������SoftAP�汾��
 * @details ��ʼ��WiFiģ�飬����ΪSoftAPģʽ�����ӵ�WebSocket������
 */
void AT_INIT(void)
{
    AT_INIT_FLAG = true;
    ChangeDeviceState(0);   /* ����Ϊ��ʼ��״̬����ɫ�� */
    
    /* 1. ����ATָ����Ӧ */
    UART7_SendATCommand("AT\r\n");	
    AT_OK_FLAG = false;
    while (!AT_OK_FLAG)
    {
        systick_delay_ms(500);
        if (AT_OK_FLAG) { break; }
        UART7_SendATCommand("AT\r\n");
    }
    
    /* 2. �������� */
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
    
    /* ���δ���ã������������� */
    if (!AT_CWSTATE_FLAG)
    {
        /* �ָ��������� */
        UART7_SendATCommand("AT+RESTORE\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+RESTORE\r\n");
        }	
        
        /* ����WiFiģʽΪAP+Station */
        UART7_SendATCommand("AT+CWMODE=3\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+CWMODE=3\r\n");
        }		
        
        /* ����SoftAP���� */
        UART7_SendATCommand("AT+CWSAP=\"AITalk_SoftAP\",\"PULAN\",11,0,3\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+CWSAP=\"AITalk_SoftAP\",\"PULAN\",11,0,3\r\n");	
        }			
        
        /* ���ö����� */
        UART7_SendATCommand("AT+CIPMUX=1\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+CIPMUX=1\r\n");
        }		
        
        /* ����Web������ */
        UART7_SendATCommand("AT+WEBSERVER=1,80,60\r\n");	
        AT_OK_FLAG = false;
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+WEBSERVER=1,80,60\r\n");
        }		
        
        /* �ٴμ��WiFi״̬ */
        AT_CWSTATE_FLAG = false;
        UART7_SendATCommand("AT+CWSTATE?\r\n");	
        while (!AT_CWSTATE_FLAG)
        {
            systick_delay_ms(500);
            if (AT_CWSTATE_FLAG) { break; }
            UART7_SendATCommand("AT+CWSTATE?\r\n");
        }		
    }
    
    /* 3. �ͷ����WebSocket���� */
    AT_OK_FLAG = false;
    AT_WSOPENE_FLAG = false;
    UART7_SendATCommand("AT+WSOPEN?\r\n");
    while (!AT_OK_FLAG)
    {
        systick_delay_ms(500);
        if (AT_OK_FLAG) { break; }
        UART7_SendATCommand("AT+WSOPEN?\r\n");
    }
    
    /* ���WebSocketδ�򿪣��������ú����� */
    if (!AT_WSOPENE_FLAG)
    {
        /* ����WebSocket���� */
        AT_OK_FLAG = false;
        UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
        while (!AT_OK_FLAG)
        {
            systick_delay_ms(500);
            if (AT_OK_FLAG) { break; }
            UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
        }
        
        /* ���ӵ�WebSocket������ */
        AT_WSCONNECTEDE_FLAG = false;
        systick_delay_ms(500);
        UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
        while (!AT_WSCONNECTEDE_FLAG) // �ȴ����ӳɹ���Ӧ
        {
            systick_delay_ms(500);
            if (AT_WSCONNECTEDE_FLAG) { break; }
            UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
        }	
    }
    
    AT_INIT_FLAG = false;
    ChangeDeviceState(1);   /* ����Ϊ���ӳɹ�״̬����ɫ�� */
    systick_delay_ms(1000);
}


///**
// * @brief ATָ���ʼ��������BluFi�汾��
// * @details ��ʼ��WiFiģ�飬����ΪBluFi����ģʽ�����ӵ�WebSocket������
// */
//void AT_INIT(void)
//{
//    AT_INIT_FLAG = true;
//    ChangeDeviceState(0);   /* ����Ϊ��ʼ��״̬����ɫ�� */
//    
//    /* 1. ����ATָ����Ӧ */
//    UART7_SendATCommand("AT\r\n");	
//    AT_OK_FLAG = false;
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT\r\n");
//    }
//    
//    /* 2. BluFi�������� */
//    // ע�͵���Stationģʽ���ã�������Ϊ�ο���
//    // AT_OK_FLAG = false;
//    // UART7_SendATCommand("AT+CWMODE=0\r\n");	
//    // while (!AT_OK_FLAG)
//    // {
//    //     systick_delay_ms(500);
//    //     if (AT_OK_FLAG) { break; }
//    //     UART7_SendATCommand("AT+CWMODE=0\r\n");	
//    // }
//    
//    /* ����BluFi���ܣ���ȷ���رգ� */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    }
//    
//    /* ����BluFi�豸���� */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFINAME=\"AutoAITalk BLUFI\"\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFINAME=\"AI BOT BLUFI\"\r\n");
//    }
//    
//    /* ����BluFi���� */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFI=1\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFI=1\r\n");
//    }
//    
//    /* 3. �ȴ�WIFI������� */
//    AT_CWSTATE_FLAG = false;
//    UART7_SendATCommand("AT+CWSTATE?\r\n");	
//    while (!AT_CWSTATE_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_CWSTATE_FLAG) { break; }
//        UART7_SendATCommand("AT+CWSTATE?\r\n");
//    }
//    
//    /* ������ɺ����BluFi���� */
//    AT_OK_FLAG = false;
//    UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    while (!AT_OK_FLAG)
//    {
//        systick_delay_ms(500);
//        if (AT_OK_FLAG) { break; }
//        UART7_SendATCommand("AT+BLUFI=0\r\n");	
//    }
//    
//    /* 4. �ͷ����WebSocket���� */
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
//    /* ���WebSocketδ�򿪣��������ú����� */
//    if (!AT_WSOPENE_FLAG)
//    {
//        /* ����WebSocket���� */
//        AT_OK_FLAG = false;
//        UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
//        while (!AT_OK_FLAG)
//        {
//            systick_delay_ms(500);
//            if (AT_OK_FLAG) { break; }
//            UART7_SendATCommand("AT+WSCFG=0,30,600,4096\r\n");
//        }
//        
//        /* ���ӵ�WebSocket������ */
//        AT_WSCONNECTEDE_FLAG = false;
//        systick_delay_ms(500);
//        // ������������ַѡ���������ʹ��
//        //UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
//        UART7_SendATCommand("AT+WSOPEN=0,\"ws://8.148.191.21:23530//\"\r\n");	
//        while (!AT_WSCONNECTEDE_FLAG) // �ȴ����ӳɹ���Ӧ +WS_CONNECTED
//        {
//            systick_delay_ms(500);
//            if (AT_WSCONNECTEDE_FLAG) { break; }
//            //UART7_SendATCommand("AT+WSOPEN=0,\"ws://5.181.225.4:23530//\"\r\n");	
//            UART7_SendATCommand("AT+WSOPEN=0,\"ws://8.148.191.21:23530//\"\r\n");
//        }	
//    }
//    
//    AT_INIT_FLAG = false;
//    ChangeDeviceState(1);   /* ����Ϊ���ӳɹ�״̬����ɫ�� */
//    systick_delay_ms(1000);
//}

/**
 * @brief WebSocket���͵ȴ���ʱ����
 * @details �ȴ�WebSocket����׼�����������г�ʱ����
 * @param Cnt4Per100Us ��ʱ������ÿ100΢�����һ�Σ�
 * @return bool �Ƿ��ڳ�ʱǰ׼������
 */
bool WsSend_WaitTimeOut(int Cnt4Per100Us)
{
    int cnt = 0;
    while (cnt < Cnt4Per100Us)
    {
        if (AT_WSSEND_FLAG) { return true; }  /* ����׼������ */
        systick_delay_us(100);
        cnt++;
    }
    return false;  /* ��ʱδ���� */
}