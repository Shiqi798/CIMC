#ifndef __MSG_APP_H
#define __MSG_APP_H

#include "msg.h"

msg_result_t msg_app_handle_cmd(msg_frame_t *frame);
void msg_app_after_send(void);
void msg_app_auto_report_poll(void);
uint8_t msg_app_auto_report_is_active(void);
void msg_app_alarm_print(void);
void msg_app_reset_alarm_state(void);

#endif
