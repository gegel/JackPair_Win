/*
 Enumerating com ports in Windows 

 original by PJ Naughter, 1998 - 2013 
 http://www.naughter.com/enumser.html  (Web: www.naughter.com, Email: pjna@naughter.com)

 reorganize by Gaiger Chen , Jul, 2015

 NO COPYRIGHT,  welcome to use for everyone.
*/

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <tchar.h>
#include <setupapi.h>
#include <locale.h>

#define MAX_PORT_NUM      (256)
#define MAX_STR_LEN       (256*sizeof(TCHAR))

BOOL EnumerateComPortQueryDosDevice(UINT *pNumber, TCHAR *pPortName, int strMaxLen)
{
 UINT i, jj;
 INT ret;

 OSVERSIONINFOEX osvi;
 ULONGLONG dwlConditionMask;
 DWORD dwChars;
 
 TCHAR *pDevices;   
 UINT nChars;

 ret = FALSE;
 
 memset(&osvi, 0, sizeof(osvi));
 osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
 osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;
 dwlConditionMask = 0;

 VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);

 if(FALSE == VerifyVersionInfo(&osvi, VER_PLATFORMID, dwlConditionMask))
 {
   DWORD dwError = GetLastError();
  printf((char*)("VerifyVersionInfo error, %d\n", dwError));
  return -1;
 }/*if*/

   
 pDevices = NULL;

 nChars = 4096;   
 pDevices = (TCHAR*)HeapAlloc(GetProcessHeap(), 
  HEAP_GENERATE_EXCEPTIONS, nChars*sizeof(TCHAR));

 while(0 < nChars)
 {
  dwChars = QueryDosDevice(NULL, pDevices, nChars);

  if(0 == dwChars)
  {
   DWORD dwError = GetLastError();

   if(ERROR_INSUFFICIENT_BUFFER == dwError)
   {      
    nChars *= 2;    
    HeapFree(GetProcessHeap(), 0, pDevices);    
    pDevices = (TCHAR*)HeapAlloc(GetProcessHeap(), 
     HEAP_GENERATE_EXCEPTIONS, nChars*sizeof(TCHAR));

    continue;
   }/*if ERROR_INSUFFICIENT_BUFFER == dwError*/
   
   _tprintf((char*)("QueryDosDevice error, %d\n", dwError));
   return -1;     
  }/*if */


  //printf("dwChars = %d\n", dwChars);
  i = 0;
  jj = 0;
  while (TEXT('\0') != pDevices[i] )
  {
   TCHAR* pszCurrentDevice;
   size_t nLen;
   pszCurrentDevice = &(pDevices[i]);
   nLen = _tcslen(pszCurrentDevice);

   //_tprintf(TEXT("%s\n"), &pTargetPathStr[i]);
   if (3 < nLen)
            {
              if ((0 == _tcsnicmp(pszCurrentDevice, TEXT("COM"), 3))
      && FALSE != isdigit(pszCurrentDevice[3]) )
              {
                //Work out the port number                
    _tcsncpy(pPortName + jj*strMaxLen, 
     pszCurrentDevice, strMaxLen);
    jj++; 
    
              }
            }

   i += (nLen + 1);
  }
  
  break;
 }/*while*/
 
 if(NULL != pDevices)
  HeapFree(GetProcessHeap(), 0, pDevices); 
 
  
 *pNumber = jj;

 if(0 < jj)
  ret = TRUE;

 return ret;
}/*EnumerateComPortByQueryDosDevice*/

