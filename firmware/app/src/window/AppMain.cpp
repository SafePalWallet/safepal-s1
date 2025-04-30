#define LOG_TAG "AppMain"

#include "common.h"
#include "AppMain.h"
#include <pthread.h>
#include <stdlib.h>
#include "minigui_comm.h"
#include "device.h"
#include "fileutil.h"
#include "CameraManager.h"
#include "device_manager.h"
#include "secure_api.h"
#include "wallet_proto.h"
#include "storage_manager.h"
#include "wallet_manager.h"
#include "loading_win.h"

#ifdef BUILD_FOR_DEV

#include "upgrade_helper.h"

#endif

typedef struct {
	int request;
} ctl_common_req;

//#define DEBUG_CALLBACK_MESSAGE

#ifdef DEBUG_CALLBACK_MESSAGE
#define cbmsg_xdebug(fmt, arg...) db_msg("CALLBACK_MSG_DEBUG: "fmt, ##arg)
#else
#define cbmsg_xdebug(fmt, arg...)
#endif


AppMain *AppMain::sAppMain = NULL;
GuiCallBack *AppMain::sGuiCallBack = NULL;

AppMain *AppMain::getInstance() {
	return sAppMain;
}

bool AppMain::haveGuiCallBack() {
	return sGuiCallBack != NULL;
}

GuiCallBack *AppMain::getGuiCallBack() {
	return sGuiCallBack;
}

void AppMain::setGuiCallBack(GuiCallBack *callBack) {
	sGuiCallBack = callBack;
}

AppMain::AppMain() {
	sAppMain = this;
	mCameraManager = NULL;
	mEventManager = NULL;

	mShutdowning = false;
	mShutdownFutureTime = 0;
	mShutdownCountdownTime = 0;
	mForceReboot = 0;
	mShutdowLevel = 0;
	memset(mCallbackMessageBuff, 0, sizeof(mCallbackMessageBuff));

	pthread_cond_init(&mMsgCondition, NULL);
	pthread_mutex_init(&mMsgMutex, NULL);
	wallet_init0();
	mMessageHeader = NULL;
}

AppMain::~AppMain() {
	if (mCameraManager) {
		delete mCameraManager;
		mCameraManager = NULL;
	}
	if (mEventManager) {
		delete mEventManager;
		mEventManager = NULL;
	}
}

int AppMain::init0() {
	db_msg("AppMain::init call running version:%s", PRODUCT_VERSION);
	device_manager_init();
	mCameraManager = new CameraManager();
	mEventManager = new EventManager();
	return 0;
}

int AppMain::initSystemEnv() { return 0;}

int AppMain::init() {
	db_msg("init start");
	DEBUG_PERFORMANCE();
	mCameraManager->init();
	DEBUG_PERFORMANCE();
	return 0;
}

static void *shutdownThread(void *param) {
	db_msg("shutdownThread start");
	AppMain *cm = (AppMain *) param;
	if (NULL == cm) {
		db_msg("invalid param");
		return 0;
	}
	cm->shutdownLoop();
	return 0;
}

