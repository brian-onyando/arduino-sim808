#include "SIMComAT.h"
#include <errno.h>

void SIMComAT::begin(Stream& port)
{
	_port = &port;
	_output.begin(LOG_LEVEL_VERBOSE, this, false);
#if _SIM808_DEBUG
	_debug.begin(LOG_LEVEL_VERBOSE, &Serial, false);
#endif // _SIM808_DEBUG
}

void SIMComAT::flushInput()
{
	//immediatly or soon available lines while be discarded
	while (available() || (delay(100), available())) {
		readLine();
	}
}

void SIMComAT::send() 
{
	flushInput();
	_port->println();
}

size_t SIMComAT::readLine(uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT)
{
	uint16_t originalTimeout = timeout;
	do {

		uint8_t i = 0;
		timeout = originalTimeout;

		while (timeout-- && i < BUFFER_SIZE)
		{
			while (available())
			{
				char c = read();
				if (c == '\r') continue;
				if (c == '\n')
				{
					if (i == 0) continue; //beginning of a new line
					else //end of the line
					{
						timeout = 0;
						break;
					}
				}
				replyBuffer[i] = c;
				i++;
			}

			delay(1);
		}

		replyBuffer[i] = 0; //string term

		RECEIVEARROW;
		SIM808_PRINT(replyBuffer);
		SIM808_PRINT(CR);
	} while (strstr_P(replyBuffer, PSTR("OVER-VOLTAGE")) != NULL); //discard over voltage warnings

	delay(100);
	return strlen(replyBuffer);
}

void SIMComAT::readNextLine(uint16_t timeout)
{
	uint8_t i = 0;
	memset(replyBuffer, 0, BUFFER_SIZE);

	uint16_t start = millis();

	do {
		while(available()) {
			char c = read();
			replyBuffer[i] = c;
			i++;

			if(c == '\n') {
				timeout = 0;
				break;
			}
		}
	} while(millis() - start < timeout);

	replyBuffer[i] = '\0';
}

int8_t SIMComAT::waitResponse(uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT, 
	Sim808ConstStr s1 = SFP(TOKEN_OK),
	Sim808ConstStr s2 = SFP(TOKEN_ERROR),
	Sim808ConstStr s3 = NULL,
	Sim808ConstStr s4 = NULL)
{
	uint16_t start = millis();
	Sim808ConstStr wantedTokens[4] = { s1, s2, s3, s4 };

	do {
		readNextLine(timeout - (millis() - start));
		for(uint8_t i = 0; i < 4; i++) {
			if(wantedTokens[i] && strstr_P(replyBuffer, SFPT(wantedTokens[i])) == replyBuffer) return i;
		}
	} while(!millis() - start < timeout);

	return -1;
}

template<typename T>size_t SIMComAT::sendGetResponse(T msg, char* response, uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT)
{
	SENDARROW;
	print(msg);
	return sendGetResponse(response, timeout);
}

size_t SIMComAT::sendGetResponse(char* response, uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT) 
{
	send();
	readLine(timeout);
	
	return copyResponse(response);
}

size_t SIMComAT::copyResponse(char * response)
{
	size_t len = strlen(replyBuffer);
	if (response != NULL) {
		size_t maxLen = min(len + 1, BUFFER_SIZE - 1);
		strlcpy(response, replyBuffer, maxLen);
	}

	return len;
}

bool SIMComAT::sendAssertResponse(const __FlashStringHelper *msg, const __FlashStringHelper *expectedResponse, uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT)
{
	if (!sendGetResponse(msg, NULL, timeout)) return false;
	return assertResponse(expectedResponse);
}

bool SIMComAT::sendAssertResponse(const __FlashStringHelper *expectedResponse, uint16_t timeout = SIMCOMAT_DEFAULT_TIMEOUT)
{
	if (!sendGetResponse(NULL, timeout)) return false;
	return assertResponse(expectedResponse);
}

bool SIMComAT::assertResponse(const __FlashStringHelper *expectedResponse)
{
	SIM808_PRINT_P("assertResponse : [%s], [%S]", replyBuffer, expectedResponse);
	return !strcasecmp_P(replyBuffer, (char *)expectedResponse);
}

char* SIMComAT::find(const char* str, char divider, uint8_t index)
{
	SIM808_PRINT_P("find : [%s, %c, %i]", str, divider, index);

	char* p = strchr(str, ':');
	if (p == NULL) p = strchr(str, str[0]); //ditching eventual response header

	p++;
	for (uint8_t i = 0; i < index; i++)
	{
		p = strchr(p, divider);
		if (p == NULL) return NULL;
		p++;
	}

	SIM808_PRINT_P("find : [%s, %c, %i], [%s]", str, divider, index, p);

	return p;
}

bool SIMComAT::parse(const char* str, char divider, uint8_t index, uint8_t* result)
{
	uint16_t tmpResult;
	if (!parse(str, divider, index, &tmpResult)) return false;

	*result = (uint8_t)tmpResult;
	return true;
}

bool SIMComAT::parse(const char* str, char divider, uint8_t index, int8_t* result)
{
	int16_t tmpResult;
	if (!parse(str, divider, index, &tmpResult)) return false;

	*result = (int8_t)tmpResult;
	return true;
}

bool SIMComAT::parse(const char* str, char divider, uint8_t index, uint16_t* result)
{
	char* p = find(str, divider, index);
	if (p == NULL) return false;

	errno = 0;
	*result = strtoul(p, NULL, 10);

	return errno == 0;
}

bool SIMComAT::parse(const char* str, char divider, uint8_t index, int16_t* result)
{	
	char* p = find(str, divider, index);
	if (p == NULL) return false;

	errno = 0;
	*result = strtol(p, NULL, 10);
	
	return errno == 0;
}

bool SIMComAT::parse(const char* str, char divider, uint8_t index, float* result)
{
	char* p = find(str, divider, index);
	if (p == NULL) return false;

	errno = 0;
	*result = strtod(p, NULL);

	return errno == 0;
}
