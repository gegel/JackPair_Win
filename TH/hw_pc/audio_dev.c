
#include <basetsd.h>
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "audio_dev.h"
//Transcode cp866(ru) to latin
static void r2tru(char* in, char* out)
{
 static char tbl_win[64]={
     -32, -31, -30, -29, -28, -27, -26, -25,
     -24, -23, -22, -21, -20, -19, -18, -17,
     -16, -15, -14, -13, -12, -11, -10, -9,
     -8, -7, -4, -5, -6, -3, -2, -1,
     -64, -63, -62, -61, -60, -59, -58, -57,
     -56, -55, -54, -53, -52, -51, -50, -49,
     -48, -47, -46, -45, -44, -43, -42, -41,
     -40, -39, -36, -37, -38, -35, -34, -33
 };
 static char* tbl_trn="abvgdezzijklmnoprstufhccss'y'ejjABVGDEZZIIKLMNOPRSTUFHCCSS'I'EJJ";
 int i,j;
 char*p=out;

 for(i=0;i<1+(int)strlen(in); i++)
 {
  if(in[i]>=0) (*p++)=in[i];
  else
  {
   for(j=0; j<64; j++)
   {
    if(in[i]==tbl_win[j])
    {
     (*p++)=tbl_trn[j];
     if((j==6)||(j==23)||(j==24)||(j==38)||(j==55)||(j==56)) (*p++)='h'; //zh, ch, sh
     if((j==25)||(j==57)) (*p++)='c'; //sc
     if((j==30)||(j==62)) (*p++)='u'; //ju
     if((j==31)||(j==63)) (*p++)='a'; //sc
     break;
    }
    if(j==64) (*p++)='?';
   }
  }
 }
}


//=============================Level 0==================


//*****************************************************************************
//discower wave input and output in system and print devices list
//
int get_audio_dev(int devtype, int devnum, char* dev)
{
 WAVEOUTCAPS DevCapsOut;
 WAVEINCAPS DevCapsIn;
 int NumWaveDevs;
 int i;

 if(devtype) //Output devices: devtype=1
 {
  //get number of wave output devices in system
  NumWaveDevs = waveOutGetNumDevs ();
  dev[0]=0;
  if(devnum<=NumWaveDevs)
  {
   waveOutGetDevCaps (devnum, &DevCapsOut, sizeof (DevCapsOut)); //get devices name
   r2tru(DevCapsOut.szPname, dev);
  }
 }
 else //input devices: devtype=0
 {
  //get number of wave input devices in system
  NumWaveDevs = waveInGetNumDevs ();
  dev[0]=0;
  if(devnum<=NumWaveDevs)
  {
   waveInGetDevCaps (devnum, &DevCapsIn, sizeof (DevCapsIn)); //get devices name
   r2tru(DevCapsIn.szPname, dev);
  }
 }
 
 return NumWaveDevs;
}
