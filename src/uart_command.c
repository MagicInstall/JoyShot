/*
    上位机串口命令

    2021-08-25  wing    创建.
 */

#include "esp_log.h"
#include "uart_command.h"
#include "ns_controller.h"

static const char *TAG = "UART_COMM";

static NS_Controller_Type_t _joy_type = Left_Joycon; // TODO: 暂时简单设置默认值
static ns_button_status_t _keys_data;

#define KEY_MASK_Y          0b00000001
#define KEY_MASK_X          0b00000010
#define KEY_MASK_B          0b00000100
#define KEY_MASK_A          0b00001000
#define KEY_MASK_Right_SR   0b00010000
#define KEY_MASK_Right_SL   0b00100000
#define KEY_MASK_R          0b01000000
#define KEY_MASK_ZR         0b10000000
#define KEY_MASK_Minus      (0b00000001 << 8)
#define KEY_MASK_Plus       (0b00000010 << 8)
#define KEY_MASK_R_Stick    (0b00000100 << 8)
#define KEY_MASK_L_Stick    (0b00001000 << 8)
#define KEY_MASK_Home       (0b00010000 << 8)
#define KEY_MASK_Capture    (0b00100000 << 8)
// #define KEY_MASK_R          (0b01000000 << 8)
#define KEY_MASK_Char_Grip  (0b10000000 << 8)
#define KEY_MASK_Down       (0b00000001 << 16)
#define KEY_MASK_Up         (0b00000010 << 16)
#define KEY_MASK_Right      (0b00000100 << 16)
#define KEY_MASK_Left       (0b00001000 << 16)
#define KEY_MASK_Left_SR    (0b00010000 << 16)
#define KEY_MASK_Left_SL    (0b00100000 << 16)
#define KEY_MASK_L          (0b01000000 << 16)
#define KEY_MASK_ZL         (0b10000000 << 16)

#define UART_SET_KEY(keys) {                                \
    _keys_data.button_status_1 |= keys & 0xFF;               \
    _keys_data.button_status_2 |= (keys & 0xFF00) >> 8;      \
    _keys_data.button_status_3 |= (keys & 0xFF0000) >> 16;   \
}

#define UART_RES_KEY(keys) {                                \
    _keys_data.button_status_1 &= ~(keys & 0xFF);            \
    _keys_data.button_status_2 &= ~((keys & 0xFF00) >> 8);   \
    _keys_data.button_status_3 &= ~((keys & 0xFF0000) >> 16);\
}

//いずれamiibo実装するならここ拡張するかも
#define UART_BUF_SIZE   256
QueueHandle_t uart_queue;

