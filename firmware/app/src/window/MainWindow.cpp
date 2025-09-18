#define LOG_TAG "MainWindow"

//#include <sys/ptrace.h>
#include <signal.h>
#include <qr_pack.h>
#include "storage_manager.h"
#include "widgets.h"
#include "common.h"
#include "win.h"
#include "GuiMain.h"
#include "MainPanel.h"
#include "TxShowWin.h"
#include "TxMoreWin.h"
#include "QrProcWin.h"
#include "GuideWin.h"
#include "SettingWin.h"
#include "CoinsWin.h"
#include "CoinDetailWin.h"
#include "SignHistoryWin.h"
#include "device_manager.h"
#include "CameraManager.h"
#include "Dialog.h"
#include "wallet_proto.h"
#include "settings.h"
#include "secure_api.h"
#include "device.h"
#include "AboutWin.h"
#include "OTAOKWin.h"
#include "Passphrase.h"
#include "VerifyWin.h"
#include "loading_win.h"
#include "show_bmp.h"
#include "wallet_util_hw.h"
#include "TxVerifyCodeWin.h"
#include "TxRawDataWin.h"

#define WINID_CAN_QUICK_SCAN(w) ((w)==WINDOWID_MAINPANEL||(w)==WINDOWID_SETTING||(w)==WINDOWID_COINS_MANAGER||(w)==WINDOWID_COIN_DETAIL||(w)==WINDOWID_SIGN_HISTORY)

//#define DEBUG_IN_CMD_LINE
extern int wallet_main_entry(int argc, const char *argv[], GuiCallBack *callback);

static GuiMain *mStaticGuiMain = NULL;
static AppMain *mAppMain = NULL;

GuiMain *GuiMain::getInstance() {
	return mStaticGuiMain;
}

static int KeyMsgHook(void *context, HWND dst_wnd, PROC_MSG_TYPE msg, WPARAM wparam, LPARAM lparam) {
	return ((GuiMain *) context)->hookKeyMsg(dst_wnd, msg, wparam, lparam);
}

static PROC_RET WalletWinProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	GuiMain *main = (GuiMain *) GetWindowAdditionalData(hWnd);
	return main ? main->mainWindowProc(hWnd, message, wParam, lParam) : DefaultWindowProc(hWnd, message, wParam, lParam);
}

GuiMain::GuiMain() {

	for (int i = 0; i < WINDOWID_MAXID; i++) {
		mWindows[i] = NULL;
	}

	mCurWindowID = WINDOWID_MAINPANEL;

	mFinishConfig = false;

	mHookKey = 0;
	mHookHwnd = HWND_INVALID;
}

CommonWindow *GuiMain::getWindowObject(int windowID, int create) {
	if (windowID <= 0 || windowID >= WINDOWID_MAXID) {
		db_error("invalid win:%d", windowID);
		return NULL;
	}
	if (mWindows[windowID] != NULL) {
		return mWindows[windowID];
	}
	if (!create) {
		return NULL;
	}
	CommonWindow *win = NULL;
	switch (windowID) {
		case WINDOWID_MAINPANEL:
			win = new MainPanel();
			break;
		case WINDOWID_SCAN:
			win = new ScanQrCode();
			break;
		case WINDOWID_QRPROC:
			win = new QrProcWin();
			break;
		case WINDOWID_TXSHOW:
			win = new TxShowWin();
			break;
		case WINDOWID_TXMORE:
			win = new TxMoreWin();
			break;
		case WINDOWID_GUIDE:
			win = new GuideWin();
			break;
		case WINDOWID_SETTING:
			win = new SettingWin();
			break;
		case WINDOWID_COINS_MANAGER:
			win = new CoinsWin();
			break;
		case WINDOWID_COIN_DETAIL:
			win = new CoinDetailWin();
			break;
		case WINDOWID_SIGN_HISTORY:
			win = new SignHistoryWin();
			break;
		case WINDOWID_FACTORY:
			win = new FactoryWin();
			break;
		case WINDOWID_ABOUT:
			win = new AboutWin();
			break;
		case WINDOWID_OTAOK:
			win = new OTAOKWin();
			break;
		case WINDOWID_PASSPHRASE:
			win = new Passphrase();
			break;
		case WINDOWID_VERIFY:
			win = new VerifyWin();
			break;
		case WINDOWID_TX_VERIFY_CODE:
			win = new TxVerifyCodeWin();
			break;
		case WINDOWID_TX_RAW_DATA:
			win = new TxRawDataWin();
			break;
		default:
			db_error("unkown win:%d", windowID);
	}
	if (win) {
		mWindows[windowID] = win;
		win->createMainWindow(windowID, mHwnd);
	}
	return win;
}