void AppMain::shutdownLoop() {
	while (mEventManager == NULL) {
		sleep(1);
	}
	long currentTime = getClockTime();
	long shutdownWaitTime = 0;
	long screenofftime = 0;
	int screenSaverTime = 0;
	settings_backup_to_nvm(0);

	int hw_break_cnt = 0;
	if (device_is_inited() && device_get_hw_break_state(3) == 1) {
		Global_HW_Break_State = 1;
	}
	device_set_last_input_time(currentTime);
	device_set_last_active_time(currentTime);
	while (true) {
		currentTime = getClockTime();
		if (Global_LosePower) {
			db_msg("----->> lose power happen...");
			if(device_is_refuse_shutdown()){
				db_msg("usb is online refuse shutdown");
			}else{
				break;
			}
		}
		if (mShutdownFutureTime > 0 && currentTime >= mShutdownFutureTime) {
			db_msg("----->> ShutdownFutureTime:%ld happen...", mShutdownFutureTime);
			if(device_is_refuse_shutdown()){
				db_msg("usb is online refuse shutdown");
			}else{
				break;
			}
		}

		if (mShutdownCountdownTime > 0) {
			if (currentTime <= mShutdownCountdownTime) {
				if ((mShutdownCountdownTime - currentTime <= 4) && canCancelShutdown()) {
					db_msg("cancel ShutdownCountdownTime 0");
					mShutdownCountdownTime = 0;
				}
			} else {
				if (device_usb_online() > 0) {
					mShutdownCountdownTime = currentTime + 10;
					db_msg("cancel ShutdownCountdownTime,app connect and use online");
				} else {
					db_msg("----->> ShutdownCountdownTime:%ld happen...", mShutdownCountdownTime);
					if(device_is_refuse_shutdown()){
						db_msg("usb is online refuse shutdown");
					}else{
						break;
					}
				}
			}
		}

		if (mForceReboot) {
			db_msg("----->> force reboot:%d", mForceReboot);
			sleep(1);
			if(device_is_refuse_shutdown()){
				db_msg("usb is online refuse shutdown");
			}else{
				break;
			}
		}

		sleep(1);
		currentTime++;
		settings_backup_to_nvm(0);
		storage_trySync2Disk();

		if (Global_HW_Break_State > 0 && Global_HW_Break_State < currentTime) {
			if (device_get_hw_break_state(3) == 1) {
				Global_HW_Break_State = currentTime;
				hw_break_cnt++;
			} else {
				hw_break_cnt = 0;
				Global_HW_Break_State = 0;
			}
			if (gHaveSeed && hw_break_cnt > 10) {
				db_serr("hw_break_cnt:%d,destory seed", hw_break_cnt);
				destoryDevice();
				hw_break_cnt = 0;
			}
		}

#ifdef CONFIG_HAVE_GUI
		if (loading_win_processing) {
			device_set_last_active_time(currentTime);
		} else if (device_is_screen_on()) {
			screenSaverTime = settings_get_screen_saver_time();
			if (screenSaverTime > 0 && (device_last_active_time() + screenSaverTime) < currentTime) {
				screenOff();
				Global_USB_State = device_usb_online();
			}
		}
#endif
		//screenOn when usb change 
		if(!device_is_screen_on() && (Global_USB_State!=device_usb_online()) && !Global_FactoryTesting){
			screenOn();
			set_temp_screen_time(0);
		}

		if (Global_FactoryTesting) {
			shutdownWaitTime = 0;
		} else {
			shutdownWaitTime = settings_get_auto_shutdown_time();
		}
		screenofftime = device_screen_off_time();
		if (currentTime % 60 == 0) {
			db_msg("version:%s now:%ld screenoff:%ld waitime:%ld", PRODUCT_VERSION, currentTime, screenofftime, shutdownWaitTime);
		}
		if (shutdownWaitTime > 0 && screenofftime > 0 && (screenofftime + shutdownWaitTime) < currentTime) {
			db_msg("Auto shutdown timenow:%ld screenofftime:%ld waitTime:%ld", currentTime, screenofftime, shutdownWaitTime);
			if(device_is_refuse_shutdown()){
				db_msg("usb is online refuse shutdown");
			}else{
				break;
			}
		}
	}

	db_msg("ready to shutdown timenow:%ld screenTime:%d shutdownWaitTime:%ld", currentTime, screenSaverTime, shutdownWaitTime);

	int level;
	if (Global_LosePower) {
		level = 3;
	} else {
		level = 1;
	}
	shutdown(level);
}

int AppMain::destoryDevice() {
	if (app_run_in_dev_mode()) {
		db_secure("dev mode,skip destory");
		return -1;
	}

	if (!device_is_inited()) {
		db_serr("device not inited,skip");
		return -1;
	}

	Global_Key_Shield = 1;
	int ret = wallet_destorySeed(0);
	db_secure("destorySeed ret:%d", ret);
	doFactoryReset();
	//force shutdown
	mShutdownFutureTime = getClockTime();
	return 0;
}

int AppMain::initStage2() {
	db_msg("initStage2 start");

	mEventManager->setEventListener(this);

	//check power
#ifdef CONFIG_HAVE_BATTERY
	if (device_usb_online() == 0) {
		if (device_battery_capacity() <= BATTERY_LOW_LEVEL) {
			sleep(1);
			mShutdowning = true;
			mShutdowLevel = 1;
			powerOffDelay();
			powerOffAsync();
			return 0;
		}
	}
#endif

	mEventManager->init(); //init later

	pthread_t shutdowntid = 0;
	pthread_create(&shutdowntid, NULL, shutdownThread, this);
	db_msg("initStage2 end");
	return 0;
}

bool AppMain::canCancelShutdown() {
	int state = 0;
#ifdef CONFIG_HAVE_BATTERY
	state = device_battery_is_online();
	db_msg("device_battery_is_online:%d", state);
	if (state == 1) {   
		return false;
	}
#endif
	return true;
}

