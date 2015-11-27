
#include "uart.h"

class Serial
{
public:
	Serial()
	{
		_serial = serial_create();
	}
	~Serial()
	{
		serial_destroy(_serial);
	}

	int Connect(char device[], int baud)
	{
		return serial_connect(_serial, device, baud);
	}
	int Send(uint8_t data[], int length)
	{
		return serial_send(_serial, data, length);
	}
	void Put(uint8_t data)
	{
		return serial_put(_serial, data);
	}
	int Available()
	{
		return serial_available(_serial);
	}
	char Get()
	{
		return serial_get(_serial);
	}
	char Blocking_get()
	{
		return serial_blocking_get(_serial);
	}
	void Clear()
	{
		return serial_clear(_serial);
	}
	int Disconnect()
	{
		return serial_close(_serial);
	}
private:
	serial_t* _serial;
};
