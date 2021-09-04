/**
 *    WS28212 RMT 方式驱动
 * 
 *    2021-09-02    wing    创建.
 */

#ifndef RMT_WS2812_H
#define RMT_WS2812_H

#include <stdint.h>
#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_err.h"

#ifndef WS2812_RMT_CLK_DIV
/// 目前认为固定使用2分频就OK
#define WS2812_RMT_CLK_DIV 2 
#endif

/**
 *  WS2812 时序配置
 * 
 *  T0H, T0L, T1H, T1L, RES 的实际时间公式:
 *                    1
 *  t(ns) = ------------------------- X T0H
 *          APB_CLK / clock_divider 
 * 
 *  T0H, T0L, T1H, T1L, RES 的取值公式:
 *      T0H = t(ns) / ((1 / (APB_CLK / clock_divider)) * 1000000000(ns))
 *  最终:   
 *      T0H = t(ns) / (1000000000(ns) / APB_CLK) / clock_divider
 */
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
    /// 需要双倍内存.
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

typedef union
{
    uint32_t rgb;

    struct __attribute__((packed))
    {
        uint8_t b, g, r;
    };
} WS2812_COLOR_t;

// WS2812_COLOR_t set_LedRGB(uint8_t r, uint8_t g, uint8_t b);

esp_err_t WS2812_Init(WS2812_TIMNG_CONFUG_t *config);

/**
 *  
 */
esp_err_t WS2812_Fill_Buffer(WS2812_COLOR_t *colors, uint32_t count);

/**
 *  传送RGB数据
 * 
 *  @param[in]   colors -       RGB数据.
 * 
 *  @param[in]   count -        传送多少个RGB数据, 
 *                              由于WS2812 的级联方式, 以及大多数WS2812 传输数据比较慢,
 *                              当有很多WS2812 串联的时候, 若只改变靠前的少数WS2812, 
 *                              可以只传送前方的少数数据, 进而节省时间及功耗.
 * 
 *  @param[in]   wait  -        是否阻塞至传送完成才返回.
 * 
 *  @param[out]  stopwatch  -   若参数wait 为true, 则在返回后可得到本次发送的耗时, 单位是;
 *                              若调用本方法时, 上一次的传送仍未完成, 
 *                              则等到其完成后才开始计算;
 *                              返回值是两种情况:
 *                              若返回正数, 本次发送的耗时(刷新时间), 可以用来计算帧数(仅作参考);
 *                              若返回负数, 表示RGB 数据转换到RMT 数据的耗时大于发送的耗时.
 */
esp_err_t WS2812_Refresh(/*WS2812_COLOR_t *colors, uint32_t count, */bool wait, uint32_t *stopwatch);

/**
 *  WS2812_Fill_Buffer() + WS2812_Refresh() 的简易版
 *  该方法在发送后不阻塞直接返回.
 */
esp_err_t WS2812_Send_LEDs(WS2812_COLOR_t *colors, uint32_t count);


esp_err_t WS2812_Loop_Start(void);
esp_err_t WS2812_Loop_Stop(void);


// void WS_Test(void);

#endif