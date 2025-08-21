# AutoAITalk

## 简介

　　AutoAITalk 是基于国民技术 N32H482REL7 微控制器开发的嵌入式语音交互平台，系统性地构建了一套面向 ARM Cortex-M 系列微控制器（如 N32、STM32 等）的轻量化、高实时性语音交互解决方案。针对当前主流开源语音平台在硬件兼容性、系统资源占用和开发透明度方面的不足，本项目通过深度优化嵌入式驱动层、通信协议栈与音频处理流程，实现了在资源受限设备上的高效运行。


　　本平台专为智能硬件开发者设计，具备高实时性交互、语音控制其他设备和个性化 AI 角色定制等核心功能，既满足教学研究需求，又能直接应用于商业级产品开发，为 AI 玩具、智能家居等领域提供一个高效、低成本、灵活且易于集成的解决方案。开发者可基于本项目的技术框架与实现思路，灵活拓展出多种智能音频应用，例如：便携式录音设备、智能蓝牙/WIFI音箱、智能玩具，等等。

　　详细的技术文档见<a href="http://www.planrobots.com/" target="_blank">普蓝机器人官网</a>

## 基本框架

　　系统在设备端通过 I2S 数字音频接口与 INMP441 MEMS 麦克风无缝对接，以确保实时性与高保真的语音信号采集。在软件层面，经过声道数据提取、增益调整等预处理后，利用 OPUS 编码器对 PCM 语音数据进行高效压缩，以优化传输效率。

　　随后，经由 ESPC2-12-N4 无线通信模组及 WebSocket 协议，系统能够将压缩后的 OPUS 语音数据实时上传至服务端。在服务端，首先运用 VAD 技术识别人类语音活动，再通过 ASR 技术将语音转换为文本。接下来，文本信息被送入大型语言模型进行深度语义理解和响应生成，生成的文本回复通过 TTS 技术转化为语音输出，并再次使用 Opus 编码器进行压缩，然后回传至设备端。

　　在设备端，接收到的音频数据经解码恢复为 PCM 格式的音流，PCM 音频数据通过 I2S 接口传输至 MAX98357A D 类功放芯片，经功率放大后驱动扬声器输出高质量语音。

　　系统还配备了一款专门设计的 APP， 该 APP 不仅支持用户实时查看语音交互记录，还提供了便捷的界面以供用户调整模型的各项参数，如音色、对话风格以及人物设定等，以满足个性化定制的需求。

<center class ='img'><img src=".\Images\voice interaction process.png"> </center>

<!-- ![DR100实物图](./image/dr100.png) -->

## 开发资源

### 嵌入式开发资源

<table> 
    <tr>
        <th style="text-align:center; vertical-align:middle; width:12%;">类别</th>
        <th style="text-align:center; vertical-align:middle;">名称</th>
        <th style="text-align:center; vertical-align:middle; width:22%;">获取方式</th>
        <th style="text-align:center; vertical-align:middle;">备注</th>
    </tr>
    <tr>
        <td rowspan="4" style="text-align:center; vertical-align:middle;">开发工具</td>
        <td style="text-align:center; vertical-align:middle;">Keil MDK</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.keil.com/" target="_blank">Keil 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">必备，推荐版本 v5.35</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">ESP-IDF</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://docs.espressif.com/projects/esp-idf/zh_CN/release-v5.4/esp32c2/get-started/windows-setup.html" target="_blank">乐鑫官方安装教程</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">必备，用于本地配置、编译并烧录 AT 固件，推荐版本 v5.4</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">SSCOM</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://gitcode.com/open-source-toolkit/b21c1" target="_blank">SSCOM5.12</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">选用，串口调试助手，用于调试设备</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">PulseView</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://sigrok.org/wiki/PulseView" target="_blank">PulseView 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">选用，逻辑分析仪，用于调试串口、SPI、PWM 等通信接口</td>
    </tr>
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="4" style="text-align:center; vertical-align:middle;">组件</td>
        <td style="text-align:center; vertical-align:middle;">N32H48x_DFP.1.0.0.pack</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.nationstech.com/support/dow/" target="_blank">国民技术官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">芯片支持包</td>
    </tr>   
    <tr>
        <td style="text-align:center; vertical-align:middle;">开发库</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.nationstech.com/support/dow/" target="_blank">国民技术官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">标准库</td>
    </tr>  
     <tr>
        <td style="text-align:center; vertical-align:middle;">Opus 库</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://opus-codec.org/" target="_blank">Opus 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">用于音频压缩，需要自行下载源码移植到 Keil MDK 中</td>
    </tr>     
    <tr>
        <td style="text-align:center; vertical-align:middle;">ST-Link V2 驱动</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.stmcu.com.cn/Designresource/list/STM32%20MCU/development_tools/development_tools" target="_blank">ST 中文官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">下载 STSW_LINK009</td>
    </tr> 
