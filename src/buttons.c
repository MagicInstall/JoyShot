/*
 *  按键
 * 
 *  非线程安全
 * 
 *  2021-08-07  wing    创建.
 */

#include <string.h>
#include "buttons.h"

typedef struct
{
    gpio_num_t              gpio_num;       // 引脚号
    bool                    holding;        // 按住
    esp_timer_handle_t      timer_handle;   // 定时器
    Button_Event_Callback   callback;       // 事件回调
} _Button_Class_t;

_Button_Class_t *_buttons[BUTTON_COUNT_MAX];
uint8_t          _button_cnt = 0;

/// 所有定时器统一进这个中断
static void _btn_timer_cb(void *arg) 
{
    _Button_Class_t *btn = (_Button_Class_t *)arg;

    // 已触发按下
    if (btn->holding) 
    {
        if (gpio_get_level(btn->gpio_num))
        {
            // 触发松开
            esp_timer_stop(btn->timer_handle);
            btn->callback(Button_Event_Off);
            btn->holding = false; // 在运行完callback 后才清除, 保证callback 能完成长时间运行的脚本!
        }
    }
    // 未触发按下
    else
    {
        // 消抖
        if (gpio_get_level(btn->gpio_num))
        {
            esp_timer_stop(btn->timer_handle);
            return;
        }

        // 触发按下
        btn->holding = true;
        btn->callback(Button_Event_On);
    }
}

/// 所有按键统一进这个中断
static void IRAM_ATTR _btn_isr_handler(void* arg) 
{
    if (((_Button_Class_t *)arg)->holding) return;
    
    esp_timer_handle_t timer_handle = ((_Button_Class_t *)arg)->timer_handle;
    // uint32_t          *check        = &(((btn_args *)arg)->btn_event_check);

    // 不论上次是否因为抖动而触发，定时器都将重新开始
    // esp_timer_stop(timer_handle);
    // *check = 0;
    // ((_Button_Class_t *)arg)->holding = false;
    esp_timer_start_periodic(timer_handle, BUTTON_DEFAULT_DEBOUNCE * 1000);
}

// TODO: 目前未实现重复启动按键检测
Button_Handle_t Button_Enable(Button_Config_t *config)
{
    if (_button_cnt >= BUTTON_COUNT_MAX)
        return NULL; // (Button_Handle_t);

    // TODO: 加入重复检测...

    if (gpio_config(&(config->gpio_config)) != ESP_OK)
        return NULL;
    if (gpio_set_intr_type(config->gpio_num, config->gpio_config.intr_type) != ESP_OK)
        return NULL;
    _Button_Class_t *new_btn = NULL;
    if (config->callback != NULL)
    {
        if (gpio_install_isr_service(0/*目前使用默认标志*/) != ESP_OK)
            return NULL;

        new_btn = (_Button_Class_t *)malloc(sizeof(_Button_Class_t));
        memset(new_btn, 0, sizeof(_Button_Class_t));
        if (gpio_isr_handler_add(config->gpio_num, _btn_isr_handler, (void *)new_btn) != ESP_OK)
        {
            free(new_btn);
            return NULL;
        }
    }
        
    esp_timer_create_args_t timer_arg = {
        .callback = &_btn_timer_cb,
		.arg = (void *) new_btn,
		.name = "BtnTimer" 
	};
    if (esp_timer_create(&timer_arg, &(new_btn->timer_handle)) != ESP_OK)
    {
        free(new_btn);
        return NULL;
    }

    new_btn->gpio_num = config->gpio_num;
    new_btn->callback = config->callback;
    _buttons[_button_cnt] = new_btn;
    _button_cnt ++;
    return new_btn;
}
