/**
 *  WS28212 RMT 方式驱动
 * 
 *  重点是WS2812_Fill_Buffer() 和 WS2812_Refresh() 两个方法,
 *  
 *  若不是周期性的动态灯效, 一般直接用WS2812_Send_LEDs() 刷新WS2812,
 *  其内部分别调用WS2812_Fill_Buffer() 和 WS2812_Refresh();
 * 
 *  若需要动态灯效, 推荐打开自动刷新(WS2812_Loop_Start()), 
 *  然后在需要改变RGBs 时使用WS2812_Fill_Buffer() 填充数据即可, 
 *  自动刷新机制会在下一个刷新时机时发送到WS2812;
 * 
 *  使用自动刷新时, 直接调用WS2812_Send_LEDs() 可不用等待下一个刷新时机
 *  立即发送新的数据.
 *  
 *  本驱动大致测试过, 基本上是线程完全的(除了初始化方法);
 * 
 *  2021-09-02    wing    创建.
 */

#ifndef RMT_WS2812_H
#define RMT_WS2812_H

#include <stdint.h>
#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifndef WS2812_RMT_CHANNEL
// 默认使用最后一个, 将前面的7个mem_block 留给其它应用
#define WS2812_RMT_CHANNEL RMT_CHANNEL_7
#endif
#ifndef WS2812_RMT_CLK_DIV
/// 对于WS2812, 固定使用2分频就OK
#define WS2812_RMT_CLK_DIV 2 
#endif

/**
 *  Gamma 校正表的数据类型
 *      
 *      可能过以下代码自行生成gamma 表
 * @code 
 *      #include <math.h>
 *      typedef unsigned char UNIT8; //用 8 位无符号数表示 0～255 之间的整数
 *      UNIT8 g_GammaLUT[256]; //这个数组保存BuildTable() 后256个gamma校正值
 *      //①归一化、预补偿、反归一化;
 *      //②将结果存入 gamma 查找表。
 *     
 *      // @param fPrecompensation 从公式得fPrecompensation=1/gamma
 *      void BuildTable(float fPrecompensation ) {
 *          int i;
 *          float f;
 *          for( i=0;i<256;i++) {
 *              f=(i+0.5F)/256; // 归一化
 *              f=(float)pow(f,fPrecompensation);
 *              g_GammaLUT[i]=(UNIT8)(f*256-0.5F); // 反归一化
 *          }
 *      }
 * @endcode
 * @return {*}
 */
typedef uint8_t WS2812_GAMMA_LUT[256];

typedef struct 
{
    /// 除了34及以上的只读Pin, 其它都可以
    gpio_num_t      output_io_num;
    /// 使用的通道号
    rmt_channel_t   rmt_channel;
    /// 当前RMT Channel 中串联的LED 数量,
    /// 该值也用于分配缓存的长度.
    uint32_t        leds_count_max;
    /// 是否启用双缓冲
    /// 需要双倍内存, 若不是有数以百计的WS2812, 则不需要双缓冲.
    bool            double_buffer;
    /// unit as ns
    uint16_t        T0H;    
    /// unit as ns
    uint16_t        T0L;
    /// unit as ns
    uint16_t        T1H;
    /// unit as ns
    uint16_t        T1L;
    /// unit as ns; 
    /// 最大值是819174 x WS2812_RMT_CLK_DIV, 
    /// 超过此值时高几bit 可能被截断.
    uint32_t        RES;
} WS2812_CONFIG_t;

/**
 *  载入默认RMT配置
 * 
 *  5个时序经测试正常运行   wing    2022.08.18
 * 
 * @param[in]   gpio_num  GPIO_NUM_x
 * @param[in]   count     该gpio_num 下串联了多少个ws2812。
 */
#define WS2812_Default_Config(io_num, count) {          \
    .output_io_num      = io_num,                       \
    .rmt_channel        = WS2812_RMT_CHANNEL,           \
    .leds_count_max     = count,                        \
    .double_buffer      = false,                        \
    .T0H                = 300,                          \
    .T0L                = 800,                          \
    .T1H                = 800,                          \
    .T1L                = 800,                          \
    .RES                = 200000,                       \
}