</table>

### 硬件设计资源

<table> 
    <tr>
        <th style="text-align:center; vertical-align:middle; width:22%;">类别</th>
        <th style="text-align:center; vertical-align:middle; width:25%;">名称</th>
        <th style="text-align:center; vertical-align:middle;">备注</th>
    </tr>
    <tr>
        <td rowspan="1" style="text-align:center; vertical-align:middle;">主控 MCU</td>
        <td style="text-align:center; vertical-align:middle;">N32H482REL7</td>
        <td style="text-align:center; vertical-align:middle;">32 位 ARM Cortex-M4F 内核，主频 240MHz，片内 Flash 容量 512KB</td>
    </tr>
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="1" style="text-align:center; vertical-align:middle;">无线通信模块</td>
        <td style="text-align:center; vertical-align:middle;">ESPC2-12-N4</td>
        <td style="text-align:center; vertical-align:middle;">通用型 WIFI 和低功耗蓝牙双模模块</td>
    </tr>  
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="1" style="text-align:center; vertical-align:middle;">显示模块</td>
        <td style="text-align:center; vertical-align:middle;">WS2812B</td>
        <td style="text-align:center; vertical-align:middle;">RGB LED，具备可编程控制特性，支持丰富的色彩变化与动态显示效果</td>
    </tr>    
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="3" style="text-align:center; vertical-align:middle;">音频模块</td>
        <td style="text-align:center; vertical-align:middle;">INMP441</td>
        <td style="text-align:center; vertical-align:middle;">音频输入麦克风，基于 MEMS 电容式传感技术，音频信号以 PCM 形式输出</td>
    </tr>    
    <tr>
        <td style="text-align:center; vertical-align:middle;">MAX98357AETE+T</td>
        <td style="text-align:center; vertical-align:middle;">音频输出功放，PCM 输入 D 类功率放大器，可提供 AB 类音频性能，同时具有 D 类的效率</td>
    </tr> 
    <tr>
        <td style="text-align:center; vertical-align:middle;">喇叭</td>
        <td style="text-align:center; vertical-align:middle;">扬声器，连接音频输出功放</td>
    </tr> 
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="1" style="text-align:center; vertical-align:middle;">外部时钟模块</td>
        <td style="text-align:center; vertical-align:middle;">Crystal</td>
        <td style="text-align:center; vertical-align:middle;">8MHz 外部晶振</td>
    </tr>
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="1" style="text-align:center; vertical-align:middle;">烧录模块</td>
        <td style="text-align:center; vertical-align:middle;">SWD</td>
        <td style="text-align:center; vertical-align:middle;">主要依靠 SWDIO 和 SWCLK 两根核心引脚进行数据传输。并增设有可选引脚 NRST 用于硬件复位，可使芯片恢复到初始状态，方便重新进行烧录或调试操作</td>
    </tr>
    <!-- ____________________________________________________________ -->
    <tr>
        <td rowspan="3" style="text-align:center; vertical-align:middle;">电源模块</td>
        <td style="text-align:center; vertical-align:middle;">TypeC、锂电池</td>
        <td style="text-align:center; vertical-align:middle;">通过电源管理模块、USB-TypeC 供电接口电路、锂电池充电电路、DC-DC 转换电路为整个硬件系统提供稳定、可靠的供电支持</td>
    </tr> 
</table>

### 服务端开发资源

<table> 
    <tr>
        <th style="text-align:center; vertical-align:middle; width:12%;">名称</th>
        <th style="text-align:center; vertical-align:middle; width:22%;">获取方式</th>
        <th style="text-align:center; vertical-align:middle;">备注</th>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">PyCharm</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.jetbrains.com/pycharm/" target="_blank">PyCharm 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">社区版即可，或其他Python 编辑器</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">Dcker Desktop</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.docker.com/" target="_blank">Docker 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">选择版本 for Windows-AMD64</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">SakuraFrap 启动器</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://www.natfrp.com/tunnel/download" target="_blank">SakuraFrap官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">用于建立隧道，使设备在不同局域网也能连接上服务器</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">Ollama</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://ollama.com/" target="_blank">Ollama 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">配置 Dify 的模型来源</td>
    </tr>
</table>

### APP开发资源

