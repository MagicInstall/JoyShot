/**
 *  WS28212 RMT 方式驱动
 * 
 *  2021-09-02  wing    创建.
 *  2021-09-04  wing    加入双缓冲.
 */

#include <string.h>
#include "rmt_ws2812.h"
#include "esp_log.h"

#include "freertos/semphr.h"

#define BITS_PER_LED 24
// #define LED_BUFFER_ITEMS ((WS2812_COUNT_MAX * BITS_PER_LED))


static const char *TAG = "WS2812";

//#define RMT_TICK_1_NS    (1/(80000000/RMT_CLK_DIV))   /*!< RMT counter value for 10 us.(Source clock is APB clock) */
// APB_CLK 80MHz, clock_cycle = 1/80/RMT_CLK_DIV = 25ns
//
//#define T0H 	350ns/RMT_TICK_1_NS = 14
//#define T1H 	700ns/RMT_TICK_1_NS = 28
//#define T0L  	800ns/RMT_TICK_1_NS = 32
//#define T1L  	600ns/RMT_TICK_1_NS = 24
/**
 *  RTM 时序配置
 * 
 *  T0H, T0L, T1H, T1L, RES 的实际时间公式:
 *                    1
 *  t(ns) = ------------------------- X T0H
 *          APB_CLK / clock_divider 
 * 
 *  T0H, T0L, T1H, T1L, RES 的取值公式:
 *      T0H = t(ns) / ((1 / (APB_CLK / clock_divider)) * 1000000000(ns))
 * 
 *  APB_CLK 并不是一定是80mHz, 而(APB_CLK / clock_divider) 的值可以在rmt_config()后,
 *  通过rmt_get_counter_clock() 得到,
 *  最终:   
 *      T0H = t(ns) / (1000000000(ns) / rmt_get_counter_clock)
 *      T0H = t(ns) / _rmt_1_tick_ns
 */
static uint32_t _rmt_1_tick_ns = 25;

/**
 *  这里的T0H, T0L, T1H, T1L, RES 的值与WS2812_Init() 时的不同,
 *  WS2812_Init()使用纳秒, 这里是保存换算后的tick 数.
 */
static WS2812_CONFIG_t _rmt_config;


// // These values are determined by measuring pulse timing with logic analyzer and adjusting to match datasheet.
// #define T0H 14 // 0 bit high
// #define T1H 28 // 1 bit high time
// #define T0L 32 // low time for either bit
// #define T1L 24 // low time for either bit

typedef enum
{
    Send_Buffer_IDLE,
    Send_Buffer_Filled,
    Send_Buffer_Refreshed,
    /// 正在Fill 或者Refresh.
    Send_Buffer_Busy,
    Send_Buffer_Uninitialized,
} Send_Buffer_Status_t;

typedef struct
{
    Send_Buffer_Status_t    starus;
    SemaphoreHandle_t       semaphore;
    // WS2812_Fill_Buffer() 时填充的item 总数.
    uint32_t                count;
    rmt_item32_t *          items;
} Send_Buffer_t;


static Send_Buffer_t _send_buf_A = {Send_Buffer_Uninitialized, NULL, 0, NULL};
static Send_Buffer_t _send_buf_B = {Send_Buffer_Uninitialized, NULL, 0, NULL};


static const WS2812_CONFIG_t _WS2812_DEFAULT_CONFIG = {
    .Output_IO_Num  = WS2812_DIN_GPIO_NUM, 
    .RMT_Channel    = WS2812_RMT_CHANNEL,  
    .Double_Buffer  = false,
    .LEDs_Count_Max = WS2812_COUNT,
    .T0H            = 300, //350,
    .T0L            = 800, //1300,
    .T1H            = 800, //1300,
    .T1L            = 800, //1300,
    .RES            = 200000,
};
const WS2812_CONFIG_t * WS2812_DEFAULT_CONFIG(void) {
    return &_WS2812_DEFAULT_CONFIG;
}


// WS2812_COLOR_t set_LedRGB(uint8_t r, uint8_t g, uint8_t b)
// {
//     WS2812_COLOR_t v;

//     v.r = r;
//     v.g = g;
//     v.b = b;
//     return v;
// }

#define _CONVERT_TO_TICKS(ns)   (ns / _rmt_1_tick_ns)