void AppMain::onLosePower(int event, const char *msg, bool playsound) {
	bool shutdowncount = true;
	bool online = device_battery_is_online();
	int capacity = device_battery_capacity();
	if (capacity < 0) {
		capacity = 0;
	}
	int shutdown_time;
	if (capacity > BATTERY_HIGH_LEVEL) {
		shutdown_time = 10;
	} else if (capacity > BATTERY_MIDDLE_LEVEL) {
		shutdown_time = 5;
	} else if (capacity > BATTERY_LOW_LEVEL) {
		shutdown_time = 3;
	} else {
		shutdown_time = 2;
	}
	long timwnow = getClockTime();
	db_msg("event:%d -> %s btcapacity:%d online:%d shutdown count:%d %d now:%ld", event, msg, capacity, online, shutdowncount, shutdown_time, timwnow);

	if (shutdowncount) {
		if (mShutdownCountdownTime == 0 || (shutdown_time + timwnow) < mShutdownCountdownTime) {
			db_msg("set ShutdownCountdownTime:%d", shutdown_time);
			mShutdownCountdownTime = shutdown_time + timwnow;
		}
	}
}

void AppMain::onResumePower(int event, const char *msg) {
	db_msg("event:%d -> %s", event, msg);
	Global_LosePower = false;
	mShutdownCountdownTime = 0;
	if (mShutdowLevel != 0) { 
		mForceReboot = 1;
	}
}

bool AppMain::isPreviewing() {
	return mCameraManager->isPreviewing();
}

int AppMain::startPreview() {
	mCameraManager->startPreview();
	return 0;
}

int AppMain::stopPreview() {
	mCameraManager->stopPreview();
	return 0;
}

int AppMain::qrChunk(int datalen, char *data) {
	mCameraManager->qrChunk(datalen, data);
	return 0;
}

int AppMain::camExist() {
	return mCameraManager->camExist();
}

bool AppMain::isShutdownIng() {
	return mShutdowning;
}

bool AppMain::isScreenOn() {
	return device_is_screen_on();
}

bool AppMain::screenOn(bool force) {
	device_set_last_input_time(getClockTime());
	if (!force && device_is_screen_on()) {
		return true;
	}
	return device_screen_on() == 0;
}

bool AppMain::screenOff() {
	if (device_screen_off() != 0) {
		return false;
	}
	sendUiEvent(UI_EVENT_SCREEN_STATE, 0);
	return true;
}

static void *doPowerOffAsync(void *param) {
	db_msg("call shutdown");
	device_shutdown(0);
	return NULL;
}

void AppMain::powerOffAsync() {
	pthread_t threadid = 0;
	pthread_create(&threadid, NULL, doPowerOffAsync, this);
}

void AppMain::doPowerOffDelay() {
	int level = mShutdowLevel;
	int reboot = mForceReboot;
	db_msg("wait level:%d reboot:%d", level, reboot);
	sleep(10);//wait some time
	if (level > 0 && level < 9) {
		db_msg("call level:%d reboot:%d", level, reboot);
		device_shutdown(reboot);
	}
}

static void *do_power_off_delay(void *param) {
	AppMain *m = (AppMain *) param;
	m->doPowerOffDelay();
	return NULL;
}

void AppMain::powerOffDelay() {
	pthread_t threadid = 0;
	pthread_create(&threadid, NULL, do_power_off_delay, this);
}

void AppMain::shutdown(int level) {
	long clocktime = getClockTime();

	if(device_is_refuse_shutdown() || loading_win_processing){
		db_msg("usb is online refuse shutdown");
		return;
	}
	
	if (mShutdowLevel != 0) {
		db_msg("try shutdown but now level is:%d", level);
		return;
	}
	
	mCameraManager->stopPreview();//close scan before logo show
	usleep(20*1000);
	
	if (level > 10) {
		db_msg("shutdown >>> level is:%d", level);
		level = level % 10;
	}
	mShutdowLevel = level;
	mShutdowning = true;
	bool isLosePower = Global_LosePower;
	db_msg("shutdown >>> BEGIN level=%d LosePower:%d forceReboot:%d uptime:%ld", level, isLosePower, mForceReboot, clocktime);
	sendUiEvent(UI_EVENT_SHUTDOWN, level);	
	usleep(20*1000);//wait for shutdown logo show 
	
	settings_backup_to_nvm(0);
	storage_syncData2Disk();
	if (!isLosePower) {
		powerOffDelay();
	}
	db_msg("shutdown >>> level=%d step 1 forceReboot:%d", level, mForceReboot);
	db_msg("shutdown >>> step 10 ,level=%d LosePower:%d", level, isLosePower);
	mShutdowLevel = 9;
	if (mForceReboot) {
		db_msg("shutdown >>> we actually want reboot");
		device_shutdown(1);
	} else {
		powerOffAsync();
	}
	db_msg("shutdown >>> END level:%d LosePower:%d", level, isLosePower);
	int trytime = 0;
	do {
		sleep(2);
		device_shutdown(mForceReboot);
	} while (trytime++ < 5);
}

