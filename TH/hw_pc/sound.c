#include <string.h> //strncpy
#include <stdio.h> //printf
#include <stdlib.h> //abs

#include "audio_wave8.h"  //Headset voice grabbing/playing
#include "audio_wave48.h" //Line carrier grabbing/playing
#include "sound.h"

//flags
unsigned char is_VoiceIn=0; //Voice recording is active
unsigned char is_LineIn=0;  //Line recording is active
unsigned char is_VoiceOut=0; //Voice playing is active
unsigned char is_LineOut=0; //Line playing is active
//devices
char dev_VoiceIn[32];  //Voice recording device
char dev_VoiceOut[32]; //Voice playing device
char dev_LineIn[32];   //Line recording device
char dev_LineOut[32];  //Line playing device


//temporary raw buffers
short raw_mike_buf[180]; //exactly one frame of 8KHz samples for Mike rec
short raw_line_buf[180*6]; //exactly one frame of 48KHz samples for Line rec/play

//work buffers (accumulators)
short mike_buf[1440]; //accumulator for resampled Mike input
short line_buf[1440]; //accumulator for resampled Line input
short spk_buf[1440];  //accumulator for resumpled Spc output

//pointers
int mike_ptr=0;    //number of samples in mike_buf
int line_ptr=0;    //number of samples in line_buf
int spk_ptr=0;   //number of samples in spk_buf

//resampling coefficients and accumulators
double line_rk=6.0; //current resampling coeff for Line input
double mike_rk=1.333333; //current resampling coeff for Mike
double spk_rk=0.75;  //current resampling coeff for Spc
double line_rk_open=6.0; //average resampling coeff for Line
double mike_rk_open=1.333333; //average resampling coeff for Mike
double spk_rk_open=0.75;  //average resampling coeff for Spc

//overlaping values for stream resampling

double m_up_pos=0; //mike resamplers position
short m_left_sample=0; //mike resamplers base

double s_up_pos=1.0; //spc resamplers position
short s_left_sample=0;//spc resamplers base

double l_up_pos=0; //line input resamplers position
short l_left_sample=0; //line input resamplers base

double o_up_pos=0; //line output resamplers position
short o_left_sample=0; //line output resamplers base

 //tones for test only
static const short tone[12]={0, 8000, 13858, 16000, 13858, 8000, 0, -8000, -13858, -16000, -13858, -8000};
static const short tone48[12*6]={0,
 8000, 8000, 8000, 8000, 8000, 8000,
 13858, 13858, 13858, 13858, 13858, 13858,
 16000, 16000, 16000, 16000, 16000, 16000,
 13858, 13858, 13858, 13858, 13858, 13858,
 8000, 8000, 8000, 8000, 8000, 8000,
 0, 0, 0, 0, 0, 0,
 -8000, -8000, -8000, -8000, -8000, -8000,
 -13858, -13858, -13858, -13858, -13858, -13858,
 -16000,  -16000, -16000, -16000, -16000, -16000,
 -13858, -13858, -13858, -13858, -13858, -13858,
 -8000 -8000, -8000, -8000, -8000, -8000
};

//Synchronizing coefficients
#define MDM_FAST_CORR 0.00125 //coefficient for fast tune Line recording rate
#define MDM_SLOW_CORR 0.0000125 //coefficient for slow  tune Line recording rate

#define LINE_DELAY 6*360.0   //target fill of Line rplaying buffer
#define MIKE_FAST_CORR 0.0001 //coefficient for fast tune Mike recording rate
#define MIKE_SLOW_CORR 0.000001 //coefficient for slow tune Mike recording rate

#define SPK_DELAY 360.0    //target fill of speacker playing buffer
#define SPK_FAST_CORR 0.0001 //coefficient for fast tune Spc playing rate
#define SPK_SLOW_CORR 0.000001 //coefficient for slow tune Spc playing rate

//helpers prototypes
int o_resample(short* src, short* dest, int srclen);
int l_resample(short* src, short* dest, int srclen, double fstep);
int s_resample(short* src, short* dest, int srclen, double fstep);
int m_resample(short* src, short* dest, int srclen, double fstep);

//=====================Interface==============================

