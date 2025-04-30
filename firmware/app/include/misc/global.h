
#ifndef WALLET_GLOBAL_H
#define WALLET_GLOBAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
extern int Global_LosePower;
extern unsigned int Global_BootMsClock;
extern int Global_FactoryTesting;
extern int Global_USB_Change;
extern int Global_USB_State;
extern int Global_App_Inited;
extern int Global_Have_New_DBTX;
extern int Global_Have_New_DBTX2;
extern int Global_Have_New_DBCoin;
extern unsigned int Global_MainHwnd;
extern void *Global_Font18;
extern void *Global_Font20;
extern int Global_Guide_Index;
extern int Global_Guide_KeyL;
extern int Global_Guide_KeyR;
extern int Global_Guide_KeyU;
extern int Global_Guide_KeyD;
extern int Global_Guide_abort;
extern int Global_Active_step;
extern int Global_Temp_Screen_Time;
extern int GLobal_PIN_Passed;
extern int GLobal_IN_OTAOK_Win;
extern int Global_Key_Shield;
extern int gProcessing;
extern long Global_HW_Break_State;
extern int Global_QR_Proc_Result;
extern long long Global_Key_Random_Source;
extern int Global_Device_Account_Index;
extern uint64_t gSeedAccountId;
extern int gHaveSeed;
extern int GLobal_CoinsWin_EditMode;
extern int GLobal_Is_Coin_EVM_Category;
extern int GLobal_Is_Se_Processing;
extern int GLobal_Is_Se_Upgrading;

#ifdef BUILD_FOR_DEV

void set_app_run_mode(int m);

#endif

int app_run_in_dev_mode();

const char *get_system_res_point();

int set_temp_screen_time(int t);

#ifdef __cplusplus
}
#endif

#endif
