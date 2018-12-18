 ///////////////////////////////////////////////
//
// **************************
//
// Project/Software name: X-Phone
// Author: "Van Gegel" <gegelcopy@ukr.net>
//
// THIS IS A FREE SOFTWARE  AND FOR TEST ONLY!!!
// Please do not use it in the case of life and death
// This software is released under GNU LGPL:
//
// * LGPL 3.0 <http://www.gnu.org/licenses/lgpl.html>
//
// You’re free to copy, distribute and make commercial use
// of this software under the following conditions:
//
// * You have to cite the author (and copyright owner): Van Gegel
// * You have to provide a link to the author’s Homepage: <http://torfone.org>
//
///////////////////////////////////////////////


//This file contains low-level procedure for wave audio
//Win32 system for X-Phone (OnionPhone) project

//FOR WIN32 ONLY!

#include <basetsd.h>
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#include "audio_wave48.h"
#define UNREF(x) (void)(x)

#define _SampleRate 48000  //Samle Rate
#define _BitsPerSample 16  //PCM16 mode
#define _Channels 1     // mono
#define _CHSIZE 360*6 //chunk size in bytes (2 * sample125us)
#define _CHNUMS 16  //number of chunks for buffer size 2400 samples (300 mS) can be buffered
#define _ROLLMASK (_CHNUMS-1) //and-mask for roll buffers while pointers incremented
//=======================================================

HWAVEIN _In=0;                   // Wave input device
HWAVEOUT _Out=0;                 // Wave output device
WAVEFORMATEX _Format;          // Audio format structure
UINT _BytesPerSample;          // Bytes in sample (2 for PCM16)
HANDLE _WorkerThreadHandle=0;    // Wave thread
DWORD _WorkerThreadId=0;         // ID of wave thread

int _IsSound=0;   //flag: sound OK
int _IsGo=0;     //flag: input runs
int _DevInN=0;    //system input numer devicce specified in donfig file
int _DevOutN=0;   //system output numer devicce specified in donfig file
int _ChSize=_CHSIZE; //chunk size
int _ChNums=_CHNUMS; //number of chunks

//audio input
WAVEHDR *_Hdr_in[2]; //headers for 2 chunks
unsigned char _wave_in[_CHNUMS][_CHSIZE+40];  //output buffers for 16 frames 20 mS each
volatile unsigned char _p_in=0; //pointer to chunk will be returned by input device
unsigned char _g_in=0; //pointer to chunk will be readed by application
int _n_in=0; //number of bytes ready in this frame

int _IsGrab=0;    //flags for grab/play devices was initialized
int _IsPlay=0;

//audio output
WAVEHDR *_Hdr_out[_CHNUMS]; //headers for each chunk
unsigned char _wave_out[_CHNUMS][_CHSIZE+40]; //output buffers for 16 frames 20 mS each
unsigned char _p_out=0;  //pointer to chunk will be passed to output device
int _n_out=0;  //number of bytes already exist in this frame
volatile unsigned char _g_out=0;  //pointer to chunk will be returned by output device

//Internal procedures
int _OpenDevices (void); //open in & out devices
void _StopDevices (void); //paused
void _CloseDevices (void); //closed devised (finalize)
void _ReleaseBuffers(void); //de - allocate memory


int _dlg_start(void); //create tread


//*****************************************************************************
//Stor audio input/autput
void _StopDevices (void)
{
  if(_In) waveInReset (_In);
  if(_Out) waveOutReset (_Out);
}

//*****************************************************************************
//Close audio input/output
void _CloseDevices (void)
{
 if (_In) waveInClose (_In);
 _In = NULL;
 if (_Out) waveOutClose (_Out);
 _Out = NULL;
}