int GuiMain::createWindow(int windowID) {
	if (windowID <= 0 || windowID >= WINDOWID_MAXID) {
		db_error("invalid win:%d", windowID);
		return -1;
	}
	HWND hwnd = win_get_hwnd(windowID);
	if (IS_VALID_HWND(hwnd)) {
		return 0;
	}

	db_debug("create win:%d start", windowID);
	CommonWindow *win = getWindowObject(windowID, 1);
	if (!win) {
		return -1;
	}
	hwnd = win->getHwnd();
	if (!IS_VALID_HWND(hwnd)) {
		db_error("create id:%d win:%p failed", windowID, win);
		return -1;
	}
	win_set_hwnd(windowID, hwnd);
	db_debug("create win:%d end", windowID);
	return 0;
}

int GuiMain::destoryWindow(int windowID) {
	db_msg("curr win:%d delete win:%d ", mCurWindowID, windowID);
	if (mCurWindowID == windowID) {
		return -1;
	}
	CommonWindow *win = getWindowObject(windowID);
	if (win && !win->isHold()) {
		win_set_hwnd(windowID, HWND_INVALID);
		mWindows[windowID] = NULL;
		delete (win);
	}
	return 0;
}

int GuiMain::sendMessage(int windowID, int iMsg, WPARAM wParam, LPARAM lParam) {
	if (createWindow(windowID) == 0) {
		return win_send_message(windowID, iMsg, wParam, lParam);
	} else {
		db_error("invalid win:%d", windowID);
		return -1;
	}
}

GuiMain::~GuiMain() {

	for (int i = 1; i < WINDOWID_MAXID; i++) {
		if (mWindows[i] != NULL) {
			delete mWindows[i];
			mWindows[i] = NULL;
		}
	}

	if (mFinishConfig) {
		UnRegisterWltWindows();
	}
}

int GuiMain::onInitAppMain() {
	db_msg("main inited");
	mAppMain = AppMain::getInstance();
	if (mAppMain == NULL) {
		db_error("get main false");
		return -1;
	}

	if (createMainWindows() != 0) {
		db_error("onInitAppMain false");
		return -2;
	}
	return 0;
}

int GuiMain::onLosePower() {
	return 0;
}