//first initialization, setup sound devices
//input is order number of device in list (Windows) or device name (Linux)
void init_Snd(char* voice_in, char* voice_out, char* line_in, char* line_out)
{
 //flags
is_VoiceIn=0; //Voice recording is active
is_LineIn=0;  //Line recording is active
is_VoiceOut=0; //Voice playing is active
is_LineOut=0; //Line playing is active

//pointersint mike_ptr=0;    //number of samples in mike_buf
line_ptr=0;    //number of samples in line_buf
spk_ptr=0;   //number of samples in spk_buf

//resampling coefficients and accumulators
line_rk=6.0; //current resampling coeff for Line input
mike_rk=1.333333; //current resampling coeff for Mike
spk_rk=0.75;  //current resampling coeff for Spc
line_rk_open=6.0; //average resampling coeff for Line
mike_rk_open=1.333333; //average resampling coeff for Mike
spk_rk_open=0.75;  //average resampling coeff for Spc

//overlaping values for stream resampling

m_up_pos=0; //mike resamplers position
m_left_sample=0; //mike resamplers base

s_up_pos=1.0; //spc resamplers position
s_left_sample=0;//spc resamplers base

l_up_pos=0; //line input resamplers position
l_left_sample=0; //line input resamplers base

o_up_pos=0; //line output resamplers position
o_left_sample=0; //line output resamplers base


 //audio devices
 strncpy(dev_VoiceIn, voice_in, sizeof(dev_VoiceIn)); //voice recoder
 strncpy(dev_VoiceOut, voice_out, sizeof(dev_VoiceOut)); //voice player
 strncpy(dev_LineIn, line_in, sizeof(dev_LineIn));  //line recoder
 strncpy(dev_LineOut, line_out, sizeof(dev_LineOut)); //line player
}

//start Voice recording/playing
int start_Voice(void)
{
 int ret;
 if((!is_VoiceIn)&&(!is_VoiceOut))  //if device not active
 {
  ret=soundinit(dev_VoiceIn, dev_VoiceOut); //start devices
  if(ret&2) is_VoiceIn=1;//activite and set flag by result
  if(ret&1) is_VoiceOut=1;
 }
 if(is_VoiceIn) soundrec(1); //run recording if device is active
 return ret; 
}

//stop voice recording/playing
void stop_Voice(void)
{
 if(is_VoiceIn) soundrec(0); //stop recording process
 if(is_VoiceIn || is_VoiceOut) //if device is active
  {
   soundterm(); //terminate sound thread
   is_VoiceIn=0; //clear flag of device active
   is_VoiceOut=0;
  }
}

//start line recording/playing
int start_Line(void)
{
 int ret;

 if((!is_LineIn)&&(!is_LineOut))   //if device not active
 {
  ret=_soundinit(dev_LineIn, dev_LineOut); //start devices
  if(ret&2) is_LineIn=1;  //activite and set flag by result
  if(ret&1) is_LineOut=1;
 }
 if(is_LineIn) _soundrec(1);   //run recording if device is active
 return ret; 
}

 //stop voice recording/playing
void stop_Line(void)
{
 if(is_LineIn) _soundrec(0); //stop recording process
 if(is_LineIn || is_LineOut) //if device is active
  {
   _soundterm(); //terminate sound thread
   is_LineIn=0; //clear flag of device active
   is_LineOut=0;
  }
}


//polling for recording 720 8KHz PCM samples from Line (resampled from 48KHz)
//returns 720 or 0 if no sufficient samples yet
int rec_Line(short* pcm)
{
 int i, k;
 if(!is_LineIn) return 0;  //check audio is enabled
 i=_soundgrab((char*) raw_line_buf, 6*180); //grab 48 KHz one frame from Line
 if(i<1) return 0; //check we have anything

 //resample to 8KHz rate with fine tuning samplerate
 i=l_resample(raw_line_buf, line_buf+line_ptr,i,line_rk);
 line_ptr+=i; //add new samples to buffer
 if(line_ptr<720) return 0; //check we have enought to output
 memcpy(pcm, line_buf, 720*sizeof(short));  //output
 line_ptr-=720; //samples left in buffer
 if(line_ptr) memcpy(line_buf, line_buf+720, line_ptr*sizeof(short)); //move tail
 return 720; //return number of output samples
}