static void _uart_rx_task()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 10, &uart_queue, 0));
 
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE);
    
    uint32_t r_key = 0;
    bool press = false;
    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, UART_BUF_SIZE, 500 / portTICK_RATE_MS); // 用portMAX_DELAY 会等到buff 满才跳出阻塞 );
        if (len < 2) continue;

        // ESP_LOGI(TAG, "%s", data);
        r_key = 0;
        press = false;

        
        switch (data[0])
        {
        /* -------- 基本命令以'j'开头 -------- */
        case 'j':
        case 'J':
            switch (data[1])
            {
            // 设置手柄类型
            case 't':
            case 'T':
                switch (data[2])
                {
                case '1': _joy_type = Left_Joycon; break;
                case '2': _joy_type = Right_Joycon; break;
                case '3': _joy_type = Switch_pro; break;
                default: ESP_LOGI(TAG, "Unknow Joy Type: %c", data[2]); break;
                }
                break;
            
            default: ESP_LOGI(TAG, "Unknow command: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        
        /* -------- 以下是发送按键的命令 -------- */
        case 'y':
        case 'Y':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Y; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Y = 1; break;
            case '0': _keys_data.Y = 0; break;
            default: ESP_LOGI(TAG, "Y error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'x':
        case 'X':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_X; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.X = 1; break;
            case '0': _keys_data.X = 0; break;
            default: ESP_LOGI(TAG, "X error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'b':
        case 'B':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_B; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.B = 1; break;
            case '0': _keys_data.B = 0; break;
            default: ESP_LOGI(TAG, "B error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'a':
        case 'A':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_A; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.A = 1; break;
            case '0': _keys_data.A = 0; break;
            default: ESP_LOGI(TAG, "A error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'v':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Down; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Down = 1; break;
            case '0': _keys_data.Down = 0; break;
            default: ESP_LOGI(TAG, "Down error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case '^':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Up; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Up = 1; break;
            case '0': _keys_data.Up = 0; break;
            default: ESP_LOGI(TAG, "Up error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case '>':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Right; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Right = 1; break;
            case '0': _keys_data.Right = 0; break;
            default: ESP_LOGI(TAG, "Right error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case '<':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Left; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Left = 1; break;
            case '0': _keys_data.Left = 0; break;
            default: ESP_LOGI(TAG, "Left error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'h':
        case 'H':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Home; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Home = 1; break;
            case '0': _keys_data.Home = 0; break;
            default: ESP_LOGI(TAG, "Home error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'c':
        case 'C':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_Capture; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Capture = 1; break;
            case '0': _keys_data.Capture = 0; break;
            default: ESP_LOGW(TAG, "Capture error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'l':
        case 'L':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_L; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.L = 1; break;
            case '0': _keys_data.L = 0; break;
            case 'r':
            case 'R':
                switch (data[2])
                {
                case 'p': case 'P': 
                    switch (_joy_type)
                    {
                    case Left_Joycon:  r_key = KEY_MASK_Left_SL | KEY_MASK_Left_SR; break;
                    case Right_Joycon: r_key = KEY_MASK_Right_SL | KEY_MASK_Right_SR; break;
                    case Switch_pro:   r_key = KEY_MASK_L | KEY_MASK_R; break;
                    default: ESP_LOGW(TAG, "L & R error: %c(0x%X)", data[2], data[2]); break;
                    }
                    press = true; __attribute__ ((fallthrough));
                case '1': 
                    switch (_joy_type)
                    {
                    case Left_Joycon:  _keys_data.Left_SL = 1; _keys_data.Left_SR = 1; break;
                    case Right_Joycon: _keys_data.Right_SL = 1; _keys_data.Right_SR = 1; break;
                    case Switch_pro:   _keys_data.L = 1; _keys_data.R = 1; break;
                    default: break;
                    }
                    break;
                case '0': 
                    switch (_joy_type)
                    {
                    case Left_Joycon:  _keys_data.Left_SL = 0; _keys_data.Left_SR = 0; break;
                    case Right_Joycon: _keys_data.Right_SL = 0; _keys_data.Right_SR = 0; break;
                    case Switch_pro:   _keys_data.L = 0; _keys_data.R = 0; break;
                    default: break;
                    }
                    break;
                default: ESP_LOGI(TAG, "L+R error: %c(0x%X)", data[2], data[2]); break;
                }
                break;
            case 's':
            case 'S':
                switch (data[2])
                {
                case 'p': case 'P': r_key = KEY_MASK_L_Stick; press = true; __attribute__ ((fallthrough));
                case '1': _keys_data.LStick = 1; break;
                case '0': _keys_data.LStick = 0; break;
                default: ESP_LOGI(TAG, "L Stick error: %c(0x%X)", data[2], data[2]); break;
                }
                break;
            default: ESP_LOGI(TAG, "L error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        case 'r':
        case 'R':
            switch (data[1])
            {
            case 'p': case 'P': r_key = KEY_MASK_R; press = true; __attribute__ ((fallthrough));
            case '1': _keys_data.Capture = 1; break;
            case '0': _keys_data.Capture = 0; break;
            case 's':
            case 'S':
                switch (data[2])
                {
                case 'p': case 'P': r_key = KEY_MASK_R_Stick; press = true; __attribute__ ((fallthrough));
                case '1': _keys_data.RStick = 1; break;
                case '0': _keys_data.RStick = 0; break;
                default: ESP_LOGI(TAG, "R Stick error: %c(0x%X)", data[2], data[2]); break;
                }
                break;
            default: ESP_LOGI(TAG, "R error: %c(0x%X)", data[1], data[1]); break;
            }
            break;
        
        default:
            ESP_LOGI(TAG, "unknow commade: %c(0x%X)", data[0], data[0]);
            break;
        }

        NS_Set_Buttons(&_keys_data);

        if (press) 
        {
            vTaskDelay(20);
            UART_RES_KEY(r_key);
            NS_Set_Buttons(&_keys_data); 
        }
        ESP_LOGI(TAG, "heap free: %d", xPortGetFreeHeapSize());
    }
}

void UART_0_Rx_Start(void)
{
    // TODO: 检查官方的AT 命令是否启用
    
    xTaskCreate(_uart_rx_task, "uart_rx_task", 2048, NULL, 10, NULL);
}
