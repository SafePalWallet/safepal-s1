#ifndef WALLET_CAMERA_MANAGER_H
#define WALLET_CAMERA_MANAGER_H

#include "qr_pack.h"
#include "LocalMutex.h"

class CameraManager {
public:
	CameraManager();

	virtual ~CameraManager();

	int init();

	virtual int camExist();

	virtual void startPreview();

	virtual void stopPreview();

    virtual void stopPreviewNoLock();
	
	virtual void qrChunk(int datalen, char *data);

	virtual bool isPreviewing();

	void onQrResult(const char *data, int size);

private:
	Mutex mLock;
	bool mQrEnable;
	cstring *mLastQrResult;
	qr_packet mQrResult;
	qr_packet_buffer mQrBuffer;

	int clearQrDecode(int type);

	int setQrDecode(bool enable);
};

#endif