esp_err_t WS2812_Init(WS2812_CONFIG_t *config)
{
    ESP_ERROR_CHECK(config == NULL);
    if (config == NULL) return ESP_ERR_INVALID_ARG;

    // TODO: 检查_send_buf_x.starus == Send_Buffer_Uninitialized

    memcpy(&_rmt_config, config, sizeof(WS2812_CONFIG_t));

    // 初始化缓冲区
    if (_send_buf_A.semaphore) {vSemaphoreDelete(_send_buf_A.semaphore); _send_buf_A.semaphore = NULL; } 
    if (_send_buf_B.semaphore) {vSemaphoreDelete(_send_buf_B.semaphore); _send_buf_B.semaphore = NULL; } 
    _send_buf_A.semaphore = xSemaphoreCreateMutex();
    if (_rmt_config.Double_Buffer)
        _send_buf_B.semaphore = xSemaphoreCreateMutex();
    
    _send_buf_A.starus = Send_Buffer_IDLE;
    _send_buf_B.starus = Send_Buffer_IDLE;

    if (_send_buf_A.items) {free(_send_buf_A.items); _send_buf_A.items = NULL;}
    if (_send_buf_B.items) {free(_send_buf_B.items); _send_buf_B.items = NULL;}
    uint32_t buf_len = (
        sizeof(rmt_item32_t) * (_rmt_config.LEDs_Count_Max * BITS_PER_LED + 
        1/*加上一个RES信号的item*/));

    _send_buf_A.items = (rmt_item32_t *)malloc(buf_len);
    if (_send_buf_A.items == NULL) return ESP_ERR_NO_MEM;
    if (_rmt_config.Double_Buffer) 
    {
        _send_buf_B.items = (rmt_item32_t *)malloc(buf_len);
        if (_send_buf_B.items == NULL) return ESP_ERR_NO_MEM;
    }
    
    _send_buf_A.starus = Send_Buffer_IDLE;
    _send_buf_B.starus = Send_Buffer_IDLE;

    // 初始化RMT 模块
    rmt_config_t rmt;
    rmt.rmt_mode = RMT_MODE_TX;
    rmt.channel = _rmt_config.RMT_Channel;
    rmt.gpio_num = _rmt_config.Output_IO_Num;
    rmt.clk_div = WS2812_RMT_CLK_DIV;  
    rmt.mem_block_num = 1; // 发送只需要1个mem_block, RMT 驱动会自动分块发送
    rmt.tx_config.loop_en = false;
    rmt.tx_config.carrier_en = false;
    rmt.tx_config.idle_output_en = true;
    rmt.tx_config.idle_level = RMT_IDLE_LEVEL_LOW; // 空闲时低电平

    esp_err_t rst = rmt_config(&rmt);
    if (rst != ESP_OK) return rst;

    // 算出RMT 一个时钟的时长
    rst = rmt_get_counter_clock(_rmt_config.RMT_Channel, &_rmt_1_tick_ns);
    if (rst != ESP_OK) return rst;
    _rmt_1_tick_ns = 1000000000 / _rmt_1_tick_ns;

    // 将us 转换成tick 数
    _rmt_config.T0H = _CONVERT_TO_TICKS(config->T0H);
    _rmt_config.T0L = _CONVERT_TO_TICKS(config->T0L);
    _rmt_config.T1H = _CONVERT_TO_TICKS(config->T1H);
    _rmt_config.T1L = _CONVERT_TO_TICKS(config->T1L);
    _rmt_config.RES = _CONVERT_TO_TICKS(config->RES) >> 1/*将一个值分为一个方波周期两个低电平, 增加可设置值的上限*/;

    return rmt_driver_install(_rmt_config.RMT_Channel, 0, 0);
}

/**
 *  指向当前可填充数据的缓冲区的指针
 * 
 *  该指针的status 成员只有WS2812_Fill_Buffer() 可以设置为Send_Buffer_Filled;
 * 
 *  只有WS2812_Refresh() 可以切换该指针(不启用双缓冲则不会切换), 
 *  无论填充多少次, 只要一日未刷新(发送), 
 *  WS2812_Fill_Buffer() 都只是更新同一个缓冲区;
 */
static Send_Buffer_t *_fill_buf = &_send_buf_A;

/**
 *  填充缓冲区
 */