int GuiMain::onUiEvent(int event, int param, long param2) {
	int ret;
	int netcmd = 0;
	if (event > CMD_MIN_VAL && event < CMD_MAX_VAL) {
		netcmd = event;
		event = CMD_TO_UI_EVENT(event);
		if (event <= 0) {
			//db_error("Not Maped Event:0x%x param:%d", netcmd, param);
			return -1;
		}
	}
	db_msg("Event:0x%x p0:%d p2:%ld cmd:0x%x", event, param, param2, netcmd);

	switch (event) {
		case UI_EVENT_APPMAIN_INITED:
			return onInitAppMain();
		case UI_EVENT_SHUTDOWN:
			checkShutdown();
			return 0;
		case UI_EVENT_LOSE_POWER:
			return onLosePower();
		case UI_EVENT_BATTERY_CHANGE:
			if (mCurWindowID == WINDOWID_FACTORY) {
				win_send_notify(WINDOWID_FACTORY, MSG_BATTERY_CHANGED, param, param2);
			} else {
				win_send_notify(WINDOWID_MAINPANEL, MSG_BATTERY_CHANGED, param, param2);
			}
			break;
		case UI_EVENT_BATTERY_CHARGING:
			if (mCurWindowID == WINDOWID_FACTORY) {
				win_send_notify(WINDOWID_FACTORY, MSG_BATTERY_CHARGING, param, param2);
			} else {
				win_send_notify(WINDOWID_MAINPANEL, MSG_BATTERY_CHARGING, param, param2);
			}
			break;
		case UI_EVENT_QR_RESULT:
			if (mCurWindowID == WINDOWID_SCAN) {
				if (onQrResult(param, (qr_packet *) param2) != 0) {
					Global_QR_Proc_Result = -1;
					win_send_notify(WINDOWID_SCAN, MSG_DISABLE_QRSCAN_WIN, -1, 0);
				}
			}
			break;
		case UI_EVENT_QR_CHUNK:
			if (mCurWindowID == WINDOWID_SCAN) {
				win_send_notify(WINDOWID_SCAN, MSG_QR_CHUNK, param, param2);
			} else {
				db_error("error winid:%d", mCurWindowID);
			}
			break;
		case UI_EVENT_QR_ERROR:
			if (mCurWindowID == WINDOWID_SCAN) {
				win_send_notify(WINDOWID_SCAN, MSG_QR_ERROR, param, param2);
			} else {
				db_error("error winid:%d", mCurWindowID);
			}
			break;
		case UI_EVENT_SEED_CHANGED:
			if (GLobal_IN_OTAOK_Win) {
				db_msg("OTAOK,skip seed change:%d", param);
				break;
			}
			if (param <= 0) {
				if (mCurWindowID != WINDOWID_GUIDE) {
					xchangeWindow(WINDOWID_GUIDE);
				}
			}
			break;
		case UI_EVENT_SECHIP_INITED:
			win_send_notify(WINDOWID_MAINPANEL, MSG_SECHIP_INITED, param, param2);
			break;
		case UI_EVENT_LONG_KEYDOWN:
			db_msg("longkey:%d current hook key:%d", param, mHookKey);
			if (mHookKey == param) { //reset
				mHookKey = 0;
				mHookHwnd = HWND_INVALID;
			}
			if (param == INPUT_KEY_POWER) { //shutdown
				mAppMain->postMessage(CALLBACK_MSG_SHUTDOWN, 11);
			}
			break;
		case UI_EVENT_QUICK_SHORT_KEY:
			db_verbose("quckkey:%d count:%ld", param, param2);
			if (param == INPUT_KEY_POWER && mAppMain->isScreenOn()) { //screen off
				if (param2 > 1) {
					if (mCurWindowID == WINDOWID_FACTORY) {
						win_send_notify(WINDOWID_FACTORY, MSG_QUICK_SHORT_KEY, param, param2);
						break;
					}
					if (WINID_CAN_QUICK_SCAN(mCurWindowID) && mHwnd == GetActiveWindow()) {
						xchangeWindow(WINDOWID_SCAN);
					}
				} else {
					if (mCurWindowID == WINDOWID_FACTORY) {
						win_send_notify(WINDOWID_FACTORY, MSG_QUICK_SHORT_KEY, param, 1);
					}
					if (!Global_FactoryTesting) {
						mAppMain->screenOff();
						Global_USB_State = device_usb_online();//screenOn when usbState change
					}
				}
			}
			break;
		case UI_EVENT_FACT_AUTO_TEST:
			if (mCurWindowID == WINDOWID_FACTORY) {
				win_send_notify(WINDOWID_FACTORY, MSG_FACT_AUTO_TEST, param, 0);
			}
			break;
		case UI_EVENT_SCREEN_STATE:
			if (param == 0) { //screen off
				if (mCurWindowID == WINDOWID_SCAN) {
					db_msg("auto disable scan qr win");
					win_send_notify(WINDOWID_SCAN, MSG_DISABLE_QRSCAN_WIN, -4, 0);
				}
			}
			break;
		default:
			break;
	}
	//setting cmd from net
	if (netcmd && param2 == 0) {
		if (event == UI_EVENT_SET_LANG) {
			ret = res_updateLangAndFont(param);
			db_msg("chang lang to:%d ret:%d", param, ret);
		}
		if (mCurWindowID == WINDOWID_SETTING) {
			win_send_message(WINDOWID_SETTING, MLM_FLUSH_MENU_LIST, 0, 0);
		}
	}
	return 0;
}