int AppMain::postMessage(int what, int arg1, int arg2, void *obj) {
	return postMessageDelay(0, what, arg1, arg2, obj);
}

int AppMain::postMessageDelay(int delay, int what, int arg1, int arg2, void *obj) {
	cbmsg_xdebug("postMessageDelay delay:%d what:%d arg1:%d", delay, what, arg1);
	pthread_mutex_lock(&mMsgMutex);
	WinCallbackMessage *msg = NULL;
	char flag = 0;
	int signal = 0;
	for (int i = 0; i < MAX_CALLBACK_MESSAGE_NUM; i++) {
		if (mCallbackMessageBuff[i].what == 0) {
			msg = &(mCallbackMessageBuff[i]);
			flag |= CALLBACK_MESSAGE_FLAG_STATIC;
			break;
		}
	}
	if (msg == NULL) {
		msg = (WinCallbackMessage *) (malloc(sizeof(WinCallbackMessage)));
		if (msg == NULL) {
			pthread_mutex_unlock(&mMsgMutex);
			return -1;
		}
	}

	long long when;
	if (delay > 0) {
		when = getMsClockTime() + delay;
	} else {
		when = 0;
		signal = 1;
	}
	msg->when = when;
	msg->what = what;
	msg->arg1 = arg1;
	msg->arg2 = arg2;
	msg->obj = obj;
	msg->flag = flag;

	WinCallbackMessage *cbmsg;

#ifdef DEBUG_CALLBACK_MESSAGE
	cbmsg = mMessageHeader;
	while (cbmsg != NULL) {
		cbmsg_xdebug("POST before: when:%lld what:%d arg1:%d flag:%x", cbmsg->when, cbmsg->what, cbmsg->arg1, cbmsg->flag);
		cbmsg = cbmsg->next;
	}
#endif
	WinCallbackMessage *precbmsg = NULL;
	cbmsg = mMessageHeader;
	while (cbmsg != NULL) {
		if (cbmsg->when > when) {
			break;
		}
		precbmsg = cbmsg;
		cbmsg = cbmsg->next;
	}
	msg->next = cbmsg;
	if (precbmsg == NULL) {
		mMessageHeader = msg;
		signal = 1;
	} else {
		precbmsg->next = msg;
	}
#ifdef DEBUG_CALLBACK_MESSAGE
	cbmsg = mMessageHeader;
	cbmsg_xdebug("POST after header:%p header when:%lld", mMessageHeader, (mMessageHeader != NULL ? mMessageHeader->when : 0));
	while (cbmsg != NULL) {
		cbmsg_xdebug("POST after : when:%lld what:%d arg1:%d flag:%x", cbmsg->when, cbmsg->what, cbmsg->arg1, cbmsg->flag);
		cbmsg = cbmsg->next;
	}
#endif
	if (signal) {
		pthread_cond_signal(&mMsgCondition);
	}
	pthread_mutex_unlock(&mMsgMutex);
	return 0;
}

int AppMain::deleteMessage(int what) {
	cbmsg_xdebug("deleteMessage what:%d", what);
	pthread_mutex_lock(&mMsgMutex);

	WinCallbackMessage *cbmsg;
	WinCallbackMessage *precbmsg = NULL;
	WinCallbackMessage *delmsg = NULL;
#ifdef DEBUG_CALLBACK_MESSAGE
	cbmsg = mMessageHeader;
	while (cbmsg != NULL) {
		cbmsg_xdebug("Delete before: when:%lld what:%d arg1:%d flag:%x", cbmsg->when, cbmsg->what, cbmsg->arg1, cbmsg->flag);
		cbmsg = cbmsg->next;
	}
#endif
	cbmsg = mMessageHeader;
	while (cbmsg != NULL) {
		if (cbmsg->what == what) {
			if (precbmsg) {
				precbmsg->next = cbmsg->next;
			} else {
				mMessageHeader = cbmsg->next;
			}
			delmsg = cbmsg;
			cbmsg = cbmsg->next;

			if ((delmsg->flag & CALLBACK_MESSAGE_FLAG_STATIC) != 0) {
				delmsg->when = 0;
				delmsg->what = 0;
			} else {
				free(delmsg);
			}
		} else {
			precbmsg = cbmsg;
			cbmsg = cbmsg->next;
		}
	}
#ifdef DEBUG_CALLBACK_MESSAGE
	cbmsg = mMessageHeader;
	cbmsg_xdebug("Delete after header:%p header when:%lld", mMessageHeader, (mMessageHeader != NULL ? mMessageHeader->when : 0));
	while (cbmsg != NULL) {
		cbmsg_xdebug("Delete after when:%lld what:%d arg1:%d flag:%x", cbmsg->when, cbmsg->what, cbmsg->arg1, cbmsg->flag);
		cbmsg = cbmsg->next;
	}
#endif
	pthread_mutex_unlock(&mMsgMutex);
	return 0;
}

