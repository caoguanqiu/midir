#ifndef MIIO_UP_MANAGER_PRIV_H_
#define MIIO_UP_MANAGER_PRIV_H_

#include <lib/miio_up_manager.h>

int mum_get_event(mum self, const char *key, char *value);
int mum_get_property_no(mum self);
int mum_get_event_no(mum self);
int mum_get_specific_event_no(mum self, const char *key);
uint32_t mum_make_json_property(mum self, char *js, uint32_t *js_len, uint32_t *session);
int mum_send_property_ack(jsmn_node_t* pjn, void* ctx);
int mum_send_event_ack(jsmn_node_t* pjn, void* ctx);
int mum_send_log_ack(jsmn_node_t* pjn, void* ctx);
int mum_send_common_data_ack(jsmn_node_t* pjn, void* ctx);

uint32_t mum_get_dirty_property_no(mum self);
uint32_t mum_get_sending_property_no(mum self);
uint32_t mum_get_sent_property_no(mum self);

#endif /* MIIO_UP_MANAGER_PRIV_H_ */
