/**
 *  使用WS2812 LED 显示各种信息
 * 
 *  2021-09-05    wing    创建.
 */

#include <string.h>
#include "lamp.h"
#include "esp_log.h"

static const char *TAG = "Lamp";

static bool _initialized = false;

esp_err_t Lamp_Init(WS2812_CONFIG_t *ws2812_config, CPC405x_LDO_Config_t *ldo_config) {
	ESP_LOGI(TAG, "Init");

	esp_err_t ret = CPC405x_LDO_Init(ldo_config);
	if (ret != ESP_OK) return ret;
	
	ret = WS2812_Init(ws2812_config);
	if (ret != ESP_OK) return ret;

	_initialized = true;
	
	// 启动一次自动刷新，
	// 让自动刷新器内部的定时器在本核心程内创建，
	// 从而将定时器的中断固定在本核心。
	ret = WS2812_Loop_Start(1, NULL);
	if (ret != ESP_OK) return ret;
	return WS2812_Loop_Stop();
}

esp_err_t Lamp_Set_Single_Color(WS2812_COLOR_t *color) {
	esp_err_t ret = Lamp_On();
	if (ret != ESP_OK) return ret;

	static WS2812_COLOR_t buf[WS2812_COUNT];
	for (size_t i = 0; i < WS2812_COUNT; i++){
		memcpy(buf + i, color, 1);
	// 	_color_correction(buf + i, color, 1);
	}

	return WS2812_Send_LEDs(buf, WS2812_COUNT);
}

// static TaskHandle_t 	_effect_task_handle = NULL;
static Lamp_Effect_t   *_effect		= NULL;

static void _effect_task(WS2812_Event_t val) {

	switch (val) {
		// 将要刷新
		case WS2812_Event_Before_Refresh:
			// 取锁
			xSemaphoreTake(_effect->semaphore, portMAX_DELAY);
			break;

		// 刚刷新过
		case WS2812_Event_After_Refresh:
			_effect->pauseTime = xTaskGetTickCount();
			_effect->virginity = false;
			// 推进到下一帧
			switch (_effect->frameMode) {
				case LAMP_EFFECT_FRAME_MODE_FORWARD:
					// 回到a帧
					if (_effect->clipping && 
						((_effect->current + 1) > _effect->b)) {

							_effect->current = _effect->a;
					} 
					// 回到头帧
					else if ((!_effect->clipping) && 
						((_effect->current + 1) > (_effect->length - 1))) {
							
							_effect->current = 0;
					}	
					// 继续前进
					else _effect->current ++;
					break;

				case LAMP_EFFECT_FRAME_MODE_TO_AND_FRO:
					// 到达换向点就翻转方向
					if (_effect->clipping) {
						if ((_effect->direction == LAMP_EFFECT_FRAME_DIRECTION_FORWARD) &&
							(_effect->current == _effect->b)) {
								
								_effect->direction = LAMP_EFFECT_FRAME_DIRECTION_REVERSE;
						}
						else if ((_effect->direction == LAMP_EFFECT_FRAME_DIRECTION_REVERSE) &&
							(_effect->current == _effect->a)) {
								
								_effect->direction = LAMP_EFFECT_FRAME_DIRECTION_FORWARD;
						}
					} else {
						if ((_effect->direction == LAMP_EFFECT_FRAME_DIRECTION_FORWARD) &&
							(_effect->current == (_effect->length - 1))) {
								
								_effect->direction = LAMP_EFFECT_FRAME_DIRECTION_REVERSE;
						}
						else if ((_effect->direction == LAMP_EFFECT_FRAME_DIRECTION_REVERSE) &&
							(_effect->current == 0)) {
								
								_effect->direction = LAMP_EFFECT_FRAME_DIRECTION_FORWARD;
						}
					}
					_effect->current += _effect->direction; 
					break;

				case LAMP_EFFECT_FRAME_MODE_SINGLE:
					// 若还没到达末尾帧就继续推进
					if ((_effect->clipping && (_effect->current < _effect->b)) || 
						((!_effect->clipping) && (_effect->current < (_effect->length - 1)))) {

							_effect->current ++;
					}
					// 已到达末尾帧就一直停在末尾。
					break;
				
				default:
					ESP_LOGW(TAG, "Unknow Lamp_Effect_Frame_Mode_t : %d \n", _effect->frameMode);
					break;
			}
			
			// 若是占位帧侧不输出
			if (_effect->frames[_effect->current] != LAMP_EFFECT_HOLDER_FRAME)
				// 提前输出下一帧到缓冲区
				ESP_ERROR_CHECK(WS2812_Fill_Buffer(_effect->table + _effect->frames[_effect->current], WS2812_COUNT));
			
			// 还锁
			xSemaphoreGive(_effect->semaphore);	
			break;
		
		default:
			ESP_LOGW(TAG, "Unknow WS2812_Event_t : %d \n", val);
			break;
	}

}