void AppMain::handleMessage(WinCallbackMessage *msg) {
	cbmsg_xdebug("when:%lld what:%d arg1:%d flag:%x", msg->when, msg->what, msg->arg1, msg->flag);
	switch (msg->what) {
		case CALLBACK_MSG_SHUTDOWN:
			ALOGD("handleMessage CALLBACK_MSG_SHUTDOWN level:%d", msg->arg1);
			shutdown(msg->arg1);
			break;
		case CALLBACK_MSG_CAMERA_PREVIEW_STATE:
			ALOGD("handleMessage CALLBACK_MSG_CAMERA_PREVIEW_STATE state:%d", msg->arg1);
			if (msg->arg1) {
				startPreview();
			} else {
				stopPreview();
			}
			break;

		case CALLBACK_MSG_REFRESH_BATTERY:
			sendUiEvent(UI_EVENT_BATTERY_CHANGE, 0);
			break;
		case CALLBACK_MSG_LONG_KEYDOWN:
			sendUiEvent(UI_EVENT_LONG_KEYDOWN, msg->arg1);
			break;
		case CALLBACK_MSG_QUICK_SHORT_KEY:
			sendUiEvent(UI_EVENT_QUICK_SHORT_KEY, msg->arg1, msg->arg2);
			break;
		case CALLBACK_MSG_FACT_AUTO_TEST:
			sendUiEvent(UI_EVENT_FACT_AUTO_TEST, msg->arg1, msg->arg2);
			break;
	}
}

void AppMain::msgLoop(void) {
	struct timespec tv;
	struct timeval realtime;
	long long timenow;
	long howlong;
	WinCallbackMessage *origmsg;
	WinCallbackMessage *precbmsg;
	WinCallbackMessage *cbmsg;
	while (true) {
		clock_gettime(CLOCK_MONOTONIC, &tv);
		timenow = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
		if (mMessageHeader == NULL || mMessageHeader->when > timenow) {
			pthread_mutex_lock(&mMsgMutex);

			gettimeofday(&realtime, NULL);
			tv.tv_sec = realtime.tv_sec;
			tv.tv_nsec = realtime.tv_usec * 1000L;

			if (mMessageHeader == NULL) {
				tv.tv_sec += 30;
				cbmsg_xdebug("timedwait 30s");
			} else {
				howlong = mMessageHeader->when - timenow; //ms
				cbmsg_xdebug("timedwait:%ld ms", howlong);
				if (howlong <= 0) {
					pthread_mutex_unlock(&mMsgMutex);
					continue;
				}
				tv.tv_sec += (howlong / 1000);
				tv.tv_nsec += (howlong % 1000) * 1000000;
				if (tv.tv_nsec >= 1000000000L) {
					tv.tv_sec += (tv.tv_nsec / 1000000000L);
					tv.tv_nsec = tv.tv_nsec % 1000000000L;
				}
			}
			cbmsg_xdebug("timedwait header:%p header when:%lld timenow:%lld %ld %ld,wait:%ld %ld", mMessageHeader,
			             (mMessageHeader != NULL ? mMessageHeader->when : 0), timenow, realtime.tv_sec, realtime.tv_usec, tv.tv_sec, tv.tv_nsec);
			pthread_cond_timedwait(&mMsgCondition, &mMsgMutex, &tv);
			pthread_mutex_unlock(&mMsgMutex);
			continue;
		}
		cbmsg_xdebug("proc start timenow:%lld", timenow);
		pthread_mutex_lock(&mMsgMutex);
		origmsg = NULL;
		precbmsg = NULL;
		cbmsg = mMessageHeader;
		while (cbmsg != NULL) {
			if (cbmsg->when > timenow) {
				break;
			}
			precbmsg = cbmsg;
			cbmsg = cbmsg->next;
		}
		if (cbmsg == mMessageHeader) {
			ALOGE("find MessageHeader none");
			pthread_mutex_unlock(&mMsgMutex);
			continue;
		}
		origmsg = mMessageHeader;
		mMessageHeader = cbmsg;
		if (precbmsg != NULL) {
			precbmsg->next = NULL;
		}
		pthread_mutex_unlock(&mMsgMutex);

		precbmsg = NULL;
		cbmsg = origmsg;
		while (cbmsg != NULL) {
			cbmsg_xdebug("proc: now:%lld when:%lld what:%d arg1:%d flag:%x", timenow, cbmsg->when, cbmsg->what, cbmsg->arg1, cbmsg->flag);
			if (cbmsg->when <= timenow) {
				if (cbmsg->what > 0) {
					handleMessage(cbmsg);
				} else {
					ALOGE("msgLoop get error data, what:%d arg1:%d arg2:%d obj:%p", cbmsg->what, cbmsg->arg1, cbmsg->arg2, cbmsg->obj);
				}
			}
			precbmsg = cbmsg;
			cbmsg = cbmsg->next;

			if ((precbmsg->flag & CALLBACK_MESSAGE_FLAG_STATIC) != 0) {
				precbmsg->when = 0;
				precbmsg->what = 0;
			} else {
				free(precbmsg);
			}
		}
		cbmsg_xdebug("proc end");
	}
}

