/*
 *  按键
 * 
 *  非线程安全
 * 
 *  2021-08-07  wing    创建.
 */

#include <string.h>
#include "buttons.h"
#include "esp_log.h"
#include "freertos/queue.h"

typedef struct
{
    // gpio_num_t              gpio_num;       // 引脚号
    Button_Config_t         button_config;  // 引脚配置
    esp_timer_handle_t      timer_handle;   // 定时器
    QueueHandle_t 			queue;          // 事件队列
    bool                    holding;        // 按住
    // Button_Callback   callback;       // 事件回调
} _Button_Class_t;

_Button_Class_t *_buttons[BUTTON_COUNT_MAX];
uint8_t          _button_cnt = 0;

static void _btn_task(void *param) {
    while (1) {

    }
}

/// 所有定时器统一进这个中断
static void _btn_timer_cb(void *arg) 
{
    _Button_Class_t *btn = (_Button_Class_t *)arg;

    // 已触发按下
    if (btn->holding) 
    {
        if (gpio_get_level(btn->button_config.gpio_num))
        {
            // 触发松开
            esp_timer_stop(btn->timer_handle);
            // TODO: 改为消息通知另一个线程
            // xTaskCreatePinnedToCore
            // btn->button_config.callback(Button_Event_Off);
            btn->holding = false; // 在运行完callback 后才清除, 保证callback 能完成长时间运行的脚本!
        }
    }
    // 未触发按下
    else
    {
        // 消抖
        if (gpio_get_level(btn->button_config.gpio_num))
        {
            esp_timer_stop(btn->timer_handle);
            return;
        }

        // 触发按下
        btn->holding = true;
        // TODO: 改为消息通知另一个线程
        // btn->button_config.callback(Button_Event_On);
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
    esp_timer_start_periodic(timer_handle, BUTTON_DEBOUNCE);
}

// TODO: 目前未实现重复启动按键检测
Button_Handle_t Button_Enable(Button_Config_t *config)
{
    if (_button_cnt >= BUTTON_COUNT_MAX)
        return NULL; // (Button_Handle_t);

    // TODO: 加入重复检测...

    if (gpio_config(&(config->gpio_config)) != ESP_OK)
        return NULL;

    _Button_Class_t *new_btn = (_Button_Class_t *)malloc(sizeof(_Button_Class_t));
    memset(new_btn, 0, sizeof(_Button_Class_t));
    memcpy(&new_btn->button_config, config, sizeof(Button_Config_t));
    _buttons[_button_cnt] = new_btn;
    _button_cnt ++;

    if (new_btn->timer_handle == NULL) 
    {
        esp_timer_create_args_t timer_arg = {
            .callback = &_btn_timer_cb,
            .arg = (void *) new_btn,
            .name = "BtnTimer",
        };
        if (esp_timer_create(&timer_arg, &(new_btn->timer_handle)) != ESP_OK)
        {
            free(new_btn);
            return NULL;
        }
    }    

    if (Button_Set_Callback(new_btn, config->callback) != ESP_OK)
    {
        Button_Disable(new_btn);
        return NULL;
    }
    

    // if (gpio_set_intr_type(config->gpio_num, config->gpio_config.intr_type) != ESP_OK)
    //     return NULL;
    // if (config->callback != NULL)
    // {
    //     ESP_ERROR_CHECK(gpio_install_isr_service(0/*目前使用默认标志*/));

    //     if (gpio_isr_handler_add(config->gpio_num, _btn_isr_handler, (void *)new_btn) != ESP_OK)
    //     {
    //         free(new_btn);
    //         return NULL;
    //     }
        
        
    // }


    return new_btn;
}

/// 找出对应的按键对象的索引
static int _search_button_index(Button_Handle_t handle)
{
    if (handle == NULL) return -1;

    int btn_idx = -1;
    for (int i = 0; i < _button_cnt; i++)
    {
        if (_buttons[i] == handle)
        {
            btn_idx = i;
            break;
        }
    }    

    return btn_idx;
}

/// 只关闭中断相关的功能;
/// 不动GPIO 配置, 以及保留timer(但即时停止).
static void _button_callback_disable(Button_Handle_t handle) 
{
    _Button_Class_t *btn = (_Button_Class_t *)handle;

    if (btn->button_config.callback != NULL)
    {
        gpio_isr_handler_remove(btn->button_config.gpio_num);
        gpio_intr_disable(btn->button_config.gpio_num);
        esp_timer_stop(btn->timer_handle);
        btn->holding = false;
    }

    return;
}

esp_err_t Button_Set_Callback(Button_Handle_t handle, Button_Callback cb)
{
    // 检查handle 
    int btn_idx = _search_button_index(handle);
    if (btn_idx < 0) return ESP_ERR_INVALID_ARG;

    _Button_Class_t *btn = (_Button_Class_t *)handle;
    // // callback 无变化直接返回
    // if (btn->button_config.callback == cb) return ESP_OK;

    // 先禁用按键中断
    if (btn->button_config.callback != NULL)
        _button_callback_disable(btn);

    // 启用
    if (cb != NULL){
        btn->button_config.callback = cb;
        
        gpio_install_isr_service(ESP_INTR_FLAG_LEVEL6/*较高优先级*/);

        esp_err_t rst;
        rst = gpio_set_intr_type(btn->button_config.gpio_num, 
                                btn->button_config.gpio_config.intr_type);
        if (rst != ESP_OK) return rst;

        rst = gpio_isr_handler_add(btn->button_config.gpio_num, _btn_isr_handler, (void *)btn);
        if (rst != ESP_OK) return rst;
    }

    return ESP_OK;
}
    
void Button_Disable(Button_Handle_t handle) 
{
    // 递归撤销全部按键
    if (handle == (Button_Handle_t *)Button_Disable_ALL) 
    {
        for (int i = _button_cnt; i > 0; i--)
        {
            Button_Disable(_buttons[i - 1]);
        }
        return;
    }

    int btn_idx = _search_button_index(handle);
    if (btn_idx < 0) 
        return; //  ESP_ERR_INVALID_ARG

    _button_callback_disable(_buttons[btn_idx]);
    ESP_ERROR_CHECK(esp_timer_delete(_buttons[btn_idx]->timer_handle));
    ESP_ERROR_CHECK(gpio_reset_pin(_buttons[btn_idx]->button_config.gpio_num));

    // 从数组中间删除对象的引用
    for (int i = btn_idx; i < _button_cnt -1; i++)
    {
        if (((i + 1) >= _button_cnt) || (_buttons[i + 1] == NULL)) {
            _buttons[i] = NULL;
            _button_cnt --;
            break;
        }

        _buttons[i] = _buttons[i + 1];
    }
    free(handle);
}

Button_Level_t Button_Click(gpio_num_t gpio_num) {
    static gpio_config_t config = {
        // .pin_bit_mask   = (1ULL << gpio_num),
        .intr_type      = GPIO_INTR_NEGEDGE,
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_ENABLE,  
    };
    config.pin_bit_mask = (1ULL << gpio_num);
    ESP_ERROR_CHECK(gpio_config(&config));
    
    return gpio_get_level(gpio_num)==0 ? Button_On : Button_Off;
}