//*****************************************************************************
//release dynamically allocated wave header
void _ReleaseBuffers(void)
{
 volatile int i;
 if(_WorkerThreadId) PostThreadMessage (_WorkerThreadId, WM_QUIT, 0, 0);
 for(i=0;i<_CHNUMS; i++) if(_Hdr_out[i]) LocalFree ((HLOCAL)_Hdr_out[i]);
 for(i=0;i<_CHNUMS; i++) if(_Hdr_in[i]) LocalFree ((HLOCAL)_Hdr_in[i]);
}

//*****************************************************************************
//Open and init wave input and output
int _OpenDevices (void)
{
  MMRESULT Res;
  volatile int i;
  WAVEHDR *Hdr;
  int ret=0;

  InitCommonControls (); //init system controls
  _In = NULL; //clear devices pointers
  _Out = NULL;
  //set audio format
  _BytesPerSample = (_BitsPerSample + 7) / 8;
  _Format.wFormatTag = WAVE_FORMAT_PCM;
  _Format.nChannels = _Channels;
  _Format.wBitsPerSample = _BitsPerSample;
  _Format.nBlockAlign = (WORD)(_Channels * _BytesPerSample);
  _Format.nSamplesPerSec = _SampleRate;
  _Format.nAvgBytesPerSec = _Format.nSamplesPerSec * _Format.nBlockAlign;
  _Format.cbSize = 0;

  //---------------------Open Devices----------------
  //Output
  if(_DevOutN>=0)
  {
  Res = waveOutOpen (
      (LPHWAVEOUT)&_Out,
      _DevOutN - 1,
      &_Format,
      (DWORD)_WorkerThreadId,
      0,
      CALLBACK_THREAD
    );
  if (Res != MMSYSERR_NOERROR) _Out=0;
  }

  //Input
  if(_DevInN>=0)
  {
  Res = waveInOpen (
      (LPHWAVEIN)&_In,
      _DevInN - 1,
      &_Format,
      (DWORD)_WorkerThreadId,
      0,
      CALLBACK_THREAD
    );
  if (Res != MMSYSERR_NOERROR) _In=0;
  }
  //------------Allocates wave headers----------------
 if(_Out)
 {
  waveOutPause (_Out); //stop output device;
  //For output device allocate header for each frame in buffer
  for(i=0; i<_CHNUMS; i++)
  {
   Hdr = (WAVEHDR *)LocalAlloc (LMEM_FIXED, sizeof (*Hdr));
   if (Hdr)
   {
    Hdr->lpData = (char*)_wave_out[i];
    Hdr->dwBufferLength = _ChSize;
    Hdr->dwFlags = 0;
    Hdr->dwLoops = 0;
    Hdr->dwUser = 0;
    _Hdr_out[i]=Hdr;
   }
   else
   {
    for(i=0;i<_CHNUMS; i++) if(_Hdr_out[i]) LocalFree ((HLOCAL)_Hdr_out[i]);
    break;
   }
  }

  if(i==_CHNUMS)
  {
  //init output pointers
   _p_out=0;
   _g_out=0;
   _n_out=0;
   printf("Headset ready\r\n");
   ret|=1;
  }
 }

 if(_In)
 {
  //-------------------------init wave input---------------
  //Allocates 2 headers
  for(i=0; i<2; i++)
  {
   Hdr = (WAVEHDR *)LocalAlloc (LMEM_FIXED, sizeof (*Hdr));
   if (Hdr)
   {
    Hdr->lpData = (char*)_wave_in[i];
    Hdr->dwBufferLength = _ChSize;
    Hdr->dwFlags = 0;
    Hdr->dwLoops = 0;
    Hdr->dwUser = 0;
    _Hdr_in[i]=Hdr;
    waveInPrepareHeader (_In, Hdr, sizeof (WAVEHDR));
    Res = waveInAddBuffer (_In, Hdr, sizeof (WAVEHDR));
   }
   if( (!Hdr) || (Res != MMSYSERR_NOERROR) )
   {
    //for(i=0;i<_CHNUMS; i++) if(_Hdr_out[i]) LocalFree ((HLOCAL)_Hdr_out[i]);
    for(i=0;i<_CHNUMS; i++) if(_Hdr_in[i]) LocalFree ((HLOCAL)_Hdr_in[i]);
    return ret;
   }
  }
  //init input pointers
  _p_in=0;
  _g_in=0;
  _n_in=0;
  printf("Mike ready\r\n");
  ret|=2;
 }
  return ret;
}
//========================Top level=================