int GuiMain::createMainWindows(void) {
	db_msg("create main window start");
	if (res_initStage1() < 0) {
		db_error("initStage1 failed");
		return -1;
	}

	if (RegisterWltWindows() < 0) {
		db_error("register windows failed");
		return -1;
	}

	RegisterKeyMsgHook((void *) this, KeyMsgHook);

	MAINWINCREATE CreateInfo;
	WIN_RECT rect;
	CreateInfo.dwStyle = WS_VISIBLE;
	CreateInfo.dwExStyle = WS_EX_AUTOSECONDARYDC;
	CreateInfo.spCaption = "";
	CreateInfo.hMenu = 0;
	CreateInfo.hCursor = 0;
	CreateInfo.hIcon = 0;
	CreateInfo.MainWindowProc = WalletWinProc;

	res_getPos(MK_system, &rect);
	CreateInfo.lx = rect.x;
	CreateInfo.ty = rect.y;
	CreateInfo.rx = rect.w;
	CreateInfo.by = rect.h;

	db_msg("main win x:%d y:%d w:%d h:%d", rect.x, rect.y, rect.w, rect.h);

	CreateInfo.iBkColor = RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00); /* the color key */
	CreateInfo.dwAddData = (DWORD) this;
	CreateInfo.hHosting = HWND_DESKTOP;

	mHwnd = CreateMainWindow(&CreateInfo);
	if (mHwnd == HWND_INVALID) {
		db_error("create Mainwindow failed");
		return -1;
	}
	Global_MainHwnd = mHwnd;
	ShowWindow(mHwnd, SW_SHOWNORMAL);
	win_set_hwnd(WINDOWID_MAIN, mHwnd);

	int winid;
	if (!device_is_inited()) {
		db_msg("device not inited");
		winid = WINDOWID_FACTORY;
	} else if (gSettings->mOtaPreVersion > 0 && (gSettings->mOtaPreVersion <= DEVICE_APP_INT_VERSION)) {
		db_msg("after OTA");
		winid = WINDOWID_OTAOK;
		GLobal_IN_OTAOK_Win = 1;
	} else {
		winid = (gHaveSeed == 1) ? WINDOWID_MAINPANEL : WINDOWID_GUIDE;
	}
	db_msg("winid:%d seed:%d", winid, gHaveSeed);
#ifdef DEV_DEBUG_IGNORE_SEED
	winid = WINDOWID_MAINPANEL;
	db_msg("IGNORE_SEED force set winid:%d", winid);
#endif
	mCurWindowID = winid;
	if (createWindow(winid) == 0) {
		win_show_window(winid);
	}
	db_msg("create main window end");
	return 0;
}

void GuiMain::initStage2() {
	db_msg("initStage2 start");
	if (getClockTime() > 30) {
		db_msg("set screenOn when not start init");
		mAppMain->screenOn(true);
	}

	mFinishConfig = true;
	db_msg("initStage2 end");
}

static bool inline isKeyEvent(int msg) {
	return msg == MSG_KEYDOWN || msg == MSG_KEYUP;
}