//play 540 8KHz samples over Spc
void play_Spc(short* pcm)
{
 int i, j, k;
 double delay;
 
 if(!is_VoiceOut) return; //check audio is enabled
 delay=(double)(spk_ptr+getdelay()-SPK_DELAY); //get overal playing delay
 //if delay too big we must decrese spk_rk
 spk_rk=spk_rk_open+SPK_FAST_CORR*delay; //current rate value (fast tune)
 spk_rk_open+=SPK_SLOW_CORR*delay; //average rate value (slow tune)
 if(spk_rk_open>0.79) spk_rk_open=0.79; //restrict coefficients
 if(spk_rk_open<0.71) spk_rk_open=0.71;
 if(spk_rk>0.83) spk_rk=0.83;
 if(spk_rk<0.67) spk_rk=0.67;

 //spk_rk=1.000;
 //for(k=0;k<15*3; k++) memcpy(pcm+k*15, tone, 12*2);

 i=s_resample(pcm, spk_buf+spk_ptr, 540, spk_rk); //resample voice will be played
 spk_ptr+=i;  //add new samles
 j=0; //counter of played samples
 while(spk_ptr>=180) //check we have a frame
 {
 // for(k=0;k<15*3; k++) memcpy(spk_buf+j+k*15, tone, 12*2);
  i=soundplay(180, (unsigned char*)(spk_buf+j)); //play frame
  if(i<0)
  {
   soundplay(180, (unsigned char*)(spk_buf+j)); //if underrun play again
   soundplay(180, (unsigned char*)(spk_buf+j)); //if underrun play again
   //uprintf("play_Spc underrun!\r\n");
  }
  j+=i; //count played samples
  spk_ptr-=i; //unplayed samples left
 }

 //move tail to start of buffer
 if(spk_ptr && j) memcpy(spk_buf, spk_buf+j, spk_ptr*sizeof(short));

 //delay=getdelay();
 //printf("SPC Del=%d rk=%0.4f ork=%0.4f\r\n", (int)delay, spk_rk, spk_rk_open);
}

//========================================================================

//poll for recording 720 8KHz samles from Mike to pcm
//returns 540 or 0 if no sufficient samples yet
int rec_Mike(short* pcm)
{
 int i;
 double delay;
 
 if(!is_VoiceIn) return 0;  //check audio is enabled
 i=soundgrab((char*) raw_mike_buf, 180); //grab one frame of PCM speech
 if(i<1) return 0;

 i=m_resample(raw_mike_buf, mike_buf+mike_ptr,i,mike_rk);
 mike_ptr+=i;
 if(mike_ptr<540) return 0;
 memcpy(pcm, mike_buf, 540*sizeof(short));
 mike_ptr-=540;
 if(mike_ptr) memcpy(mike_buf, mike_buf+540, mike_ptr*sizeof(short));
 return 540;
}

//play 720 8KHz samples to Line (converting to 48KHz without rate tuning)
void play_Line(short* pcm)
{
 int i, j;
 double delay;

 if(!is_LineOut) return; //check audio is enabled

//!!!!!!! may be replace to Mike processing

//---------------------------------------------
 //if delay to big we must increase mike_rk (produce low samples)
 delay=(double)(_getdelay() - LINE_DELAY); //get difference of playing delay in samples
 mike_rk = MIKE_FAST_CORR * delay + mike_rk_open; //one-time correct Mike rate
 mike_rk_open += MIKE_SLOW_CORR * delay; //continuosly adjust Mike rate
 if(mike_rk_open>1.37) mike_rk_open=1.37; //restrict coefficients
 if(mike_rk_open<1.29) mike_rk_open=1.29;
 if(mike_rk>1.41) mike_rk=1.41;
 if(mike_rk<1.25) mike_rk=1.25;
//----------------------------------------------

 for(i=0;i<4;i++) //play 4 frames
 {
  o_resample(pcm+i*180,raw_line_buf,180); //upsample exactly by 6
  j=_soundplay(180*6, (unsigned char*)raw_line_buf); //play frame
  if(j<0)
  {
   _soundplay(180*6, (unsigned char*)raw_line_buf); //if underrun play again
   _soundplay(180*6, (unsigned char*)raw_line_buf); //if underrun play again
   //uprintf("play_Line underrun!\r\n");
  }
 }

 //printf("MIC Del=%d rk=%0.4f ork=%0.4f\r\n", (int)delay, mike_rk, mike_rk_open);

}

//tune sampling rate of Line input for lock phaze and compensate sampling frequency differency
short setLineRate(short value)
{
 short ret;
 #define R_K 10000.0
 line_rk=line_rk_open-MDM_FAST_CORR*value;//+line_rk_open; //fast tune current rate value
 line_rk_open-=MDM_SLOW_CORR*value; //slow tune (compensate sampling frequensy differency)
 if(line_rk_open>6.24) line_rk_open=6.24; //restrict coefficients
 if(line_rk_open<5.76) line_rk_open=5.76;
 if(line_rk>6.12) line_rk=6.12;
 if(line_rk<5.88) line_rk=5.88;
 //printf("V=%d, R=%0.4f O=%0.4f\r\n", value, line_rk, line_rk_open);
 ret=(short)(R_K*(6.0-line_rk_open));
 return ret;
}