//*****************************************************************************
//init wave devices
int _soundinit(char* device_in, char* device_out)
{
  unsigned int i;
  _IsSound=0; //set sound flag to OK

  //set input device for use
  if(!device_in[0]) _DevInN=-1;
  else if(device_in[0]!='#') for(i=0;i<strlen(device_in); i++)
  {
   if((device_in[i]>='0')&&(device_in[i]<='9')) //search first number in the string
   {
    _DevInN=device_in[i]-'0';  //set recording wave device by this number
    break;
   }
  }

  //set input device for use
  if(!device_out[0]) _DevOutN=-1;
  else if(device_out[0]!='#') for(i=0;i<strlen(device_out); i++)
  {
   if((device_out[i]>='0')&&(device_out[i]<='9'))   //search first number in the string
   {
    _DevOutN=device_out[i]-'0';  //set playing wave device by this number
    break;
   }
  }

  printf("In/out wave devices %d/%d will be used\r\n", _DevInN, _DevOutN);
  
  //dlg_init(); //print list of avaliable sound devices
  _IsSound=_dlg_start(); //open wave input and wave output
  return _IsSound;
}


//*****************************************************************************
//terminate wave devices
void _soundterm(void)
{
 if(_WorkerThreadId) PostThreadMessage (_WorkerThreadId, WM_QUIT, 0, 0);
 Sleep(500);

 CloseHandle (_WorkerThreadHandle); //stop audio thread
 _WorkerThreadId = 0;
 _WorkerThreadHandle = NULL;
 //StopDevices ();  //paused
 _CloseDevices (); //close devices
 Sleep(500); //some time for close compleet
 _ReleaseBuffers(); //release memory
 //SetPriorityClass (GetCurrentProcess (), DefPrioClass); //restore priority of main procedure
 printf("Wave Thread stopped!\r\n");

 _In=0;                   // Wave input device
 _Out=0;                 // Wave output device
 _WorkerThreadHandle=0;    // Wave thread
 _WorkerThreadId=0;         // ID of wave thread

 _IsSound=0;   //flag: sound OK
 _IsGo=0;     //flag: input runs
 _DevInN=0;    //system input numer devicce specified in donfig file
 _DevOutN=0;   //system output numer devicce specified in donfig file
 _ChSize=_CHSIZE; //chunk size
 _ChNums=_CHNUMS; //number of chunks

 _IsGrab=0;    //flags for grab/play devices was initialized
 _IsPlay=0;

//audio input

 _p_in=0; //pointer to chunk will be returned by input device
 _g_in=0; //pointer to chunk will be readed by application
 _n_in=0; //number of bytes ready in this frame

//audio output
 _p_out=0;  //pointer to chunk will be passed to output device
 _n_out=0;  //number of bytes already exist in this frameg_out=0;  //pointer to chunk will be returned by output device


}

//*****************************************************************************
//Get number of samples in output queue
int _getdelay(void)
{
 int i=(int)_g_out; //read volatile value: pointer to next buffer will be released
 i=-i;
 i=i+_p_out; //buffers in queue now
 if(i<0) i=_CHNUMS+i; //correct roll
 i=i-2; //skip two work buffers
 if(i<0) i=0; //correct
 i=i*_ChSize; //bytes in queue
 i=i+_n_out; //add tail in current buffer
 i=i/2; //samples in queue
 return i;
}

//*****************************************************************************
//get number of samples in chunk (frame)
int _getchunksize(void)
{
 return _ChSize/2;
}