esp_err_t WS2812_Fill_Buffer(WS2812_COLOR_t *colors, uint32_t count)
{
    if (count > _rmt_config.LEDs_Count_Max)
    {
        count = _rmt_config.LEDs_Count_Max;
        ESP_LOGI(TAG, "WS2812_Fill_Buffer() - Exceeded the count max , set to preset");
    }

    xSemaphoreTake(_fill_buf->semaphore, portMAX_DELAY);
    // if (_fill_buf->status)


    uint32_t item_idx = 0;
    uint32_t grb, mask, bit_is_set;

    // 按LED 设置
    for (uint32_t idx = 0; idx < count; idx++)
    {
        // 转换成WS2812 的GRB格式 
        grb = colors[idx].b | (colors[idx].r << 8) | (colors[idx].g << 16);
        mask = 1 << (BITS_PER_LED - 1);
        for (uint32_t bit = 0; bit < BITS_PER_LED; bit++)
        {
            bit_is_set = grb & mask;
            // _fill_buf[idx * BITS_PER_LED + bit] = bit_is_set ? 
            _fill_buf->items[item_idx ++] = bit_is_set ? 
                    (rmt_item32_t){{{_rmt_config.T1H, 1, _rmt_config.T1L, 0}}} : 
                    (rmt_item32_t){{{_rmt_config.T0H, 1, _rmt_config.T0L, 0}}};
            mask >>= 1;
        }
    }

    // 加入RES
    _fill_buf->items[item_idx ++] = (rmt_item32_t){{{_rmt_config.RES, 0, _rmt_config.RES, 0}}}; // 两个都是低电平

    _fill_buf->count = item_idx;
    _fill_buf->starus = Send_Buffer_Filled;

    xSemaphoreGive(_fill_buf->semaphore);
    return ESP_OK;
}

/**
 *  通过RMT 发送数据到WS2812
 */
esp_err_t WS2812_Refresh(bool wait, TickType_t xTicksToWait)
{   
    ESP_ERROR_CHECK(rmt_wait_tx_done(_rmt_config.RMT_Channel, portMAX_DELAY));

    // 选择缓冲区
    Send_Buffer_t *current_buf = _fill_buf;

    esp_err_t rst = xSemaphoreTake(current_buf->semaphore, xTicksToWait);
    if (rst != pdTRUE) 
    {
        ESP_LOGI("WS2812_Refresh", "Semaphore timeout.");
        return rst;    // 超时则立刻返回
    }

    // 若缓冲区未填充
    if (current_buf->starus != Send_Buffer_Filled)
    {
        // 还锁并返回
        xSemaphoreGive(current_buf->semaphore);
        return ESP_OK;
    }

    // 拿到锁之后先切换缓冲区, 让WS2812_Fill_Buffer() 填充下一个缓冲区.
    if (_rmt_config.Double_Buffer)
        _fill_buf = (current_buf == &_send_buf_A) ? 
                        &_send_buf_B : 
                        &_send_buf_A;


    rst = rmt_write_items(_rmt_config.RMT_Channel, current_buf->items, current_buf->count/*cnt * BITS_PER_LED + 1加上RES信号的item*/, wait);

    current_buf->starus = Send_Buffer_IDLE;

    xSemaphoreGive(current_buf->semaphore);
    return rst;
}

esp_err_t WS2812_Send_LEDs(WS2812_COLOR_t *colors, uint32_t count)
{
    esp_err_t rst = WS2812_Fill_Buffer(colors, count);
    if (rst != ESP_OK) return rst;

    return WS2812_Refresh(false, portMAX_DELAY);
} 

static void _refresh_task(void *pvParameters)
{
    ESP_ERROR_CHECK(WS2812_Refresh(false, (TickType_t)pvParameters));
}

static esp_timer_handle_t _timer_handle = NULL;

esp_err_t WS2812_Loop_Start(uint16_t Hz)
{
    ESP_ERROR_CHECK(Hz < 1);
    ESP_ERROR_CHECK(_send_buf_A.starus == Send_Buffer_Uninitialized);

    uint64_t us = 1000000 / Hz;
    if (_timer_handle == NULL)
    {
        esp_timer_create_args_t timer_arg = {
            .callback = _refresh_task,
            /* 留给WS2812_Refresh 只有一帧的等待时间, 该时间只是大概. */
            .arg = (void *)(us / 1000 / portTICK_RATE_MS), 
            .name = "WS2812_Timer" 
        };        
        ESP_ERROR_CHECK(esp_timer_create(&timer_arg, &_timer_handle));
    }

    return esp_timer_start_periodic(_timer_handle, us);
}
esp_err_t WS2812_Loop_Stop(void)
{
    return esp_timer_stop(_timer_handle);
}

