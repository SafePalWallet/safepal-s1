#ifndef WALLET_CAMERA_ADAPTER_H
#define WALLET_CAMERA_ADAPTER_H

typedef void (*qr_callback)(const void *data, int size, void *caller);

class CameraAdapter {
public:
	CameraAdapter() {}

	virtual ~CameraAdapter() {}

	virtual int init(qr_callback cb, void *caller) = 0;

	virtual int camExist() = 0;

	virtual int startPreview() = 0;

	virtual int stopPreview() = 0;

	virtual int setQrDecode(bool enable) = 0;

	virtual int qrChunk(int datalen, char *data) = 0;

};

#endif
