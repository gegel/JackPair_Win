#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdio.h> //printf
#include <stdlib.h> //abs


#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "comport.h"

    HANDLE file=INVALID_HANDLE_VALUE;
    DWORD read=0;
    volatile DWORD opened=0;

int InitPort(int mport)
{

    COMMTIMEOUTS timeouts;
    DCB port;
    char port_name[128];// = "\\\\.\\COM2";

    opened=0;

 if(!mport)
 {
  if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
  file=INVALID_HANDLE_VALUE;
 }

 sprintf(port_name, "\\\\.\\COM%d", mport);
 //sprintf(port_name, "COM%c", mport);
 // open the comm port.
    file = CreateFile(port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if ( INVALID_HANDLE_VALUE == file) {
        return -1;
    }

     // get the current DCB, and adjust a few bits to our liking.
    memset(&port, 0, sizeof(port));
    port.DCBlength = sizeof(port);
    if ( !GetCommState(file, &port))
        {CloseHandle(file);
        file=INVALID_HANDLE_VALUE;
        return -2;}
    if (!BuildCommDCB("baud=115200 parity=n data=8 stop=1", &port))
        {CloseHandle(file);
        file=INVALID_HANDLE_VALUE;
        return -3;}
    if (!SetCommState(file, &port))
        {
         CloseHandle(file);
         file=INVALID_HANDLE_VALUE;
         return -4;
        }

    // set short timeouts on the comm port.
    timeouts.ReadIntervalTimeout = 0;//50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;//1000;
    timeouts.WriteTotalTimeoutMultiplier = 0;//1;
    timeouts.WriteTotalTimeoutConstant = 0;//1;
    if (!SetCommTimeouts(file, &timeouts))
        {CloseHandle(file);
        file=INVALID_HANDLE_VALUE;
        return -5;}
        opened=1;
    return 0;
}


int ReadPort(char* buf, int maxlen)
{
  read=0;
  if(!opened) return -2;
  if(file==INVALID_HANDLE_VALUE) return -1;
  // check for data on port and display it on screen.
  ReadFile(file, buf, maxlen-1, &read, NULL);
  return read;
}

int WritePort(char* buf, int len)
{
 DWORD written;
 if(!opened) return -2;
 if(file==INVALID_HANDLE_VALUE) return -1;
 written=0;
 written=WriteFile(file, buf, len, &written, NULL);
 return written;
}


void ClosePort(void)
{
 opened=0;
 if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
 file=INVALID_HANDLE_VALUE;
}


void SetRTS(unsigned char on)
{
  if((!opened)||(file==INVALID_HANDLE_VALUE)) return;
  EscapeCommFunction(file, on?SETRTS:CLRRTS);
}


void SetDTR(unsigned char on)
{
  if((!opened)||(file==INVALID_HANDLE_VALUE)) return;
  EscapeCommFunction(file, on?SETDTR:CLRDTR);
}

unsigned char GetCTS(void)
{
  DWORD dw;
  if((!opened)||(file==INVALID_HANDLE_VALUE)) return 2;
  GetCommModemStatus(file, &dw);
  return (unsigned char)(MS_CTS_ON&dw);
}

unsigned char GetDSR(void)
{
  DWORD dw;
 if((!opened)||(file==INVALID_HANDLE_VALUE)) return 2;
  GetCommModemStatus(file, &dw);
  return (unsigned char)(MS_DSR_ON&dw);
}

//get status of HW inputs
unsigned char GetCTSDSR(void)
{
  DWORD dw;
  unsigned char r=0; //default no actions
  if((!opened)||(file==INVALID_HANDLE_VALUE)) return 0;
  GetCommModemStatus(file, &dw);
  if(MS_CTS_ON&dw) r|=1; //CTS=0: set flag of PTT
  if(MS_DSR_ON&dw) r|=2; //DSR=0: set flag of Direct mode 
  return r;
}

void SetBRK(unsigned char on)
{
   if((!opened)||(file==INVALID_HANDLE_VALUE)) return;
   if (on) SetCommBreak(file);
   else  ClearCommBreak(file);
}


void Ql_UART_Write(int port, unsigned char* buf, int len)
{
 char b[4];
 b[0]=0x30+port;
 b[1]=':';
 b[2]=' ';
 WritePort(b, 3);
 WritePort(buf, len);
}

 
//output like printf to stdout and to control port
void uprintf(char* s, ...)
{
    char st[256];
    va_list ap;

    if((!opened)||(file==INVALID_HANDLE_VALUE)) return;
    va_start(ap, s);  //parce arguments
    vsprintf(st, s, ap); //printf arg list to array
    //printf(st);  //out to stdout
    WritePort(st, strlen(st));//sendweb(st); //out to control port
    va_end(ap);  //clear arg list
    return;
}

