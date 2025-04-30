#ifndef _WINDOWS_H
#define _WINDOWS_H

#include "common.h"
#include "EventManager.h"
#include "device_manager.h"
#include "GuiInterface.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_CALLBACK_MESSAGE_NUM 5
#define CALLBACK_MESSAGE_FLAG_STATIC 1

struct WinCallbackMessage {
	long long when;
	int what;
	int arg1;
	int arg2;
	void *obj;
	struct WinCallbackMessage *next;
	char flag;
};

class CameraManager;

class AppMain : public EventListener {

public:
	static AppMain *getInstance();

	static bool haveGuiCallBack();

	static GuiCallBack *getGuiCallBack();

	static void setGuiCallBack(GuiCallBack *callBack);

	AppMain();

	~AppMain();

	int mForceReboot;
	int mShutdowLevel;

	int init0();

	int init();

	int initStage2();

	void shutdownLoop();

	int destoryDevice();

	int postMessageDelay(int delay, int what, int arg1 = 0, int arg2 = 0, void *obj = NULL);

	int postMessage(int what, int arg1 = 0, int arg2 = 0, void *obj = NULL);

	int deleteMessage(int what);

	void msgLoop();

	int notify(int message, int data);

	int sendUiEvent(int event, int param, long param2 = 0);

	int uiCallBack(int event, int param);

	int uiControl(int cmd, int param);

	int sessionControl(int sid, int cmd, const char *buf, int len, int ctl_from);

	int sessionControl0(int sid, int cmd, const char *buf, int len, int ctl_from);

	bool isPreviewing();

	int startPreview();

	int stopPreview();

	int qrChunk(int datalen, char *data);

	int camExist();

	bool isShutdownIng();

	bool isScreenOn();

	bool screenOn(bool force = false);

	bool screenOff();

	int otaUpdate();

	void doFactoryReset();

	void doPowerOffDelay();

private:
	static AppMain *sAppMain;
	static GuiCallBack *sGuiCallBack;

	CameraManager *mCameraManager;
	EventManager *mEventManager;
	WinCallbackMessage *mMessageHeader;

	bool mShutdowning;

	long mShutdownFutureTime;
	long mShutdownCountdownTime;

	WinCallbackMessage mCallbackMessageBuff[MAX_CALLBACK_MESSAGE_NUM];
	pthread_cond_t mMsgCondition;
	pthread_mutex_t mMsgMutex;

	static int initSystemEnv();

	void handleMessage(WinCallbackMessage *msg);

	bool canCancelShutdown();

	void onLosePower(int event, const char *msg, bool playsound = true);

	void onResumePower(int event, const char *msg);

	void powerOffAsync();

	void powerOffDelay();

	void shutdown(int level);

};

#endif
