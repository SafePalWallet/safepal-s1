#define LOG_TAG "EventManager"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "EventManager.h"

#include "device_manager.h"

#include "uevent.h"
#include <sys/mman.h>

#include <linux/input.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

#include <pthread.h>
#include <string.h>
#include "common.h"
#include "device.h"
#include "fileutil.h"

#define POWER_BATTERY            "power_supply/battery"
#define UEVENT_CHANGE            "change"

EventManager::EventManager()
		: mListener(NULL) {
}

EventManager::~EventManager() {
	exit();
}

void *ueventThread(void *param) {
	db_msg("ueventThread start");
	EventManager *cm = (EventManager *) param;
	if (NULL == cm) {
		db_msg("invalid param");
		return 0;
	}
	cm->ueventLoop();
	
	return 0;
}


int EventManager::init() {
	uevent_init();

	pthread_t eventid = 0;
	pthread_create(&eventid, NULL, ueventThread, this);

	return 0;
}

int EventManager::exit(void) {
	return 0;
}

void EventManager::setEventListener(EventListener *pListener) {
	mListener = pListener;
}

static bool isBatteryEvent(char *udata) {
	const char *tmp_start;
	const char *tmp_end;
	tmp_start = udata;
	tmp_end = udata + strlen(udata) - strlen(POWER_BATTERY);

	if (strcmp(tmp_end, POWER_BATTERY)) { //battery
		return false;
	}
	if (strncmp(tmp_start, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return false;
	}
	return true;
}

int EventManager::ueventLoop(void) {
	char udata[1024] = {0};
	int ret = 0;

	while(1){
        if (!GLobal_Is_Se_Processing && !GLobal_Is_Se_Upgrading) {
#ifdef CONFIG_SUPPORT_USB_POWER
		device_usb_update_connnect_state();
#endif

#ifdef CONFIG_HAVE_BATTERY
		//if (isBatteryEvent(udata)) 
		{
			ret = device_battery_check();
			
			if (ret > 0) {//online or offline change 
				mListener->notify(ret, 0);
			} else {
				//default
				ret = EVENT_BATTERY_CHANGE;
				mListener->notify(ret, 0);
			}
			//return ret;
		}
#endif

		sleep(3);
	}
	db_msg("The event is not handled");
	return 0;
}

