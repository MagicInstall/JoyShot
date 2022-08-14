//
//  BlueCubeMod Firmware
//
//
//  Created by Nathan Reeves 2019
//

#include "esp_log.h"
// #include "esp_hidd_api.h"
// #include "esp_bt_main.h"
// #include "esp_bt_device.h"
// #include "esp_bt.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
// #include "esp_gap_bt_api.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>
// #include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/rmt.h"
// #include "soc/rmt_reg.h"
#include "driver/periph_ctrl.h"

#include "ns_controller.h"
#include "buttons.h"
#include "lamp.h"
// #include "rmt_ws2812.h"
#include "uart_command.h"

static const char *TAG = "Main";

// #define LED_GPIO 25
// #define PIN_SEL (1ULL << LED_GPIO)

//for reading GameCube controller values
// #define RMT_TX_GPIO_NUM 23                               // GameCube TX GPIO ----
// #define RMT_RX_GPIO_NUM 18                               // GameCube RX GPIO ----
// #define RMT_TX_CHANNEL 2                                 /*!< RMT channel for transmitter */
// #define RMT_RX_CHANNEL 3                                 /*!< RMT channel for receiver */
// #define RMT_CLK_DIV 80                                   /*!< RMT counter clock divider */
// #define RMT_TICK_10_US (80000000 / RMT_CLK_DIV / 100000) /*!< RMT counter value for 10 us.(Source clock is APB clock) */
// #define rmt_item32_tIMEOUT_US 9500                       /*!< RMT receiver timeout value(us) */

// //Calibration
// static int lxcalib = 0;
// static int lycalib = 0;
// static int cxcalib = 0;
// static int cycalib = 0;
// static int lcalib = 0;
// static int rcalib = 0;
// //Buttons and sticks
// static uint8_t but1_send = 0;
// static uint8_t but2_send = 0;
// static uint8_t but3_send = 0;
// static uint8_t lx_send = 127;
// static uint8_t ly_send = 127;
// static uint8_t cx_send = 127;
// static uint8_t cy_send = 127;
// static uint8_t lt_send = 0;
// static uint8_t rt_send = 0;


// //RMT Transmitter Init - for reading GameCube controller
// rmt_item32_t items[25];
// rmt_config_t rmt_tx;

// TaskHandle_t BlinkHandle = NULL;


// static void rmt_tx_init()
// {

//     rmt_tx.channel = RMT_TX_CHANNEL;
//     rmt_tx.gpio_num = RMT_TX_GPIO_NUM;
//     rmt_tx.mem_block_num = 1;
//     rmt_tx.clk_div = RMT_CLK_DIV;
//     rmt_tx.tx_config.loop_en = false;
//     rmt_tx.tx_config.carrier_freq_hz = 24000000;
//     rmt_tx.tx_config.carrier_level = 1;
//     rmt_tx.tx_config.carrier_en = 0;
//     rmt_tx.tx_config.idle_level = 1;
//     rmt_tx.tx_config.idle_output_en = true;
//     rmt_tx.rmt_mode = 0;
//     rmt_config(&rmt_tx);
//     rmt_driver_install(rmt_tx.channel, 0, 0);

//     //Fill items[] with console->controller command: 0100 0000 0000 0011 0000 0010

//     items[0].duration0 = 3;
//     items[0].level0 = 0;
//     items[0].duration1 = 1;
//     items[0].level1 = 1;
//     items[1].duration0 = 1;
//     items[1].level0 = 0;
//     items[1].duration1 = 3;
//     items[1].level1 = 1;
//     int j;
//     for (j = 0; j < 12; j++)
//     {
//         items[j + 2].duration0 = 3;
//         items[j + 2].level0 = 0;
//         items[j + 2].duration1 = 1;
//         items[j + 2].level1 = 1;
//     }
//     items[14].duration0 = 1;
//     items[14].level0 = 0;
//     items[14].duration1 = 3;
//     items[14].level1 = 1;
//     items[15].duration0 = 1;
//     items[15].level0 = 0;
//     items[15].duration1 = 3;
//     items[15].level1 = 1;
//     for (j = 0; j < 8; j++)
//     {
//         items[j + 16].duration0 = 3;
//         items[j + 16].level0 = 0;
//         items[j + 16].duration1 = 1;
//         items[j + 16].level1 = 1;
//     }
//     items[24].duration0 = 1;
//     items[24].level0 = 0;
//     items[24].duration1 = 3;
//     items[24].level1 = 1;
// }

