#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void uprintf(char* s, ...);
int InitPort(int mport);
int ReadPort(char* buf, int maxlen);
int WritePort(char* buf, int len);
void ClosePort(void);
void Ql_UART_Write(int port, unsigned char* buf, int len);
void SetRTS(unsigned char on);
void SetDTR(unsigned char on);
unsigned char GetCTS(void);
unsigned char GetDSR(void);
unsigned char GetCTSDSR(void);
void SetBRK(unsigned char on);

#ifdef __cplusplus
}
#endif