int GuiMain::hookKeyMsg(HWND dst_wnd, int msg, WPARAM wparam, LPARAM lparam) {
	static bool timer_seted = false;
	static long long quick_key_time = 0;
	static int quick_key_click = 0;
	int keycode = wparam;
	if (msg == MSG_KEYDOWN) {
		Global_Key_Random_Source += keycode * getMsClockTime();
		if (Global_Guide_Index == 1) {
			switch (keycode) {
				case INPUT_KEY_LEFT:
					Global_Guide_KeyL++;
					break;
				case INPUT_KEY_RIGHT:
					Global_Guide_KeyR++;
					break;
				case INPUT_KEY_UP:
					Global_Guide_KeyU++;
					break;
				case INPUT_KEY_DOWN:
					Global_Guide_KeyD++;
					break;
				case INPUT_KEY_OK:
					Global_Guide_KeyL = 0;
					Global_Guide_KeyR = 0;
					Global_Guide_KeyU = 0;
					Global_Guide_KeyD = 0;
					break;
				default:
					Global_Guide_KeyL = 0;
					Global_Guide_KeyR = 0;
			}
		}
	}
	if (loading_win_processing || Global_Key_Shield) {
		db_msg("loading:%d key_shield:%d msg:0x%x %s key:%d skip...", loading_win_processing, Global_Key_Shield, msg, Message2Str(msg), keycode);
		if (timer_seted) {
			timer_seted = false;
			mAppMain->deleteMessage(CALLBACK_MSG_LONG_KEYDOWN);
		}
		return HOOK_STOP;
	}

	if (isShutdownIng()) {
		db_msg("shutdowning");
		return HOOK_STOP;
	}
	db_verbose("msg:0x%x %s key:%d lparam:%ld winid:%d dst_wnd:%d", msg, Message2Str(msg), keycode, lparam, mCurWindowID, dst_wnd);
	long long clocknow;
	bool isScreenOff = !mAppMain->isScreenOn();
	if (isKeyEvent(msg)) {
		if (msg == MSG_KEYDOWN) {
			device_set_last_input_time(getClockTime());
			if (isScreenOff) {
				db_verbose("STOP ScreenOff msg:0x%x key:%d lparam:%ld winid:%d", msg, keycode, lparam, mCurWindowID);
				mAppMain->screenOn();
				mHookKey = 0;
				mHookHwnd = HWND_INVALID;
				if (keycode == INPUT_KEY_POWER) {
					quick_key_time = getMsClockTime();
				}
				if (mCurWindowID == WINDOWID_FACTORY) {
					win_send_notify(WINDOWID_FACTORY, MSG_QUICK_SHORT_KEY, INPUT_KEY_POWER, 1);
				}
				return HOOK_STOP;
			}
			mHookKey = keycode;
			mHookHwnd = dst_wnd;

			if (keycode == INPUT_KEY_POWER) {
				if (!timer_seted) {
					timer_seted = true;
					mAppMain->postMessageDelay(2000, CALLBACK_MSG_LONG_KEYDOWN, keycode);
				}
				if (!is_powerkey_as_esc()) { //screen off when key up
					db_verbose("STOP power key not need");
					clocknow = getMsClockTime();
					if (quick_key_time + 300 < clocknow) {
						quick_key_click = 0;
					}
					quick_key_time = clocknow;
					mAppMain->deleteMessage(CALLBACK_MSG_QUICK_SHORT_KEY);
					return HOOK_STOP;
				}
			}
			quick_key_time = 0;
		} else if (msg == MSG_KEYUP) {
			if (timer_seted) {
				timer_seted = false;
				mAppMain->deleteMessage(CALLBACK_MSG_LONG_KEYDOWN);
			}
			if (dst_wnd != mHookHwnd || keycode != mHookKey) {
				db_verbose("STOP last wnd:%d key:%d current wnd:%d key:%d", mHookHwnd, mHookKey, dst_wnd, keycode);
				return HOOK_STOP;
			}
			mHookKey = 0;
			mHookHwnd = HWND_INVALID;

			if (keycode == INPUT_KEY_POWER) {
				if (!is_powerkey_as_esc()) {
					mAppMain->deleteMessage(CALLBACK_MSG_QUICK_SHORT_KEY);
					clocknow = getMsClockTime();
					db_verbose("clocknow:%lld quick_key_time:%lld quick_key_click:%d", clocknow, quick_key_time, quick_key_click);
					if (quick_key_time + 300 < clocknow) {
						quick_key_click = 0;
					} else {
						quick_key_click++;
					}
					quick_key_time = clocknow;
					if (quick_key_click) {
						mAppMain->postMessageDelay(300, CALLBACK_MSG_QUICK_SHORT_KEY, keycode, quick_key_click);
					} else {
						if (!isScreenOff && !Global_FactoryTesting) {
							mAppMain->screenOff();
							Global_USB_State = device_usb_online();
						}
						db_verbose("STOP power key only set screenOff");
					}
					return HOOK_STOP;
				}
			}
		}
	}

	if (isScreenOff) {
		db_verbose("STOP ScreenOff");
		return HOOK_STOP;
	}
	return HOOK_GOON;
}

int GuiMain::keyProc(int keyCode, int isLongPress) {
	db_msg("key:%d long:%d curWindowID:%d", keyCode, isLongPress, mCurWindowID);
	if (!mCurWindowID) {
		return 0;
	}

    long long d = getMsClockTime();
    if ((d - Global_BootMsClock) < 1300) {//wait app stable
        return 0;
    }

    DispatchKeyEvent(mCurWindowID, keyCode, isLongPress);
	return 0;
}

void GuiMain::DispatchKeyEvent(int winid, int keyCode, int isLongPress) {
	HWND hWnd = win_get_hwnd(winid);
	if (!IS_VALID_HWND(hWnd)) {
		db_error("invalid win:%d", winid);
		return;
	}
	if (loading_win_processing || Global_Key_Shield) {
		db_msg("loading_win_processing:%d key_shield:%d skip...", loading_win_processing, Global_Key_Shield);
		return;
	}
	int newWinid = SendMessage(hWnd, MSG_INPUT_KEY, (WPARAM) keyCode, (LPARAM) isLongPress);
	if (newWinid < 0 || newWinid >= WINDOWID_MAXID) {
		db_error("error new windowID :%d", newWinid);
		return;
	}
	if (newWinid && newWinid != mCurWindowID) {
		db_msg("key changed window from:%d => %d", mCurWindowID, newWinid);
		changeWindow(newWinid);
	}
}