//*****************************************************************************
//get total buffers size in samples
int _getbufsize(void)
{
 return ((_ChSize*(_CHNUMS-1))/2);
}

//*****************************************************************************
//skip all samples grabbed before
void _soundflush(void)
{
 //in: not released yet
}

//*****************************************************************************
//skip all unplayed samples
void _soundflush1(void)
{
 //out: not released yet
}

//*****************************************************************************
//------------------------------------------------
//grab up to len samples from wave input device
//return number of actually getted samples
int _soundgrab(char *buf, int len)
{
 int i, l, d=0;
 unsigned char cpp, cp=_p_in; //read volatile value
 //p_in pointes to first frame in work now
 //p_in+1 frame also in work
 //p_in-1 frame was last returned by input device

 //g_in pointes to the most oldest unread frame
 //n_in is number of unreaded bytes in it (tail)
 if((!_In)||(!(_IsSound&2))) return 0;

 if(_IsGo)//check for device opened
 {
  cpp=cp+1;  //pointer to buffer passed to input device
  cpp&=_ROLLMASK;
  l=len*2; //length in bytes (for 16 bit audio samples)
  while(l>0) //process up to length
  { //check for pointed buffer not uses by input device now
   if((_g_in==cp)||(_g_in==cpp)) break; //2 chunks uses by input device at time
   i=_ChSize-_n_in; //ready bytes in this frame
   if(i>l) i=l;  //if we need less then exist
   memcpy(buf, &_wave_in[_g_in][_n_in], i); //copy to output
   d+=i; //bytes outed
   l-=i; //bytes remains
   _n_in+=i; //bytes processed in current frame
   if(_n_in>=_ChSize) //if all bytes of current frame processed
   {
    _g_in++;   //pointer to next frame
    _g_in=_g_in&_ROLLMASK; //roll mask (16 frames total)
    _n_in=0;  //no byte of this frame has not yet been read
   }
  }
 }
 //if(!d) Sleep(20);
 return (d/2); //returns number of outputted frames
}


//*****************************************************************************
//pass up to len samples from buf to wave output device for playing
//returns number of samples actually passed to wave output device
int _soundplay(int len, unsigned char *buf)
{
 #define STARTDELAY 2 //number of sylency chunk passed to output before playing
 int d=0; //bytes passed
 int i, l;
 unsigned char cp, cg=_g_out; //read volatile value
 //g_out pointes to last played (and empty now) frame
 //p_out points to frame for writing data now
 //at-least one frame must be played at time (normally two and more)
 //if no frames played at time (g_out==p_out) that is underrun occured
 //and we must push some frames of silency (for prevention next underrun)
 //and restart playing

 if((!(_IsSound&1))||(!_Out)) return len;  //check for device ready
 if(_p_out==cg) //underrun occured
 {
  for(i=0;i<STARTDELAY;i++) //pass to device some silency frames
  {
   memset(_wave_out[i], 0, _ChSize); //put silency to first frame
   waveOutPrepareHeader (_Out, _Hdr_out[i], sizeof (WAVEHDR)); //prepare header
   waveOutWrite (_Out, _Hdr_out[i], sizeof (WAVEHDR));    //pass it to wave output device
  }
  _g_out=0; //reset pointer of returned headers
  _p_out=i; //set pointer to next header to pass to device
  _n_out=0; //it is empty
  waveOutRestart (_Out); //restart wave output process
  return -1; //return error code
 }

 l=len*2; //length in bytes (for 16 bit audio samples)
 while(l) //process all input data
 {
  cp=_p_out+1; //pointer to next frame
  cp=cp&_ROLLMASK; //roll mask
  if(cp==cg) break; //if next frame not returned yet (buffer full)
  i=_ChSize-_n_out; //number of empty bytes in this frame
  if(i>l) i=l; //if we have less bytes then empty
  memcpy(&_wave_out[_p_out][_n_out], buf+d, i); //copy input data to frame
  l-=i; //remain data bytes
  d+=i; //processed bytes
  _n_out+=i; //empty bytes remains in current frame
  if(_n_out>=_ChSize) //if chunk full
  {
   waveOutPrepareHeader (_Out, _Hdr_out[_p_out], sizeof (WAVEHDR)); //prepare header
   waveOutWrite (_Out, _Hdr_out[_p_out], sizeof (WAVEHDR));    //pass it to wave output device
   _p_out=cp; //pointer to next chunk
   _n_out=0;  //all bytes in this frame are empty
  }
 } //while(l)
 d=d/2;
 return (d);  //returns bumber of accepted samples
}