//====================Helpers===========================

//------------------------------------------------------------------------------
//Mike resampler: fine tune 8KHz Mike signal before MELP analise
//to match fixed playing rate of modem Line output
//------------------------------------------------------------------------------
int m_resample(short* src, short* dest, int srclen, double fstep)
{

 int i, diff=0;
 short* sptr=src; //source
 short* dptr=dest; //destination
 //double fstep=rate/8000.0; //ratio between specified and default rates

 //process samples
 for(i=0;i<srclen;i++) //process melpe frame
 {
  diff = *sptr-m_left_sample; //computes difference beetwen current and basic samples
  while(m_up_pos <= 1.0) //while position not crosses a boundary
  {
   *dptr++ = m_left_sample + ((double)diff * m_up_pos); //set destination by basic, difference and position
    m_up_pos += fstep; //move position forward to fractional step
  }
  m_left_sample = *sptr++; //set current sample as a  basic
  m_up_pos = m_up_pos - 1.0; //move position back to one outputted sample
 }
 return dptr-dest;  //number of outputted samples
}




//------------------------------------------------------------------------------
//Line input resampler:  downrate 48K line input to 8K modem input
//with fine tuning depends demodulator rate adjust
//------------------------------------------------------------------------------
int l_resample(short* src, short* dest, int srcLen, double fstep)
{

 int i, diff=0;
 short* sptr=src; //source
 short* dptr=dest; //destination

 //process samples
 for(i=0;i<srcLen;i++) //process melpe frame
 {
  diff = *sptr-l_left_sample; //computes difference beetwen current and basic samples
  while(l_up_pos <= 1.0) //while position not crosses a boundary
  {
   *dptr++ = l_left_sample + ((double)diff * l_up_pos); //set destination by basic, difference and position
    l_up_pos += fstep; //move position forward to fractional step
  }
  l_left_sample = *sptr++; //set current sample as a  basic
  l_up_pos = l_up_pos - 1.0; //move position back to one outputted sample
 }
 return dptr-dest;  //number of outputted samples

}


//------------------------------------------------------------------------------
//Line output resampler:  uprate 8K modulator output to 48K Line output
//with no tuning
//------------------------------------------------------------------------------
int o_resample(short* src, short* dest, int srclen)
{
 int i, j;
 short* sptr=src; //source
 short* dptr=dest; //destination
 double diff;

 //double fstep=rate/8000.0; //ratio between specified and default rates

 //process samples
 for(i=0;i<srclen;i++) //process melpe frame
 {
  diff = (double)(*sptr - o_left_sample)/6; //computes difference beetwen current and basic samples
  for(j=0;j<6;j++) *dptr++ = o_left_sample + (short)(diff*j); //increase rate by 6
  o_left_sample = *sptr++; //set current sample as a  basic
 }

 return dptr-dest;  //number of outputted samples
}


//------------------------------------------------------------------------------
//Speacker resampler: fine tune Speaker signal
//for match demodulator output
//------------------------------------------------------------------------------
static int s_resample(short* src, short* dest, int srcLen, double fstep)
{

 int i, diff=0;
 short* sptr=src; //source
 short* dptr=dest; //destination

 //process samples
 for(i=0;i<srcLen;i++) //process melpe frame
 {
  diff = *sptr-s_left_sample; //computes difference beetwen current and basic samples
  while(s_up_pos <= 1.0) //while position not crosses a boundary
  {
   *dptr++ = s_left_sample + ((double)diff * s_up_pos); //set destination by basic, difference and position
    s_up_pos += fstep; //move position forward to fractional step
  }
  s_left_sample = *sptr++; //set current sample as a  basic
  s_up_pos = s_up_pos - 1.0; //move position back to one outputted sample
 }
 return dptr-dest;  //number of outputted samples

}

//get average signal level  in Log range 0-15 of 540 PCM samples frame
unsigned char get_level(short* m)
{
 short i;
 int s=0;

 for(i=0;i<540;i++) s+=abs(m[i]); //summ frame energy
 s>>=9; //fit to 16 bits
 i=0;
 while(s>>=1) i++; //i=Log2(s)
 return (unsigned char)i; //returns log signal level
}