//RMT Receiver Init
// rmt_config_t rmt_rx;
// static void rmt_rx_init()
// {
//     rmt_rx.channel = RMT_RX_CHANNEL;
//    rmt_rx.gpio_num = RMT_RX_GPIO_NUM;
//     rmt_rx.clk_div = RMT_CLK_DIV;
//     rmt_rx.mem_block_num = 4;
//     rmt_rx.rmt_mode = RMT_MODE_RX;
//     rmt_rx.rx_config.idle_threshold = rmt_item32_tIMEOUT_US / 10 * (RMT_TICK_10_US);
//     rmt_config(&rmt_rx);
// }

// //Polls controller and formats response
// //GameCube Controller Protocol: http://www.int03.co.uk/crema/hardware/gamecube/gc-control.html
// static void get_buttons()
// {
//     ESP_LOGI("hi", "Hello world from core %d!\n", xPortGetCoreID());
//     //button init values
//     uint8_t but1 = 0;
//     uint8_t but2 = 0;
//     uint8_t but3 = 0;
//     uint8_t dpad = 0x08; //Released
//     uint8_t lx = 0;
//     uint8_t ly = 0;
//     uint8_t cx = 0;
//     uint8_t cy = 0;
//     uint8_t lt = 0;
//     uint8_t rt = 0;

//     //Sample and find calibration value for sticks
//     int calib_loop = 0;
//     int xsum = 0;
//     int ysum = 0;
//     int cxsum = 0;
//     int cysum = 0;
//     int lsum = 0;
//     int rsum = 0;
//     while (calib_loop < 5)
//     {
//         lx = 0;
//         ly = 0;
//         cx = 0;
//         cy = 0;
//         lt = 0;
//         rt = 0;
//         rmt_write_items(rmt_tx.channel, items, 25, 0);
//         rmt_rx_start(rmt_rx.channel, 1);

//         vTaskDelay(10);

//         rmt_item32_t *item = (rmt_item32_t *)(RMT_CHANNEL_MEM(rmt_rx.channel));
//         if (item[33].duration0 == 1 && item[27].duration0 == 1 && item[26].duration0 == 3 && item[25].duration0 == 3)
//         {

//             //LEFT STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 41].duration0 == 1))
//                 {
//                     lx += pow(2, 8 - x - 1);
//                 }
//             }

//             //LEFT STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 49].duration0 == 1))
//                 {
//                     ly += pow(2, 8 - x - 1);
//                 }
//             }
//             //C STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 57].duration0 == 1))
//                 {
//                     cx += pow(2, 8 - x - 1);
//                 }
//             }

//             //C STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 65].duration0 == 1))
//                 {
//                     cy += pow(2, 8 - x - 1);
//                 }
//             }

//             //R AN
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 73].duration0 == 1))
//                 {
//                     rt += pow(2, 8 - x - 1);
//                 }
//             }

//             //L AN
//             for (int x = 8; x > -1; x--)
//             {
//                 if ((item[x + 81].duration0 == 1))
//                 {
//                     lt += pow(2, 8 - x - 1);
//                 }
//             }

//             xsum += lx;
//             ysum += ly;
//             cxsum += cx;
//             cysum += cy;
//             lsum += lt;
//             rsum += rt;
//             calib_loop++;
//         }
//     }

//     //Set Stick Calibration
//     lxcalib = 127 - (xsum / 5);
//     lycalib = 127 - (ysum / 5);
//     cxcalib = 127 - (cxsum / 5);
//     cycalib = 127 - (cysum / 5);
//     lcalib = 127 - (lsum / 5);
//     rcalib = 127 - (rsum / 5);

//     while (1)
//     {
//         but1 = 0;
//         but2 = 0;
//         but3 = 0;
//         dpad = 0x08;
//         lx = 0;
//         ly = 0;
//         cx = 0;
//         cy = 0;
//         lt = 0;
//         rt = 0;

//         //Write command to controller
//         rmt_write_items(rmt_tx.channel, items, 25, 0);
//         rmt_rx_start(rmt_rx.channel, 1);

//         vTaskDelay(6); //6ms between sample

//         rmt_item32_t *item = (rmt_item32_t *)(RMT_CHANNEL_MEM(rmt_rx.channel));

//         //Check first 3 bits and high bit at index 33
//         if (item[33].duration0 == 1 && item[27].duration0 == 1 && item[26].duration0 == 3 && item[25].duration0 == 3)
//         {

//             //Button report: first item is item[25]
//             //0 0 1 S Y X B A
//             //1 L R Z U D R L
//             //Joystick X (8bit)
//             //Joystick Y (8bit)
//             //C-Stick X (8bit)
//             //C-Stick Y (8bit)
//             //L Trigger Analog (8/4bit)
//             //R Trigger Analog (8/4bit)