/**
 *  由于LAMP_EFFECT_STATE_MODE_PAUSE_ADD_ELAPSED 模式
 * 	计算从哪帧开始的过程代码比较长，
 *  因此将该过程拆分出来以便阅读。
 * 
 * 	TODO: 未测试...
 * 
 * @return 当方法返回的时候，effect->current已经被设置好，
 * 		   此返回值只是简单返回effect->current，不需要再赋值给effect->current。
 */
static uint32_t _pause_add_elapsed_mode(Lamp_Effect_t *effect) {
	// 算出距离上次暂停应该经过了多少帧(大约)
	TickType_t ticks = effect->virginity ? 0 : xTaskGetTickCount() - effect->pauseTime;
	uint32_t ms = ( ( ((uint64_t) ticks) * 1000) / configTICK_RATE_HZ );
	uint32_t elapsed = ms / (1000 / effect->freq);

	// 根据不同的播放模式计算从哪帧开始
	switch (effect->frameMode)
	{
	case LAMP_EFFECT_FRAME_MODE_FORWARD:
		// 裁剪模式
		if (effect->clipping) {
			uint32_t len = effect->b - effect->a + 1;
			effect->current = (((effect->current - effect->a)) % len) + effect->a;
		}
		// 完整模式 
		else {
			effect->current = (elapsed + effect->current) % (effect->length);
		}
		break;

	case LAMP_EFFECT_FRAME_MODE_SINGLE:
		// 裁剪模式
		if (effect->clipping) {
			if ((effect->current + elapsed) > effect->b)
				effect->current = effect->b;
			else
				effect->current += elapsed; 
		}
		// 完整模式 
		else {
			if ((effect->current + elapsed) > (effect->length - 1))
				effect->current = effect->length - 1;
			else
				effect->current += elapsed; 			
		}
		break;
	
	case LAMP_EFFECT_FRAME_MODE_TO_AND_FRO:
		// 裁剪模式，原理同完整模式
		if (effect->clipping) {
			uint32_t len = effect->b - effect->a + 1;
			// effect->current = ((elapsed + (effect->current - effect->a)) % len) + effect->a;
			uint32_t double_length  = len > 1 ? 
									  (len - 1/*减去尾帧*/) * 2 :
									  1;

			// 裁剪模式的当前位置先偏移到0开始算
			uint32_t double_current = effect->direction == LAMP_EFFECT_FRAME_DIRECTION_FORWARD?
									  effect->current - effect->a :
									  double_length - (effect->current - effect->a);
			// 算出双倍长度在偏移之后（从0开始）的位置
			effect->current = (elapsed + double_current) % (double_length);

			// 位置超过长度就表示在反向播放
			if (effect->current >= (len -1)) {
				effect->current = (len -1) - (effect->current - (len - 1));
				effect->direction = LAMP_EFFECT_FRAME_DIRECTION_REVERSE;
			} else {
				effect->direction = LAMP_EFFECT_FRAME_DIRECTION_FORWARD;
			}

			// 偏移回裁剪模式的位置
			effect->current += effect->a;
		}
		// 完整模式 
		else {
			// 往复模式的播放长度当作正向模式的2倍，
			// 由于两方向的尾帧（反向的尾帧就是正向的头帧）不会连着播两次，
			// 所以计算的时候去掉1帧。
			uint32_t double_length  = effect->length > 1 ? 
									  (effect->length - 1/*减去尾帧*/) * 2 :
									  1;

			uint32_t double_current = effect->direction == LAMP_EFFECT_FRAME_DIRECTION_FORWARD?
									  effect->current :
									  double_length - effect->current;
			// 算出双倍长度之后的位置
			effect->current = (elapsed + double_current) % (double_length);

			// 位置超过长度就表示在反向播放
			if (effect->current >= (effect->length -1)) {
				effect->current = (effect->length -1) - (effect->current - (effect->length - 1));
				effect->direction = LAMP_EFFECT_FRAME_DIRECTION_REVERSE;
			} else {
				effect->direction = LAMP_EFFECT_FRAME_DIRECTION_FORWARD;
			}
		}

		break;
	
	default:
		ESP_LOGW(TAG, "Unknow Lamp_Effect_Frame_Mode_t : %d \n", effect->frameMode);
		break;
	}
	return effect->current;
}