int AppMain::notify(int message, int val) {
	int ret = 0;
	if (mShutdowning) {
		return 0;
	}

	switch (message) {

#ifdef CONFIG_HAVE_BATTERY
		case EVENT_BATTERY_ONLINE:
			sendUiEvent(UI_EVENT_BATTERY_CHARGING, 0);
			sendUiEvent(UI_EVENT_BATTERY_CHANGE, 0);
			break;
		case EVENT_BATTERY_OFFLINE:
			onResumePower(message, "EVENT_BATTERY_OFFLINE");
			sendUiEvent(UI_EVENT_BATTERY_CHARGING, 1);
			break;
		case EVENT_BATTERY_CHANGE: {
			int capacity = device_battery_capacity();
			db_msg("EVENT_BATTERY_CHANGE BatteryCapacity:%d", capacity);
			sendUiEvent(UI_EVENT_BATTERY_CHANGE, capacity);
			if (capacity <= BATTERY_LOW_LEVEL && !device_has_ext_power()) {
				db_msg("BatteryCapacity:%d <= %d ,will shutdown...", capacity, BATTERY_LOW_LEVEL);
				onLosePower(EVENT_BATTERY_LOW, "EVENT_BATTERY_LOW");
			}
		}
			break;
#endif
	}
	return ret;
}

int AppMain::sendUiEvent(int event, int param, long param2) {
#ifdef  CONFIG_HAVE_GUI
	if (sGuiCallBack != NULL) {
		return sGuiCallBack->onUiEvent(event, param, param2);
	}
#endif
	return 0;
}

int AppMain::uiCallBack(int event, int param) {
	int netcmd = UI_EVENT_TO_NET_CMD(event);
	if (netcmd > 0) {
		return uiControl(netcmd, param);
	} else {
		db_error("not handle cmd:%d param:%d", event, param);
	}
	return 0;
}

int AppMain::uiControl(int cmd, int param) {
	ctl_common_req req;
	req.request = param;
	return sessionControl(-1, cmd, (char *) &req, sizeof(req), 2);
}

int AppMain::sessionControl(int sid, int cmd, const char *buf, int len, int ctl_from) {

	int ret = sessionControl0(sid, cmd, buf, len, ctl_from);
	if (ret != 0) {
		// db_error("error on cmd:0x%x ret:%d", cmd, ret);
	} else {
		if (haveGuiCallBack() && cmd > CMD_MIN_VAL && cmd < CMD_MAX_VAL) {
			int param_in;
			if (len == sizeof(ctl_common_req)) {
				param_in = ((ctl_common_req *) buf)->request;
			} else {
				param_in = 0;
			}
			sendUiEvent(cmd, param_in, ctl_from);
		}
	}
	return ret;
}

#define RESP_RESULT_TO_APP(resp, r)  do{}while(0)

