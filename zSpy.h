#ifndef ZSPY_MESSAGE_INCLUDED
#define ZSPY_MESSAGE_INCLUDED

#include "SharedBase.h"
#include <string>

class zSpy {
private:
	HWND spyHandle;

public:
	zSpy() {}

	void Send(std::string msg)
	{
		spyHandle = FindWindowA(0, "[zSpy]");

		COPYDATASTRUCT data;
		data.lpData = (char*)msg.c_str();
		data.cbData = msg.size() + 1;

		SendMessageA(this->spyHandle, WM_COPYDATA, (WPARAM)0, (LPARAM)&data);
	}
};
zSpy spy;
#endif