//*****************************************************************************
//===================Wave task========================

DWORD WINAPI _WorkerThreadProc (void *Arg)
{
  MSG Msg; //system message
  //Set hight priority
  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);
  //work cicle: wait for Message
  while (GetMessage (&Msg, NULL, 0, 0) == TRUE)
  {
   switch (Msg.message) //process system message from HW wave devices
   {
    case MM_WIM_DATA: //wave input buffer compleet
    {
     WAVEHDR *Hdr = (WAVEHDR *)Msg.lParam;
     unsigned char bc;

     waveInUnprepareHeader (_In, Hdr, sizeof (*Hdr)); //unprepare returned wave header
     //p_in is a pointer to returned frame
     bc=_p_in+1; //computes pointer to next frame (normally in use now)
     bc&=_ROLLMASK;  //roll mask
     _p_in=bc;  //set it as a pointer to next frame will be returns
     bc++;     //computes pointer to next frame for passes to input device
     bc&=_ROLLMASK; //roll mask
     //returns header with next frame to input device
     Hdr->lpData=(char*)_wave_in[bc]; //attach next buffer to this header
     waveInPrepareHeader (_In, Hdr, sizeof (*Hdr)); //prepare header
     waveInAddBuffer (_In, Hdr, sizeof (*Hdr)); //back to input device
     break;
    }

    case MM_WOM_DONE:  //wave output buffer played
    {
     WAVEHDR *Hdr = (WAVEHDR *)Msg.lParam;
     unsigned char bc;

     waveOutUnprepareHeader (_Out, Hdr, sizeof (*Hdr)); //unprepare returned wave header
     bc=_g_out; //pointer to returned header
     bc++; //pointer to next frame: normally it was early passed to device
     _g_out=bc&_ROLLMASK; //roll mask
     break;
    }//if(f_out[g_out]==1)
   } //switch (Msg.message)
  }  //while (GetMessage (&Msg, NULL, 0, 0) == TRUE)
  UNREF (Arg);
  return 0;
}
//===============end of wave task=======================


//*****************************************************************************
//start audio
int _dlg_start(void)
{
   int Success=0;
   //up priority of main task WARNING: there are some problems of Win32 priority!!!
   //SetPriorityClass (GetCurrentProcess (), HIGH_PRIORITY_CLASS);
   //creates wave thread
   _WorkerThreadHandle = CreateThread (
              NULL,
              0,
              _WorkerThreadProc,
              NULL,
              0,
              &_WorkerThreadId
            );
   //open wave devices
   Success = _OpenDevices ();
/*   if (Success)
   {  //start audio input
    if (waveInStart(In) != MMSYSERR_NOERROR)
    {  //if no inputs
     CloseDevices ();
     IsSound=0;
     printf("Starting audio input failed\r\n");
    }
   } */
   return Success;
}

//start/stop audio input
int _soundrec(int on)
{
 
 printf("Soundrec %d\r\n", on);
 if(on!=_IsGo)
 {
  MMRESULT Res;

  if(on) Res = waveInStart(_In); //start audio input
  else Res = waveInStop(_In);   //stop audio input
  if(Res != MMSYSERR_NOERROR) _IsGo=0; //error
  else _IsGo=on;                       //set flag
  //printf("Rec=%d\r\n", IsGo);
 }
 return _IsGo;                        //return status
}