BOOL EnumerateComPortByGetDefaultCommConfig(UINT *pNumber, TCHAR *pPortName, 
  int strMaxLen)
{
 UINT i, jj;
 INT ret;
 
 TCHAR *pTempPortName; 

 pTempPortName = (TCHAR*)HeapAlloc(GetProcessHeap(), 
  HEAP_GENERATE_EXCEPTIONS| HEAP_ZERO_MEMORY, strMaxLen);

 *pNumber = 0;
 jj = 0;
 ret = FALSE;

 for (i = 1; i<=255; i++){    

  //Form the Raw device name    
  COMMCONFIG cc;
  DWORD dwSize ;

  dwSize = sizeof(COMMCONFIG);

  snprintf(pTempPortName, strMaxLen/2, TEXT("COM%u"), i);

  if (FALSE == GetDefaultCommConfig(pTempPortName, &cc, &dwSize))
    continue;
    
  strncpy(pPortName + jj*strMaxLen, pTempPortName,
   strMaxLen);
  jj++;
 }/*for [1 255] */

 HeapFree(GetProcessHeap(), 0, pTempPortName);
  pTempPortName = NULL;

 *pNumber = jj;

 if(0 <jj)
  ret = TRUE;

 return ret;
}/*EnumerateComPortByGetDefaultCommConfig*/

#define TIMER_BEGIN(TIMER_LABEL) \
   { unsigned int tBegin##TIMER_LABEL, tEnd##TIMER_LABEL; \
   tBegin##TIMER_LABEL = GetTime();

#define TIMER_END(TIMER_LABEL)  \
   tEnd##TIMER_LABEL = GetTime();\
   fprintf(stderr, "%s cost time = %d ms\n", \
   #TIMER_LABEL, tEnd##TIMER_LABEL - tBegin##TIMER_LABEL); \
   }

unsigned int GetTime(void)
{
 /*winmm.lib*/
 return ( unsigned int)timeGetTime();
}/*GetTime*/


int SearchCom(int cl[16])
{
 TCHAR portName[MAX_PORT_NUM][MAX_STR_LEN];
 TCHAR friendlyName[MAX_PORT_NUM][MAX_STR_LEN];
 UINT i;
 UINT j;
 UINT k;
 UINT n;

 TIMER_BEGIN(EnumerateComPortQueryDosDevice);
 EnumerateComPortQueryDosDevice(&n, &portName[0][0], MAX_STR_LEN);
 TIMER_END(EnumerateComPortQueryDosDevice);

 for(i=0;i<16;i++) cl[i]=0;
 if(n>16) k=16; else k=n;
 for(i = 0; i< k; i++)
 {
  printf((char*)("\t%s\n"), &portName[i][0]);
  j=atoi(&portName[i][3]);
  cl[i]=j;
 }
 return n;
}

/*
int main(int argc, TCHAR argv[])
{
 TCHAR portName[MAX_PORT_NUM][MAX_STR_LEN];
 TCHAR friendlyName[MAX_PORT_NUM][MAX_STR_LEN];
 UINT i;
 UINT n;
 printf((char*)("\nQueryDosDevice method : "));
 for(i = 0; i< MAX_PORT_NUM; i++) ZeroMemory(&portName[0][0], MAX_STR_LEN);
 TIMER_BEGIN(EnumerateComPortQueryDosDevice);
 EnumerateComPortQueryDosDevice(&n, &portName[0][0], MAX_STR_LEN);
 TIMER_END(EnumerateComPortQueryDosDevice);
 printf((char*)("sought %d:\n"), n);
 for(i = 0; i< n; i++) printf("\t%s\n", &portName[i][0]);

 for(i = 0; i< MAX_PORT_NUM; i++)
 ZeroMemory(&portName[i][0], MAX_STR_LEN);

 printf((char*)("\nGetDefaultCommConfig method : \n"));
 TIMER_BEGIN(EnumerateComPortByGetDefaultCommConfig);
 EnumerateComPortByGetDefaultCommConfig(&n, &portName[0][0], MAX_STR_LEN);
 TIMER_END(EnumerateComPortByGetDefaultCommConfig);
 printf((char*)("sought %d:\n"), n);

 for(i = 0; i< n; i++)
 printf((char*)("\t%s\n"), &portName[i][0]);

 return 0;
}

*/