int AppMain::sessionControl0(int sid, int cmd, const char *buf, int len, int ctl_from) {
	if ((mShutdowning && ctl_from == 0)) {
		return -1;
	}
	int ret = 0;
	int param_in = 0;
	if (len == sizeof(ctl_common_req)) {
		param_in = ((ctl_common_req *) buf)->request;
	}
	db_msg("from:%d sid:0x%X cmd:0x%X len:%d param:%d", ctl_from, sid, cmd, len, param_in);
	switch (cmd) {

		case CTRL_CMD_FACTORY_RESET: {
			db_msg("CTRL_CMD_FACTORY_RESET ");
			RESP_RESULT_TO_APP(CTRL_CMD_FACTORY_RESET_RESP, 0);
			doFactoryReset();
		}
			break;

		case CTRL_CMD_REBOOT_DEVICE: {
			db_msg("CTRL_CMD_REBOOT_DEVICE requst");
			RESP_RESULT_TO_APP(CTRL_CMD_REBOOT_DEVICE_RESP, 0);
			mForceReboot = 1;
			break;
		}

		case CTRL_CMD_SHUTDOWNT_DEVICE: { 
			db_msg("CTRL_CMD_SHUTDOWNT_DEVICE requst,time:%d", param_in);
			RESP_RESULT_TO_APP(CTRL_CMD_SHUTDOWNT_DEVICE_RESP, 0);
			mShutdownFutureTime = getClockTime();
			break;
		}

		case CTRL_CMD_SCREEN_SLEEP:
		{
			db_msg("CTRL_CMD_SCREEN_SLEEP param_in:%d", param_in);
			settings_save(SETTING_KEY_SCREEN_SAVER, param_in);
			RESP_RESULT_TO_APP(CTRL_CMD_SCREEN_SLEEP_RESP, ret);
		}
			break;

		case CTRL_CMD_SET_AUTO_SHOTDOWN_TIME:
		{
			db_msg("CTRL_CMD_SET_AUTO_SHOTDOWN_TIME param_in:%d", param_in);
			if (param_in >= 0) {
				settings_save(SETTING_KEY_AUTO_SHUTDOWN_TIME, param_in);
			} else {
				ret = -1;
			}
			RESP_RESULT_TO_APP(CTRL_CMD_SET_AUTO_SHOTDOWN_TIME_RESP, ret);
		}
			break;

		case CTRL_CMD_SET_LANGUAGE:
		{
			db_msg("CTRL_CMD_SET_LANGUAGE param_in:%d", param_in);
			if (IS_VALID_LANG_ID(param_in)) {
				settings_save(SETTING_KEY_LANGUAGE, param_in);
				ret = 0;
			} else {
				ret = -1;
			}
			RESP_RESULT_TO_APP(CTRL_CMD_SET_LANGUAGE_RESP, ret);
		}
			break;

        case CTRL_CMD_SET_BRIGHTNESS:
        {
            db_msg("CTRL_CMD_SET_BRIGHTNESS param_in:%d", param_in);
            settings_save(SETTING_KEY_BRIGHTNESS, param_in);
            RESP_RESULT_TO_APP(CTRL_CMD_SET_BRIGHTNESS_RESP, ret);
        }
            break;

        case CTRL_CMD_SET_RAND_PIN_KEYPAD:
        {
            db_msg("CTRL_CMD_SET_RAND_PIN_KEYPAD param_in:%d", param_in);
            settings_save(SETTING_KEY_RAND_PIN_KEYPAD, param_in);
            RESP_RESULT_TO_APP(CTRL_CMD_SET_RAND_PIN_KEYPAD_RESP, ret);
        }
            break;
        case CTRL_CMD_SET_BTC_MULTI_ADDRESS:
        {
            settings_save(SETTING_KEY_BTC_MULTI_ADDRESS, param_in);
            RESP_RESULT_TO_APP(CTRL_CMD_SET_BTC_MULTI_ADDRESS_RESP, ret);
        }
            break;
		default:
			ret = -1;
			break;
	}
	return ret;
}

int AppMain::otaUpdate() {
	mShutdowning = true;
	return device_start_ota();
}

void AppMain::doFactoryReset() {
	db_msg("factory reset");
	unlink(WALLET_SETTINGS_FILE);
	settings_set_default();
	device_clean_all_info();
	if (gSettings->mLang != CONFIG_DEFAULT_LANG) {
		settings_save(SETTING_KEY_LANGUAGE, settings_get_lang());
	}
	sync();
}

static int initMain() {
	return 0;
}

#ifdef DEBUG_MEMORY
static void *showTopThread(void *param) {
	db_msg("showTopThread start");
#define SHOW_TOP_FILE_PATH DATA_POINT"/top.txt"
	remove(SHOW_TOP_FILE_PATH);
	int trytime = 0;
	char tmpbuf[256];
	FILE *fp;
	while (trytime < 120) {
		trytime++;
		snprintf(tmpbuf, sizeof(tmpbuf), "try:%d  uptime:%ld timenow:%ld\n", trytime, getMsClockTime(), time(NULL));
		db_msg("showTopThread:%s",tmpbuf);
		fp = fopen(SHOW_TOP_FILE_PATH,"a+");
		if(fp!=NULL){
			fwrite(tmpbuf,strlen(tmpbuf),1,fp);
			fflush(fp);
			fclose(fp);
			fp = NULL;
		}
		snprintf(tmpbuf, sizeof(tmpbuf), "cat /proc/meminfo  >> %s", SHOW_TOP_FILE_PATH);
		db_msg("showTopThread:%s",tmpbuf);
		system(tmpbuf);
		usleep(500000);
	}
	return 0;
}
#endif