void GuiMain::updateWindowFont() {
	HWND hChild;
	db_msg("update font");
}

PROC_RET GuiMain::mainWindowProc(HWND hWnd, PROC_MSG_TYPE message, WPARAM wParam, LPARAM lParam) {
	if (message != MSG_IDLE) {
		db_verbose("recived msg:0x%x %s, wParam: 0x%x lParam:0x%x", message, Message2Str(message), (int) wParam, (int) lParam);
		if (message == MSG_LBUTTONDOWN || message == MSG_LBUTTONUP || message == MSG_NCMOUSEMOVE) {
			db_verbose("TPEVENT msg:0x%x %s x:%d y:%d", message, Message2Str(message), LOWORD(lParam), HIWORD(lParam));
		}
	}
	if ((loading_win_processing || Global_Key_Shield) && isKeyEvent(message)) {
		db_msg("loading_win_processing:%d key_shield:%d skip key msg:%d %s", loading_win_processing, Global_Key_Shield, message, Message2Str(message));
		return 0;
	}
	static WPARAM downKey = 0;
	static long long downClock = 0;
	static bool settimer = false;

	PLOGFONT logFont;

	switch (message) {
		case MSG_CREATE:
			logFont = res_getFont(0);
			if (logFont == NULL) {
				db_error("invalid log font");
				return -1;
			}
			db_msg("init font type:%s, charset:%s, family:%s", logFont->type, logFont->charset, logFont->family);
			SetWindowFont(hWnd, logFont);
			break;
		case MSG_FONTCHANGED:
			updateWindowFont();
			break;
		case MSG_KEYDOWN: {
			downKey = wParam;
			downClock = getMsClockTime();
			if (!settimer && (wParam != INPUT_KEY_POWER) && is_support_long_key()) {
				settimer = true;
				SetTimer(hWnd, ID_TIMER_KEY, 60);//600ms
			}
		}
			break;
		case MSG_KEYUP:
			if (settimer) {
				settimer = false;
				KillTimer(hWnd, ID_TIMER_KEY);
			}
			if (wParam && (wParam == downKey)) {
				long usetime = getMsClockTime() - downClock;
				if (usetime > 800) {
					if (wParam == INPUT_KEY_POWER) { 
						keyProc(wParam, SHORT_PRESS);
					} else {
						keyProc(wParam, LONG_PRESS);
					}
				} else {
					keyProc(wParam, SHORT_PRESS);
				}
			}
			downKey = 0;
			break;
		case MSG_KEYLONGPRESS:
			downKey = 0;
			if (wParam) {
				keyProc(wParam, LONG_PRESS);
			}
			break;
		case MSG_TIMER:
			if (wParam == ID_TIMER_KEY) {
				settimer = false;
				KillTimer(hWnd, ID_TIMER_KEY);
				if (downKey) {
					SendMessage(hWnd, MSG_KEYLONGPRESS, downKey, 0);
					downKey = 0;
				}
			}
			break;
		case MSG_CHANGE_WINDOW:
			changeWindow(wParam);
			break;
		case MSG_DESTORY_WINDOW:
			destoryWindow(wParam);
			break;
		case MSG_CLOSE:
			DestroyMainWindow(hWnd);
			PostQuitMessage(hWnd);
			break;
		case MSG_DESTROY:
			break;
		case MSG_RM_LANG_CHANGED: {
			logFont = res_getFont(0);
			if (logFont == NULL) {
				db_error("invalid log font");
				return -1;
			}
			db_msg("type:%s, charset:%s, family:%s", logFont->type, logFont->charset, logFont->family);
			SetWindowFont(mHwnd, logFont);
		}
			break;

		default:
			break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

bool GuiMain::isShutdownIng() {
	return mAppMain->isShutdownIng();
}

void GuiMain::checkShutdown() {
	bool shutdowning = isShutdownIng();
	db_msg("shutdowning %d", shutdowning);
	if (shutdowning) {
		showShutdownLogo();
	}
}

int GuiMain::getCurWindowID() {
	return mCurWindowID;
}

void GuiMain::changeWindow(int windowID) {
	if (Global_FactoryTesting && (windowID == WINDOWID_MAINPANEL)) {
		db_msg("FactoryTesting NOT allow to mainpanel");
		windowID = WINDOWID_FACTORY;
	}
	if (mCurWindowID == windowID) {
		db_msg("CurWindowID == windowID:%d,skip...", windowID);
		return;
	}
	if (isShutdownIng()) {
		db_msg("isShutdownIng,skip...");
		return;
	}

	HWND toHwnd = win_get_hwnd(windowID);
	if (toHwnd == HWND_INVALID) {
		if (createWindow(windowID) == 0) {
			toHwnd = win_get_hwnd(windowID);
		}
	}
	if (toHwnd == HWND_INVALID) {
		db_error("invalid toHwnd");
		return;
	}
	db_debug("changeWindow from:%d => %d", mCurWindowID, windowID);

	if (windowID == WINDOWID_SCAN) {
		mAppMain->postMessage(CALLBACK_MSG_CAMERA_PREVIEW_STATE, 1);
		win_send_notify(WINDOWID_SCAN, MSG_CHANGE_WIN_FROM, mCurWindowID, 0);
	} else if (mCurWindowID == WINDOWID_SCAN) {
		mAppMain->postMessage(CALLBACK_MSG_CAMERA_PREVIEW_STATE, 0);
		usleep(50*1000);//wait cam close show buff
		UpdateWindow(toHwnd, TRUE);//brush fb to del scan fb
	}
	int oldwin = mCurWindowID;
	win_hide_window(oldwin);
	ShowWindow(toHwnd, SW_SHOWNORMAL);
	mCurWindowID = windowID;

	CommonWindow *win = getWindowObject(oldwin);
	if (win && !win->isHold()) {
		SendMessage(mHwnd, MSG_DESTORY_WINDOW, oldwin, 0);
	}
}

void GuiMain::xchangeWindow(int windowID) {
	db_msg("new winid:%d", windowID);
	SendMessage(mHwnd, MSG_CHANGE_WINDOW, windowID, 0);
}

void GuiMain::showShutdownLogo() {
	db_msg("ShutdownLogo start");
	char path[128];
	snprintf(path, sizeof(path), "%s/img/icon/shutdown_logo.bmp", get_system_res_point());
	show_shutdown_logo(path);
	db_msg("ShutdownLogo end");
}

int GuiMain::onQrResult(int type, qr_packet *packet) {
	if (mCurWindowID != WINDOWID_SCAN) {
		db_error("error winid:%d", mCurWindowID);
		return -1;
	}
	if (!packet->data) {
		db_error("type:%d not data", type);
		return -1;
	}

	ProtoClientMessage *msg = proto_decode_client_message(packet);
	if (!msg) {
		db_error("decode qr false type:%d", type);
		sendMessage(WINDOWID_QRPROC, MSG_QR_ERROR, packet->type ? QR_DECODE_UNSUPPORT_MSG : QR_DECODE_INVALID_MSG, 0);
		xchangeWindow(WINDOWID_QRPROC);
		return 0;
	}
    //hw need upgrade
    if (msg->min_hw_version > DEVICE_APP_INT_VERSION) {
        db_error("expect min_hw_version:%d, current version:%d", msg->min_hw_version, DEVICE_APP_INT_VERSION);
        sendMessage(WINDOWID_QRPROC, MSG_QR_ERROR, QR_DECODE_UNSUPPORT_MSG, msg->type);
        proto_client_message_delete(msg);
        xchangeWindow(WINDOWID_QRPROC);
        return 0;
    }
    //invalid account
	if (msg->account_id && gSeedAccountId && msg->account_id != ((uint32_t) gSeedAccountId)) {
		db_error("msg type:%d account:%x seed account:%llx ", msg->type, msg->account_id, gSeedAccountId);
		sendMessage(WINDOWID_QRPROC, MSG_QR_ERROR, QR_DECODE_ACCOUNT_MISMATCH, msg->type);
		proto_client_message_delete(msg);
		xchangeWindow(WINDOWID_QRPROC);
		return 0;
	}

	int invalid = 1;
	int winid = 0;
	do {
		if (!msg->type) {
			db_error("empty type");
			break;
		}
		if ((!gHaveSeed && msg->type < QR_MSG_INIT_BASE) || (gHaveSeed && msg->type > QR_MSG_INIT_BASE)) {
			db_serr("invalid state qr type:%d seed:%d", type, gHaveSeed);
			break;
		}
		if (msg->type == QR_MSG_USER_ACTIVE && device_get_active_time() > 0) {
			db_serr("device actived,skip...");
			break;
		}
		winid = get_message_process_winid(msg);
		if (!winid) {
			db_serr("invalid msg type:%d", msg->type);
			break;
		}
		invalid = 0;
	} while (0);

	if (invalid) {
		proto_client_message_delete(msg);
		return -1;
	}
	db_msg("send qr result type:%d client:%d -> win:%d", msg->type, msg->client_id, winid);
	sendMessage(winid, MSG_QR_RESULT, msg->type, (LPARAM) msg);
	xchangeWindow(winid);
	return 0;
}

void GuiMain::msgLoop(void) {
	MSG Msg;
	db_msg("begin");
	while (GetMessage(&Msg, mHwnd)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	MainWindowThreadCleanup(mHwnd);
	db_msg("end");
}

#ifdef DEBUG_IN_CMD_LINE

static void testQr(int argc, const char *argv[]) {
	unsigned char bindata[1024];
	db_debug("start qr hex:%s", argv[0]);
	qr_packet _p;
	qr_packet *p = &_p;
	init_qr_packet(p, 0);

	int binlen = 0;
	utils_hex_to_bin(argv[0], bindata, strlen(argv[0]), &binlen);
	int ret = decode_qr_packet((const char *) bindata, binlen, p);
	db_debug("decode_qr_packet binlen:%d ret:%d", binlen, ret);
	if (ret != 0) {
		return;
	}
	QrDecoder *decoder;;
	decoder = QrDecoder::getInstance();
	if (decoder == NULL) {
		db_error("new QrDecoder false");
		return;
	}
	SecureAgent *agent = SecureAgent::getInstance();
	if (p->type == QR_MSG_BITCOIN_SIGN_REQUEST) {
		pbc_rmessage *rmsg = decoder->decodeMessage(p, "Wallet.BitcoinSignRequest";
		if (rmsg == NULL) {
			db_error("BitcoinSignRequest false");
			return;
		}
		cstring *tx = agent->txSign(rmsg);
	}
	db_debug("end");
}

#endif

int MiniGUIAppMain(int argc, const char *argv[]) {
#ifdef CONFIG_DEBUG_PERFORMANCE
	DEBUG_TESTTIME0 = debugGetClockTime();
	DEBUG_TESTTIME1 = DEBUG_TESTTIME0;
#endif
#ifdef DEBUG_IN_CMD_LINE
	if (args > 1) {
		if (!strcmp(argv[1], "qr")) {
			testQr(args - 2, argv + 2);
		} else {
			printf("unkown test arg:%s\n", argv[1]);
		}
		return 0;
	}
	printf("userage:%s cmd ....\n", argv[0]);
	return 0;
#endif
	DEBUG_PERFORMANCE();
	win_init();
	DEBUG_PERFORMANCE();
	GuiMain *guimain = new GuiMain();
	DEBUG_PERFORMANCE();
	mStaticGuiMain = guimain;
	if (wallet_main_entry(argc, argv, guimain) != 1) {
		ALOGE("call wallet_main_entry false");
		delete guimain;
		return 0;
	}
	DEBUG_PERFORMANCE();
	guimain->initStage2();
	DEBUG_PERFORMANCE();
	guimain->msgLoop();
	delete guimain;
	return 0;
}

static void gin_handler(int signo) {
	ALOGE("sign:%d", signo);
}

int main(int args, const char *argv[]) {
	int ret;
#ifdef BUILD_FOR_DEV
	if (strstr(argv[0], "wallet_app")) {
		fprintf(stderr, "App run in dev mode\n");
		ALOGD("App run in dev mode\n");
		set_app_run_mode(1);
	}
#endif
	Global_BootMsClock = (unsigned int) getMsClockTime();
	ALOGD("guimain clock:%u now:%ld", Global_BootMsClock, time(NULL));
	signal(SIGTRAP, gin_handler);
	if (InitGUI(args, argv) != 0) {
		return 1;
	}
	ret = MiniGUIAppMain(args, argv);
	TerminateGUI(ret);
	return ret;
}