/**
 *  方便分量设置RGB值的结构体
 */
typedef union
{
    /// 只使用低24位, 以R8G8B8格式表示.
    uint32_t rgb;

    struct __attribute__((packed))
    {
        uint8_t b, g, r;
    };
} WS2812_COLOR_t;

/**
 * 取ws2812 的默认配置
 * @return 返回配置结构体的指针。
 */
// const WS2812_CONFIG_t * WS2812_DEFAULT_CONFIG(void);

/**
 * 初始化WS2812 
 */
esp_err_t WS2812_Init(WS2812_CONFIG_t *config);

/**
 *  填充数据但不会触发刷新
 * 
 *  @param[in]   colors -   RGB数据, 只使用低24位, 以R8G8B8格式表示.
 * 
 *  @param[in]   count  -   传送多少个RGB数据, 
 *                          由于WS2812 的级联方式, 以及大多数WS2812 传输数据比较慢,
 *                          当有很多WS2812 串联的时候, 若只改变靠前的少数WS2812, 
 *                          可以只传送前方的少数数据, 进而节省时间及功耗.
 */
esp_err_t WS2812_Fill_Buffer(WS2812_COLOR_t *colors, uint32_t count);

/**
 *  传送RGB数据
 * 
 *  不论自动刷新是否开启, 仍然可手动立即刷新.
 *  
 *  @param[in]   wait         - 是否阻塞至传送完成才返回.
 * 
 *  @param[in]   xTicksToWait - 当缓冲区正在填充时调用本方法, 本方法需要等待至填充完毕才开始发送,
 *                              若传入portMAX_DELAY 则一直等待到填充完毕并发送;
 *                              若传入具体时间(Tick), 则超时后立即返回而不发送数据.
 */
esp_err_t WS2812_Refresh(bool wait, TickType_t xTicksToWait);

/**
 *  填充数据并立即发送至WS2812
 * 
 *  WS2812_Fill_Buffer() + WS2812_Refresh() 的简易版;
 *  该方法在发送后不阻塞直接返回.
 */
esp_err_t WS2812_Send_LEDs(WS2812_COLOR_t *colors, uint32_t count);

typedef enum { 
    WS2812_Event_Before_Refresh,    // 将要刷新
    WS2812_Event_After_Refresh,     // 刚刷新过
} WS2812_Event_t;

/**
 *  事件回调
 * @param val
 * @return {*}
 */
typedef void (*WS2812_Event_Callback) (WS2812_Event_t val);

/**
 *  打开周期自动刷新
 * 
 *  按Hz 参数自动周期性地给WS2812 发送新的数据,
 *  适合制作呼吸灯之类的光效;
 *  而在每次到达刷新时间时, 当缓冲区没有新的填充, 
 *  则跳过本次刷新以节省功耗.
 * 
 *  若要在WS2812_Loop_Start() 后变更刷新率, 
 *  只需直接再WS2812_Loop_Start() 一次就行,
 *  不用WS2812_Loop_Stop().
 * 
 *  注意：
 *      启动自动刷新后的第一次刷新是在(1000000 / Hz)us后，
 *      视觉上一般感觉不到；
 *      但若要立即刷新，可手动WS2812_Send_LEDs()一次。
 * 
 *  @param[in] Hz - 每秒刷新率, 一般25Hz足够, 
 *                  建议使用能被1000000 整除的刷新率, 相对会准确一些.
 * 
 */
esp_err_t WS2812_Loop_Start(uint16_t Hz, WS2812_Event_Callback cb);

/** 
 *  停止自动刷新 
 */
esp_err_t WS2812_Loop_Stop(void);

/**
 *  启用Gamma校正
 *  
 * @param lut 校正表：传入NULL 则使用驱动自带的校正表。
 */
void WS2812_Gamma_On(WS2812_GAMMA_LUT lut);

/**
 *  禁用Gamma校正
 */
void WS2812_Gamma_Off(void);

#endif