<table> 
    <tr>
        <th style="text-align:center; vertical-align:middle; width:18%;">名称</th>
        <th style="text-align:center; vertical-align:middle; width:24%;">获取方式</th>
        <th style="text-align:center; vertical-align:middle;">备注</th>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">Android Studio</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://redirector.gvt1.com/edgedl/android/studio/install/2025.1.2.11/android-studio-2025.1.2.11-windows.exe" target="_blank">Android Studio 官网</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">Android Studio Koala Feature Drop | 2024.1.2 或更新版本</td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">db 模块</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/training/data-storage/room" target="_blank">官方指南</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/reference/androidx/room/package-summary" target="_blank">API 参考</a>
        </td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">WIFI 模块</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/training/basics/network-ops/managing" target="_blank">官方指南</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/reference/android/net/wifi/WifiManager" target="_blank">API 参考</a>
        </td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">蓝牙模块</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/guide/topics/connectivity/bluetooth/ble-overview" target="_blank">官方指南</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/reference/android/bluetooth/le/package-summary" target="_blank">API 参考</a>
        </td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">MessageAdapter</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/develop/ui/" target="_blank">官方指南</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://developer.android.com/reference/androidx/recyclerview/widget/RecyclerView.Adapter" target="_blank">API 参考</a>
        </td>
    </tr>
    <tr>
        <td style="text-align:center; vertical-align:middle;">APP</td>
        <td style="text-align:center; vertical-align:middle;">
            <a href="https://pan.baidu.com/s/1CQz5sDIkyOXNj6nOz6wzPg?pwd=1111" target="_blank">下载地址</a>
        </td>
        <td style="text-align:center; vertical-align:middle;">支持的最低版本: Android 8.0 (API 26)</td>
    </tr>
</table> 

## 快速使用（免部署）

　　在使用 AutoAITalk 进行语音交互前，需先将设备连接至 WIFI 网络，以确保其能够访问云端服务。目前，AutoAITalk 支持两种主流的无线配网方式，分别是 **SoftAP** 与 **BluFi**，用户可跟据自身需求灵活选择。

### SoftAP 配网

　　SoftAP 是一种通用性强、兼容性高的配网方式。设备上电后，自身会创建一个 WIFI 热点`AITalk_SoftAP`，用户通过手机或电脑连接该热点后，可以在浏览器中打开配网界面（网址 **192.168.4.1**）。

<center class ='img'><img src=".\Images\SoftAP1.png" height="400"> </center>

　　在配网界面中，输入目标 WIFI 的 ssid、password 之后，即可点击“开始配网”按钮进行配网。配网成功后，配网界面会弹出提示消息“**配网成功，5秒后自动关闭网页**”。

<center class ='img'><img src=".\Images\SoftAP2.png" height="400"> </center>

### BluFi 配网

　　用户打开手机蓝牙后，可通过 AutoAITalk APP 或者其他 BluFi 配网小程序进行配网，在此以 **AutoAITalk APP** 为例进行说明：

- 扫描并连接设备蓝牙`AutoAITalk_BluFi`；

<center class ='img'><img src=".\Images\BluFi1.png" height="400"> </center>

- APP 会自动将当前手机连接的 WIFI 作为目标 WIFI，用户输入密码之后，点击“发送”按钮进行配网；

<center class ='img'><img src=".\Images\BluFi2.png" height="400"> </center>

- 配网成功后，会弹出提示消息“**配网成功，正在跳转**”，并跳转到 APP 主界面。

<center class ='img'><img src=".\Images\BluFi3.png" height="600"> </center>

### 语音对话

　　设备成功接入网络，并与 AutoAITalk 云端 WebSocket 服务器完成连接握手后，状态提示灯会由白色变为蓝色，表示设备已成功上线，进入**录音监听**状态，随时接收用户的语音输入。

　　当检测到用户语音结束，即判定为“讲话完成”，状态指示灯会立即由蓝色切换为绿色，表示设备进入**回复播放**状态。此时，设备将通过扬声器播放由云端生成的语音回复，用户可清晰地听到自然流畅的应答内容。设备播放完成后，再次进入录音监听状态，等待下一次用户语音输入，实现完整的语音交互闭环。

### 个性化定制

　　在配套的 **AutoAITalk App** 中，我们面向儿童教育与家庭互动场景，预设了多种 AI 角色，如“爸爸”、“妈妈”、“姐姐”、“弟弟”，支持**角色扮演、讲故事、问答互动**等功能，为孩子提供温暖、有情感的对话陪伴体验。

<center class ='img'><img src=".\Images\APP1.png" height="600"> </center>

　　此外，用户还可根据个人喜好，在 App 中创建专属 AI 角色，包括**自定义角色名称、性格特征、设定角色原型**，等等。

<center class ='img'><img src=".\Images\APP2.png" height="600"> </center>

## 边缘部署（支持离线运行）

　　待补充。