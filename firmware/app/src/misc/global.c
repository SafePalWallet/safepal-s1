#include <string.h>
#include <unistd.h>
#include "global.h"
#include "platform.h"

#ifdef BUILD_FOR_DEV
static int app_run_mode = 0;
#endif

int Global_LosePower = 0;
unsigned int Global_BootMsClock = 0;
int Global_FactoryTesting = 0;
int Global_USB_Change = 0;
int Global_USB_State = 0;//screenOn when usbState change
int Global_App_Inited = 0;
int Global_Have_New_DBTX = 0;
int Global_Have_New_DBTX2 = 0;
int Global_Have_New_DBCoin = 0;
unsigned int Global_MainHwnd = 0;
void *Global_Font18 = 0;
void *Global_Font20 = 0;
int Global_Guide_Index = 0;
int Global_Guide_KeyL = 0;
int Global_Guide_KeyR = 0;
int Global_Guide_KeyU = 0;
int Global_Guide_KeyD = 0;
int Global_Guide_abort = 0;
int Global_Active_step = 0;
int Global_Temp_Screen_Time = 0;
#ifdef BUILD_FOR_DEV
int GLobal_PIN_Passed = 1;
#else
int GLobal_PIN_Passed = 0;
#endif
int GLobal_IN_OTAOK_Win = 0;
int Global_Key_Shield = 0;
long long Global_Key_Random_Source = 0;
int gProcessing = 0;
long Global_HW_Break_State = 0;
int Global_QR_Proc_Result = 0;
int Global_Device_Account_Index = 0;
uint64_t gSeedAccountId = 0;
int gHaveSeed = 0;
int GLobal_CoinsWin_EditMode = 0;
int GLobal_Is_Coin_EVM_Category = 0;
int GLobal_Is_Se_Processing = 0;
int GLobal_Is_Se_Upgrading = 0;


#ifdef BUILD_FOR_DEV

void set_app_run_mode(int m) {
	app_run_mode = m;
}

#endif

int app_run_in_dev_mode() {
#ifdef BUILD_FOR_DEV
	return app_run_mode;
#else
	return 0;
#endif
}

const char *get_system_res_point() {
#ifdef BUILD_FOR_DEV
	return app_run_mode ? "/mnt/extsd/res" : "/system/res";
#else
	return "/system/res";
#endif
}

int set_temp_screen_time(int t) {
	int i = Global_Temp_Screen_Time;
	Global_Temp_Screen_Time = t;
	return i;
}
