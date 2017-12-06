#include "SIM808.h"

SIM808_COMMAND(SET_ECHO, "ATE%d");

SIM808_TOKEN(RDY);
SIM808_TOKEN(AT);

SIM808::SIM808(uint8_t resetPin, uint8_t pwrKeyPin, uint8_t statusPin)
{
	_resetPin = resetPin;
	_pwrKeyPin = pwrKeyPin;
	_statusPin = statusPin;
	_ok = PSTRPTR(SIM808_TOKEN_OK);

	pinMode(_resetPin, OUTPUT);
	pinMode(_pwrKeyPin, OUTPUT);
	pinMode(_statusPin, INPUT);
	
	digitalWrite(_pwrKeyPin, HIGH);
	digitalWrite(_resetPin, HIGH);
}

SIM808::~SIM808() { }

#pragma region Public functions

void SIM808::init()
{
	reset();
	waitForReady();
	delay(3000);

	setEcho(SIM808_ECHO::OFF);
	setEcho(SIM808_ECHO::OFF); //two times make asserts headache-less
}

void SIM808::reset()
{
	digitalWrite(_resetPin, HIGH);
	delay(10);
	digitalWrite(_resetPin, LOW);
	delay(200);

	digitalWrite(_resetPin, HIGH);
}

void SIM808::waitForReady()
{
	do
	{
		SIM808_PRINT_SIMPLE_P("Waiting for echo...");
	} while (!sendAssertResponse(PSTRPTR(SIM808_TOKEN_AT), PSTRPTR(SIM808_TOKEN_AT)));

	do
	{
		SIM808_PRINT_SIMPLE_P("Waiting for RDY...");
		readLine();
	} while (!assertResponse(PSTRPTR(SIM808_TOKEN_RDY)));

}

bool SIM808::setEcho(SIM808_ECHO mode)
{
	SENDARROW;
	_output.verbose(PSTRPTR(SIM808_COMMAND_SET_ECHO), mode);

	return sendAssertResponse(_ok);
}

size_t SIM808::sendCommand(const char *cmd, char *response)
{
	flushInput();
	_output.verbose(cmd);
	sendGetResponse(response);
	readLine();

	return strlen(response);
}

#pragma endregion


