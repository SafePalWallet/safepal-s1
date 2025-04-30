#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H

#include "events.h"

class EventListener {
public:
	EventListener() {};

	virtual ~EventListener() {};

	virtual int notify(int message, int val) = 0;

};

class EventManager {
public:
	EventManager();

	~EventManager();

	int init();

	int exit();

	void setEventListener(EventListener *pListener);
	
	int ueventLoop(void);
	
private:
	EventListener *mListener;
	
};

#endif