//             if (item[32].duration0 == 1)
//                 but1 += 0x08; // A
//             if (item[31].duration0 == 1)
//                 but1 += 0x04; // B
//             if (item[30].duration0 == 1)
//                 but1 += 0x02; // X
//             if (item[29].duration0 == 1)
//                 but1 += 0x01; // Y

//             if (item[28].duration0 == 1)
//                 but2 += 0x02; // START/PLUS

//             //DPAD
//             if (item[40].duration0 == 1)
//                 but3 += 0x08; // L
//             if (item[39].duration0 == 1)
//                 but3 += 0x04; // R
//             if (item[38].duration0 == 1)
//                 but3 += 0x01; // D
//             if (item[37].duration0 == 1)
//                 but3 += 0x02; // U

//             if (item[35].duration0 == 1)
//                 but1 += 0x80; // ZR
//             if (item[34].duration0 == 1)
//                 but3 += 0x80; // ZL
//             //Buttons
//             if (item[36].duration0 == 1)
//             {
//                 but1 += 0x40; // Z
//                               // if(but3 == 0x80) { but3 += 0x40;}
//                 if (but2 == 0x02)
//                 {
//                     but2 = 0x01;
//                 } // Minus =  Z + Start
//                 if (but3 == 0x02)
//                 {
//                     but2 = 0x10;
//                 } // Home =  Z + Up
//             }

//             //LEFT STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 41].duration0 == 1)
//                 {
//                     lx += pow(2, 8 - x - 1);
//                 }
//             }

//             //LEFT STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 49].duration0 == 1)
//                 {
//                     ly += pow(2, 8 - x - 1);
//                 }
//             }

//             //C STICK X
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 57].duration0 == 1)
//                 {
//                     cx += pow(2, 8 - x - 1);
//                 }
//             }

//             //C STICK Y
//             for (int x = 8; x > -1; x--)
//             {
//                 if (item[x + 65].duration0 == 1)
//                 {
//                     cy += pow(2, 8 - x - 1);
//                 }
//             }

//             /// Analog triggers here --  Ignore for Switch :/
//             /*
//             //R AN
//             for(int x = 8; x > -1; x--)
//             {
//                 if(item[x+73].duration0 == 1)
//                 {
//                     rt += pow(2, 8-x-1);
//                 }
//             }
            
//             //L AN
//             for(int x = 8; x > -1; x--)
//             {
//                 if(item[x+81].duration0 == 1)
//                 {
//                     lt += pow(2, 8-x-1);
//                 }
//             }*/
//             /////
//             but1_send = but1;
//             but2_send = but2;
//             but3_send = but3;
//             lx_send = lx + lxcalib;
//             ly_send = ly + lycalib;
//             cx_send = cx + cxcalib;
//             cy_send = cy + cycalib;
//             lt_send = 0; //lt;//left trigger analog
//             rt_send = 0; //rt;//right trigger analog
//         }
//         else
//         {
//             //log_info("GameCube controller read fail");
//         }
//     }
// }

///Switch Replies
// static uint8_t reply02[] = {0x01, 0x40, 0x00, 0x00, 0x00, 0xe6, 0x27, 0x78, 0xab, 0xd7, 0x76, 0x00, 0x82, 0x02, 0x03, 0x48, 0x03, 0x02, 0xD8, 0xA0, 0x1D, 0x40, 0x15, 0x66, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Reply for REQUEST_DEVICE_INFO

static NS_Controller_Type_t _joy_type;
static ns_button_status_t keys_data;


static Button_Handle_t _button_0_handle;

/// 按键 0 触发事件回调
static void _button_0_Callback (Button_Event_t val)
{
    ESP_LOGI("BTN 0", "%d", val);
    
    // TODO: 脚本运行过程构想:
    //  Step 0: 重置全部按键值;
    //  Step 1: 区分JC(L)/JC(R)/Pro
    //  Step 2: 运行按下脚本
    //  Step 2 - 1: 设置按键值;
    //  Step 2 - 2: 延时;
    //  Step 2 - 3: 重复 Step2 - 1 - Step2 - 2;
    //  Step 2 - 4: 结束;
    //  Step 3 : 运行松开脚本:
    //  Step 3 - 1 ~ 4 同按下;
    //  Step 4 结束
    if (val == Button_Event_On) 
    {   
        keys_data.Capture = 1;
        NS_Set_Buttons(&keys_data);
        vTaskDelay(1000 / portTICK_RATE_MS);
        keys_data.Capture = 0;
        NS_Set_Buttons(&keys_data);
    } 
}

/// 给NS_Open() 做状态标记
static bool conneted = false;

// 前置声明
void _ns_controller_cb(NS_CONTROLLER_EVT event, NS_CONTROLLER_EVT_ARG_t *arg);

