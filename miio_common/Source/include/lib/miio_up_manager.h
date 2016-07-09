#ifndef MIIO_UP_MANAGER_H_
#define MIIO_UP_MANAGER_H_

enum mum_error_code{
	MUM_OK = 0,
	MUM_FAIL,
	MUM_NOT_INIT,
	MUM_ITEM_NONEXIST,
	MUM_KEY_TOO_LONG,
	MUM_VALUE_TOO_LONG,
	MUM_KEY_INVALID,
	MUM_VALUE_INVALID
};

typedef struct mum_struct *mum;

/** 创建MIIO UP MANAGER对象
 *
 *  @return 成功时返回对象句柄，失败返回NULL
 */
mum mum_create(void);

/** 销毁MIIO UP MANAGER对象
 *
 *  @param[in] handle: 要销毁的对象句柄的指针
 */
void mum_destroy(mum* handle);

/** 与mum_create相同，但不创建周期性上报线程
 *
 *  @return 成功时返回对象句柄，失败返回NULL
 */
mum mum_create2(void);

/** 销毁MIIO UP MANAGER对象
 *
 *  @param[in] handle: 要销毁的对象句柄的指针
 */
void mum_destroy2(mum* handle);

/** 获取一个属性的值
 *
 *  @param[in] self: 对象句柄
 *  @param[in] key: 属性的名称字符串
 *  @param[out] value: 属性的值字符串
 *
 *  @return 成功返回0，失败返回负值
 */
int mum_get_property(mum self, const char *key, char *value);

/** 添加或更新一个属性
 *
 *  @param[in] self: 对象句柄
 *  @param[in] key: 属性的名称字符串
 *  @param[in] value: 属性的值字符串
 *
 *  @return 成功返回0，失败返回负值
 */
int mum_set_property(mum self, const char *key, const char *value);

/** 添加或更新一个属性，但该属性不会再mum_send_property时上报
 *
 *  @param[in] self: 对象句柄
 *  @param[in] key: 属性的名称字符串
 *  @param[in] value: 属性的值字符串
 *
 *  @return 成功返回0，失败返回负值
 */
int mum_set_property_without_report(mum self, const char *key, const char *value);

/** 发送所有更新过的属性
 *
 *  @param[in] self: 对象句柄
 *
 *  @return 成功时返回发送的属性个数，失败返回负值
 */
int mum_send_property(mum self);

/** 发送所有更新过的属性
 *
 *  @param[in] self: 对象句柄
 *  @param[in] retry: 应用层重试次数
 *
 *  @return 成功时返回发送的属性个数，失败返回负值
 */
int mum_send_property_with_retry(mum self, uint32_t retry);

/** 添加一个事件
 *
 *  @param[in] self: 对象句柄
 *  @param[in] key: 事件的名称字符串
 *  @param[in] value: 事件的参数字符串
 *
 *  @return 成功返回0，失败返回负值
 */
int mum_set_event(mum self, const char *key, const char *value);

/** 上报缓存中的所有事件
 *
 *  @param[in] self: 对象句柄
 *  @param[in] retry：传输层重试次数
 *
 *  @return 成功返回上传事件个数，失败返回负值
 */
int mum_send_event_with_retry(mum self, uint32_t retry);

/** 上报缓存中的一条事件
 *
 *  @param[in] self: 对象句柄
 *  @param[in] retry：传输层重试次数
 *
 *  @return 成功返回上传事件个数，失败返回负值
 */
int mum_send_one_event_with_retry(mum self, uint32_t retry);

/** 发送所有未发送的事件
 *
 *  @param[in] self: 对象句柄
 *
 *  @return 成功时返回发送的事件个数，失败返回负值
 */
int mum_send_event(mum self);

/** 添加一个log
 *
 *  @param[in] self: 对象句柄
 *  @param[in] key: 事件的名称字符串
 *  @param[in] value: 事件的参数字符串
 *
 *  @return 成功返回0，失败返回负值
 */
int mum_set_log(mum self, const char *key, const char *value);
int mum_set_store(mum self, const char *key, const char *value);

/** 发送所有未发送的log
 *
 *  @param[in] self: 对象句柄
 *
 *  @return 成功时返回发送的事件个数，失败返回负值
 */
int mum_send_log(mum self);

typedef struct{
    char* js;//json string,construct by user call_back
    size_t size;
    size_t js_len;
    size_t count;//how many times has call this call_back func
}sync_info_t;
/** sync info 
 *
 *  @param[in] key 
 *  @param[in] call_back
 *  @example:
int get_sync_timer_info(sync_info_t* info){//used to get timer info,as callback
    LOG_INFO("sizeof js is :%d\r\n",info->size);
    if(info->js == NULL)return -1;

    if(info->count == 1){
        info->js_len = 0;
        info->js_len += snprintf(info->js,info->size - info->js_len,"{\"method\":\"_sync.getUserSceneInfo\",\"params\":[{\"magic\":1451455746,\"total\":2,\"cur\":%d,\"timer\":[{\"id\":\"F12345678910\"},{\"id\":\"F12345678911\"}]}]}",info->count);
        LOG_INFO("strlenof js is: %d\r\n",strlen(info->js));
        return 1;
    }
    else if(info->count == 2){
        info->js_len = 0;
        info->js_len += snprintf(info->js,info->size - info->js_len,"{\"method\":\"_sync.getUserSceneInfo\",\"params\":[{\"magic\":1451455746,\"total\":2,\"cur\":%d,\"timer\":[{\"id\":\"F12345678912\"},{\"id\":\"F12345678913\"}]}]}",info->count);
        LOG_INFO("strlenof js is: %d\r\n",strlen(info->js));
        return 0;
    }
    else{
        return 0;
    }
}

 *  @call after otn is online,like this:
    while(!otn_is_online())api_os_tick_sleep(100);
    mum_send_sync_info("timer",get_sync_timer_info);//start sync timer info
 *  
 */
#endif /* MIIO_UP_MANAGER_H_ */