static int wallet_init_check(AppMain *appMain, int init_ret) {
	sec_state_info info;
	if (init_ret == 0) {
		if (sapi_get_state_info(&info) != 0) {
			db_serr("get state info false");
			init_ret = -1;
		}
	}
	if (init_ret == 0) {
		if (gHaveSeed) {
			if (info.seed_state != 1) {
				db_error("seed diff sapi:%d local:%d", info.seed_state, gHaveSeed);
				settings_set_have_seed(0);
				appMain->sendUiEvent(UI_EVENT_SEED_CHANGED, 0);
			} else if (info.account_id) {
				uint64_t id = device_read_seed_account();
				if (!id) {
					db_serr("auto save se account_id:%llx", info.account_id);
					device_save_seed_account(info.account_id);
				} else if (id != info.account_id) {
					db_serr("invalid nvm account_id:%llx != se account_id:%llx", id, info.account_id);
					settings_set_have_seed(0);
					appMain->sendUiEvent(UI_EVENT_SEED_CHANGED, 0);
				}
			}
		}
		appMain->sendUiEvent(UI_EVENT_SECHIP_INITED, 0);
	} else {
		if (init_ret < 0 && gHaveSeed) {
			db_error("set have no seed");
			settings_set_have_seed(0);
		}
		if (init_ret == 1) {
			if (GLobal_IN_OTAOK_Win) {
				db_msg("use OTAOK win");
			} else {
				if (gHaveSeed) {
					db_error("set have no seed,wm init ret:%d", init_ret);
					settings_set_have_seed(0);
				}
				appMain->sendUiEvent(UI_EVENT_SEED_CHANGED, 0);
			}
		} else {
			appMain->sendUiEvent(UI_EVENT_SEED_CHANGED, -1);
		}
		appMain->sendUiEvent(UI_EVENT_SECHIP_INITED, init_ret);
	}
	return 0;
}

static void *wallet_main2_thread(void *param) {
	AppMain *appMain = (AppMain *) param;
	sched_yield();
	ALOGD("--------- main2 start --------");
	DEBUG_PERFORMANCE();
	int ret = device_check_host();
	if (ret > 0) {
		ALOGE("@@@ start");
		return 0;
	}
	sapi_init0();
	proto_init_env(0);
	//init secure api
	int device_inited = device_is_inited();

	ret = wallet_init(0);
	if (ret < 0 && gHaveSeed) {
		db_error("wallet_init ret:%d but have seed,retry", ret);
		usleep(300 * 1000);
		ret = wallet_init(1);
	}
	db_msg("device_inited:%d wallet_init ret:%d", device_inited, ret);
	if (device_inited) {
		if (device_check_sechip() != 0) {
			db_serr("@@@ chost");
			return 0;
		}
		wallet_init_check(appMain, ret);
	} else {
		if (gHaveSeed) {
			settings_set_have_seed(0);
		}
	}
	if (appMain->init() != 0) {
		ALOGE("init false");
		return 0;
	}
	appMain->initStage2();

	Global_App_Inited = 1;
	ALOGD("--------- msgloop start --------");
	appMain->msgLoop();
	ALOGD("--------- exit --------");
	return 0;
}

#ifdef CONFIG_HAVE_GUI

int wallet_main_entry(int argc, const char *argv[], GuiCallBack *callback) {
#else

	int main(int argc, const char *argv[]) {
#endif
	ALOGD("--------- running version:%s timenow:%ld --------", PRODUCT_VERSION, time(NULL));
	DEBUG_PERFORMANCE();
	if (initMain() != 0) {
		ALOGE("init false");
		return 0;
	}
	DEBUG_PERFORMANCE();
	int ret = device_init(NULL);
	DEBUG_PERFORMANCE();
	if (ret != 0) {
		ALOGE("init false");
		return 0;
	}
	DEBUG_PERFORMANCE();
#ifdef DEBUG_MEMORY
	pthread_t toppid = 0;
	pthread_create(&toppid, NULL, showTopThread, NULL);
	sleep(20);
#endif
	DEBUG_PERFORMANCE();
	AppMain *appMain = new AppMain();
	if (settings_init() != 0) {
		ALOGE("init false");
		return 0;
	}
	wallet_init0();
	DEBUG_PERFORMANCE();
#ifdef CONFIG_HAVE_GUI
	AppMain::setGuiCallBack(callback);
#endif
	DEBUG_PERFORMANCE();
	if (appMain->init0() != 0) {
		ALOGE("init false");
		return 0;
	}

	DEBUG_PERFORMANCE();
#ifdef CONFIG_HAVE_GUI
	if (callback) {
		ret = callback->onUiEvent(UI_EVENT_APPMAIN_INITED, 0, 0);
		if (ret != 0) {
			ALOGE("gui on init false");
			return 0;
		}
	}
	pthread_t main2_threadid = 0;
	pthread_create(&main2_threadid, NULL, wallet_main2_thread, appMain);
	return 1;
#else
	DEBUG_PERFORMANCE();
	wallet_main2_thread(appMain);
	return 0;
#endif
}
