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
 *  ** 本驱动大致测试过, 基本上是线程完全的(除了初始化方法);
 * 
 *  2021-09-02    wing    创建.
 */

#ifndef RMT_WS2812_H
#define RMT_WS2812_H

#include <stdint.h>
#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifndef WS2812_RMT_CLK_DIV
/// 对于WS2812, 固定使用2分频就OK
#define WS2812_RMT_CLK_DIV 2 
#endif


typedef struct 
{
    /// 除了34及以上的只读Pin, 其它都可以
    gpio_num_t      Output_IO_Num;
    /// 使用的通道号
    rmt_channel_t   RMT_Channel;
    /// 当前RMT Channel 中串联的LED 数量,
    /// 该值也用于分配缓存的长度.
    uint32_t        LEDs_Count_Max;
    /// 是否启用双缓冲
    /// 需要双倍内存, 若不是有数以百计的WS2812, 则不需要双缓冲.
    bool            Double_Buffer;
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
} WS2812_TIMNG_CONFUG_t;

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
 * 初始化WS2812 
 */
esp_err_t WS2812_Init(WS2812_TIMNG_CONFUG_t *config);

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
 *  @param[in] Hz - 每秒刷新率, 一般25Hz足够, 
 *                  建议使用能被1000000 整除的刷新率, 相对会准确一些.
 * 
 */
esp_err_t WS2812_Loop_Start(uint16_t Hz);

/** 停止自动刷新 */
esp_err_t WS2812_Loop_Stop(void);

#endif