esp_err_t Lamp_Effect_Start(Lamp_Effect_t *effect) {
	ESP_ERROR_CHECK((effect->freq < 1) || (effect->freq > 1000));
	ESP_ERROR_CHECK(effect->length < 1);
	ESP_ERROR_CHECK((effect->clipping) && (effect->a > effect->b));
	ESP_ERROR_CHECK(effect->frames == NULL);
	ESP_ERROR_CHECK(effect->table == NULL);

	// TODO: 测试用
	while (_initialized == false)
	{
		ESP_LOGW(TAG, "Init not completed...");
	}
	
	// if (_effect_task_handle) Lamp_Effect_Stop();

	// 取锁
	if (effect->semaphore == NULL)
		effect->semaphore = xSemaphoreCreateMutex();
	if (effect->semaphore == NULL)
		return ESP_FAIL;
	xSemaphoreTake(effect->semaphore, portMAX_DELAY);

	// 计算从哪帧开始
	switch (effect->startMode)
	{
	case LAMP_EFFECT_STATE_MODE_RESTART:
		effect->current = 0;
		break;

	case LAMP_EFFECT_STATE_MODE_PAUSE: break;
	
	case LAMP_EFFECT_STATE_MODE_PAUSE_ADD_ELAPSED:
		_pause_add_elapsed_mode(effect);
		break;
	
	default:
		ESP_LOGW(TAG, "Unknow Lamp_Effect_Start_Mode_t : %d \n", effect->startMode);
		break;
	}
	// 还锁
	xSemaphoreGive(effect->semaphore);

	// 打开电源
	esp_err_t ret = Lamp_On();
	if (ret != ESP_OK) return ret;

	//----------------------------------------------------
	//-- 注意后面是操作内部的_effect对象，不是参数的 effect ！ --


	// 等待一个正在输出的帧完成
	Lamp_Effect_t *last_eff = _effect;
	if (last_eff) xSemaphoreTake(last_eff->semaphore, portMAX_DELAY);

	// 更新光效数据的指针
	_effect = effect;

	// 先输出第一帧到缓冲区
	ESP_ERROR_CHECK(WS2812_Fill_Buffer(_effect->table + _effect->frames[_effect->current], WS2812_COUNT));

	if (last_eff) xSemaphoreGive(last_eff->semaphore);

	ret = WS2812_Loop_Start(_effect->freq, _effect_task);
	return ret;
	// esp_err_t ret = WS2812_Loop_Start(_effect->freq, _effect_task);
	// if (ret != ESP_OK) return ret;

	// if (_effect_task_handle) {
	// 	ret = xTaskCreate(_effect_task, "LampEffect", 1024, NULL, 10, &_effect_task_handle);

	// }

    // return ret == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t Lamp_Effect_Stop(void) {
	// if (_effect_task_handle) {
	// 	if (vTaskDelete(_effect_task_handle) != pdPASS)
	// 		return ESP_FAIL;

	// 	_effect_task_handle = NULL;
	// 	_effect 	= NULL; 
	// }
	
	Lamp_Off();
	WS2812_Loop_Stop();

	// 等待一个正在输出的帧完成
	if (_effect) {
		Lamp_Effect_t *tmp = _effect; // 暂时取代_effect以便在_effect = NULL后还锁
		xSemaphoreTake(tmp->semaphore, portMAX_DELAY);
		_effect->pauseTime = xTaskGetTickCount();
		_effect = NULL; 
		xSemaphoreGive(tmp->semaphore);
	}


	return ESP_OK;
}