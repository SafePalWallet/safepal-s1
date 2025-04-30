#ifndef WALLET_DEVICE_MANAGER_H
#define WALLET_DEVICE_MANAGER_H
#if __cplusplus
extern "C" {
#endif

#define BATTERY_HIGH_LEVEL 50
#define BATTERY_MIDDLE_LEVEL 30
#define BATTERY_LOW_LEVEL 10


int device_manager_init();

int device_battery_check();

int device_battery_is_online();

int device_battery_capacity();

int device_battery_voltage();

int device_battery_status();

int device_battery_is_charging();

int device_usb_update_connnect_state();

int device_usb_is_connnect();

int device_has_ext_power();

int device_usb_online();

long long device_usb_online_time();

long device_last_active_time();

int device_set_last_active_time(long time);

int device_set_last_input_time(long time);

int device_event_init();

int device_screen_on();

int device_screen_off();

int device_is_screen_on();

long device_screen_off_time();

int device_shutdown(int restart);

int device_start_ota();

int device_get_usb_config(char *config, int size);

int device_set_usb_config(const char *config);

int device_is_refuse_shutdown();

int device_can_brightness_adjust();

int device_set_brightness_level(int level);

#if __cplusplus
}
#endif
#endif