void _wait_connet_task(void *param)
{
    if ((!conneted) && (param != NULL))
    {
        esp_bd_addr_t addr;
        memcpy(addr, param, sizeof(esp_bd_addr_t));
        ESP_ERROR_CHECK(NS_Open(addr));

        vTaskDelay(5000 / portTICK_RATE_MS);
        if (!conneted)
            // 触发事件至关机
            _ns_controller_cb(NS_CONTROLLER_DISCONNECTED_EVT, NULL);
    }

    ESP_LOGI("_wait_connet_task", "heap free: %d", xPortGetFreeHeapSize());
    vTaskDelete(NULL);
}

void _ns_controller_cb(NS_CONTROLLER_EVT event, NS_CONTROLLER_EVT_ARG_t *arg)
{
    switch (event)
    {
    case NS_CONTROLLER_SCANNED_DEVICE_EVT:
        // TODO: 将地址保存在flash...
        // TODO: 是否需要新开一个线程?
        break;
    case NS_CONTROLLER_CONNECTED_EVT:
        conneted = true;
        break;
    case NS_CONTROLLER_DISCONNECTED_EVT:
        conneted = false;
        // TODO: 关机...
        printf("To sleep...\n");
        break;
    
    default:
        break;
    }
}

static WS2812_COLOR_t color[] = {
    {0xFF0000}, 
    {0x00FF00}, 
    {0x0000FF}, 
};

// 测试
uint8_t w[] = {
            0, 2, 4, 6, 
            9, 12, 15, 18,
            22, 26, 30, 34,
            39, 44, 49, 54,
            60, 66, 72, 78,
            85, 92, 99, 106,
            114, 122
};

void _ws2812_task(void *param)
{
    ESP_ERROR_CHECK(WS2812_Loop_Start(25));

    // 测试
    int i = 0;
    int a = 1;
    while (1)
    {
        color[0].r = w[i];
        color[0].g = w[i];
        color[0].b = w[i];
        color[1].r = w[i];
        color[1].g = w[i];
        color[1].b = w[i];
        i += a;
        if (i == sizeof(w) - 1) a = -1;
        if (i == 0)    a = 1;
        ESP_ERROR_CHECK(WS2812_Fill_Buffer(color, 2));
        vTaskDelay(40 / portTICK_RATE_MS);
    }
}

void app_main()
{
    // 等待外设上电
    vTaskDelay(50 / portTICK_RATE_MS);

    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // TODO: 判断重置键是否长按

    // Lamp
    ESP_ERROR_CHECK(Lamp_Init(NULL));
    WS2812_COLOR_t color = {
        .r = 0,
        .g = 0,
        .b = 0xFF
    };
    ESP_ERROR_CHECK(Lamp_Set_Color(&color));

    // WS2812 测试
    // xTaskCreatePinnedToCore(_ws2812_task, "ws2812_task", 2048, NULL, 9, NULL, 1);


    //GameCube Contoller reading init
    // rmt_tx_init();
    // rmt_rx_init();
    // xTaskCreatePinnedToCore(get_buttons, "gbuttons", 2048, NULL, 1, NULL, 1);


    memset(&keys_data, 0, sizeof(keys_data));
    // TODO: 读取脚本设置中的手柄类型
    _joy_type = Left_Joycon; // Switch_pro));

    // 启动蓝牙
    ESP_ERROR_CHECK(NS_Controller_init(_joy_type, _ns_controller_cb)); 
    NS_Set_Battery(Battery_Full);
    // NS_Set_Battery(Battery_Level_3);
    // NS_Set_Charging(true);


    // TODO: 从flash 读取匹配过的地址
    esp_bd_addr_t bd_addr = {0x58, 0x2F, 0x40, 0xDA, 0xAF, 0x01};
 
    if (bd_addr == NULL)
        ESP_ERROR_CHECK(NS_Scan());
    else
    {
        xTaskCreatePinnedToCore(_wait_connet_task, "wait_connet_task", 2048, bd_addr, 10, NULL, 1);

    }


    Button_Config_t btn_config = Button_Default_Config(GPIO_NUM_0, _button_0_Callback);
    _button_0_handle = Button_Enable(&btn_config);
    if ( _button_0_handle == NULL)
        ESP_LOGI(TAG, "BUTTON Enable failed!");

    // 启动串口0命令
    UART_0_Rx_Start();

    // gpio_config_t io_conf;
    // //disable interrupt
    // io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = PIN_SEL;
    // //disable pull-down mode
    // io_conf.pull_down_en = 0;
    // //disable pull-up mode
    // io_conf.pull_up_en = 0;
    // //configure GPIO with the given settings
    // gpio_config(&io_conf);


    // //start blinking
    // xTaskCreate(startBlink, "blink_task", 1024, NULL, 1, &BlinkHandle);
}
