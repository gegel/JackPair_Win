//---------------------------------------------------------------------------
#include <string.h> //strncpy
#include <stdio.h> //printf
#include <stdlib.h> //abs

#include "melpe_pc/melpe.h" //voice codec
#include "hw_pc/sound.h" //audio grabbing and playing
#include "hw_pc/comport.h" //COM port
#include "crp_pc/cntr.h" //sync and crypto
#include "crp_pc/crc32.h" //crc32
#include "mdm_jp/modem.h" //modem
#include"main.h"

#pragma hdrstop

//---------------------------------------------------------------------------
//interface
volatile unsigned char Q_LED=0; //lock indicator
volatile unsigned char M_LED=0; //mode indicator = work
volatile unsigned char V_LED=0; //vad indicator
volatile unsigned char K_LED=0; //having key indicator



volatile unsigned char Q_LEVEL=0; //quality level (BER on RX, VAD on TX)
volatile unsigned char S_LEVEL=0; //signal level (LineIn on RX, MikeIn on TX)
volatile unsigned char M_LEVEL=0; //Mike level
volatile unsigned char B_BTN=0;  //SecPTT (1-on)
volatile unsigned char DIR_BTN=0; //TX direct mode: no switch to
volatile unsigned char DUP_BTN=0; //RX duplex
volatile unsigned char F_PORT=0; //COM port opened


//Other settings
#define SPEECH_FRAME 540 //short PCM samples in speech frame
#define MODEM_FRAME 720 //short PCM samples in modem frame
#define START_SYNC 8 //number of sync pcket sends on start of transmitt before voice
#define SYNCNT 50  //near 15 for 1 sec trace after sync lost (1' max)


//beeper
unsigned int beep=0;  //line beep sequence value

//status flags
signed char key=0; //flag of having their key (0-no key, 1-have key untill button will be presed, -1 -  have key)
unsigned char snc; //flag of their carrier detected (0- no carrier, 1-carrier detected and synchronized)
unsigned char btn; //flag of button pressed (0-button released, 1-pressed)
unsigned char vad; //flag of our speech active (0-silency, 1-speech)

unsigned char work=0;  //current device status: 0-IKE (after run), 1-WORK mode (after key agreeded)
unsigned char vad_cnt=0; //counter of sync will be senden in start of transmit
unsigned char silency=0; //flag of play silency over speaker
unsigned char block=0; //flag of turn off speacker amplifier
short syncnt=0; //counter of frames will be counted after sync lost (for quick resync on fading)

volatile unsigned int k=0; //secret authentication code

#define WORKLEN 850

short workbuf[WORKLEN];
short outbuf[WORKLEN];

//data buffers
unsigned char bits[16]; //data block (9 data bytes + flags)
unsigned char sbts[90]; //soft bits of received data bytes (LLR)
signed char lbuf[256]={0}; //log output text buffer


volatile short adc_l_value=0; //divider for nominal samle rate 8000KHz of recording from Line (tuned by demodulator)
short l_rate=0; //average Line input sampling frequency difference
int i; //general int
unsigned char c; //general char
short* sp_ptr; //pointer to speech buffer
short* md_ptr; //pointer to modem buffer
unsigned char ptr; //cashing of volatile Start_Encoding/Start_Decoding

unsigned char pls=0; //pulse modem instead PSK
unsigned char avd=0; //anti-vad trick enable


#pragma argsused

//Main DSP processing loop func
//returns 0 on iddle or 1 on any processing
short th(void)
{
        short job=0; //any processing flag

        //######################### MIKE TO LINE ##########################

         i=rec_Mike(workbuf);  //grab audio from Mike

         if(i) //process 540 PCM speech samles was grabbed
         {
          //scan PTT button
          if(btn!=B_BTN) //if button state is change
	  {
           btn=B_BTN; //set new button state
           SetRTS(B_BTN); //set RTS=0 on Transmitt
           vad_cnt=START_SYNC;  //set counter of sync frames
           block=0; //clear Speacker block flag
           silency=0; //clear silency flag
           if(btn) melpe_i();   //!!!!!!!!!!!!!!!!!!!!!!
          }
          //set pointers
          sp_ptr=workbuf; //speech input
          md_ptr=outbuf; //line output
          M_LEVEL=get_level(sp_ptr); //get Mike level
          
//========================IKE mode processing====================
	  if(!work) //check current state id IKE mode
          {
           vad=0;  //clear VAD flag
           txkey(bits);  //generate key data, set control flag to bits[9], returns control flag (0-ctrl, 1-data)
           make_pkt(bits); //encrypt bits for key data or set control data, returns void
           if(B_BTN) //if button press
           {
//-----------------------Button press on IKE---------------
            if(!DIR_BTN)
            {
             if(pls) Modulate_p(bits, md_ptr); //modulate data to line output
             else Modulate_b(bits, md_ptr);
            }
            else speech2line(sp_ptr, md_ptr);  //or send speech to Line output in direct mode
            if(key>0) key=-1; //reverse keyalarm flag after button was pressed
           }
//-----------------------Button release on IKE---------------
           else //button released
           {
            if((key>0) && ptr) Tone(md_ptr, MODEM_FRAME/12); //send tone to line output every even frame if keyalarm flag set
            else memset(md_ptr, 0, 2*MODEM_FRAME); //or set silency
           }
          } //end of processing recorded speech frame in IKE mode

//========================WORK mode processing====================
	  else  //WORK mode
          {
 //-----------------------Button press on work---------------
           if(B_BTN) //process voice to data only if button pressed
           {
            if(!vad_cnt) //check for sync counter before transmit
            {  //start timeout was elapsed
             vad=melpe_a(bits, sp_ptr); //encode speech frame, voice flag in bits[9], returns voice flag (1-voice, 0-silency)
             if(DUP_BTN) silency=0; //clear silency flag in duplex mode:  receiving will be enabling
            }
            else //on timeout of transmit startup
            {
             vad=0; //clear vad flag
             bits[10]=0; //clear vad flag in array
             vad_cnt--; //count sync: 4-beep, 3 and 2 - silency, 1,0-block
             silency=1; //set silency flag
             if(vad_cnt==4) beep=1; //beep for user can start talk
             if(vad_cnt<2) block=1; //beep on end of sync timeout after transmission start
            }
            make_pkt(bits); //encrypt bits for voice data or set control data, returns void
            if(!DIR_BTN)
            {
             if(pls) Modulate_p(bits, md_ptr); //modulate data to line output
             else Modulate_b(bits, md_ptr);
            }
            else speech2line(sp_ptr, md_ptr); //or send speech to Line output in direct mode
           }
 //-----------------------Button release on WORK---------------          
           else //button not press: pass data from mike to line, increment outgoing counter
           {
            bits[10]=0; //clear voice flag
            make_pkt(bits); //make control packet (increment counter of transmitted frames permanently in work mode)
            memset(md_ptr, 0, 2*MODEM_FRAME); // set silency
            vad=0;	//clearerr voice flag
           }
          }	 //end of processing recoreded speech frame in WORK mode

          play_Line(outbuf); //play 540 modem or speech PCM samples to Line Output
          if(vad) V_LED=1; else V_LED=0; //set VAD state
         }


 //################# LINE TO SPC ############################



         i=rec_Line(workbuf);  //grab PCM modem samples from Line Input
         if(i)  //if frame samples grabbed
         {
          //set pointers
          md_ptr=workbuf;
          sp_ptr=outbuf;
          S_LEVEL=get_level(md_ptr); //Set line PCM  level
          if(k && key) K_LED=1; else K_LED=0; //update "Having key" status

       //=====================IKE mode processing====================
	  if(!work)
          {
           M_LED=0; //clear Mode state during Initial key exchange (IKE)
           //demodulate with soft bits
           for(i=0;i<MODEM_FRAME;i++) md_ptr[i]>>=4; //decrease PCM level to 12 bit suitable for modem

           if(B_BTN&&(!DUP_BTN)) //PTT is hold in simplex mode
           {
            snc=0;  //disable processing of received frame during TX on simplex mode
            adc_l_value=0; //clear Line inpu rate tuning value
            Q_LEVEL=0; //clear nodem qulity level
           }
           else
           {
            if(pls) adc_l_value=Demodulate_p(md_ptr, bits, sbts); //demodulate line input, returns value for fine tuning recording sample rate
            else adc_l_value=Demodulate_b(md_ptr, bits, sbts);
            snc=bits[11]&0x40; //get flag of carrier detected
            //compute BER status from modem internal statistic
            i=bits[12]; //8 MSB of BER
            i<<=4; //put to place
            i+=(bits[13]&0x0F); //4 LSB of BER
            i/=32; //fit to 4 bits
            if(i>15) i=15; //saturate
            i=15-i; //revert BER to Quality
            Q_LEVEL=i; //set baseband quality state
           }

           l_rate=setLineRate(adc_l_value);//fine tune recording sample rate
           if(B_BTN) bits[13]|=0x80; //set button flag
           if(snc) Q_LED=1; else Q_LED=0; //set state of carrier detected

           if(snc) //if carrier detected
           {
            check_pkt(bits); //check for control packets, set ctrl flag to MSB of bits[11], returns 1-voice, 0 control
            if(syncnt<SYNCNT) syncnt+=2; //increment good packets counter to limit value
           }
           else if(syncnt) //if no carrier but trace still active
           {
            check_pkt(bits); //check packet despite no carrier (mostly for increment packet counter in cntr)
            syncnt--; //decrement good packet counter down to 0 (trace will be disabled)
           }
           i=ike_ber(bits, l_rate, lbuf); //check packet for ber, output log, returns 0- we have their key otherwise 1
           if((!i) && (!key)) key=1; //set flag for alarmed other side: we already have their key
           //check carrier detected
           if(snc) //carrier detected
           {	 //process received data
            work=rxkey(bits, sbts); //process received key data, returns current mode
           }

           //resample line signal to speaker
           for(i=0;i<MODEM_FRAME;i++) md_ptr[i]<<=4; //set back PCM 16 bits level
           line2speech(md_ptr, sp_ptr); //play modem signal to Speacker
           if(B_BTN&&(!DUP_BTN)) memset(sp_ptr, 0, sizeof(short)*SPEECH_FRAME); //or set silency on transmitt in simplex mode
          } //end of received modem frame processing in IKE mode

          //----------------WORK mode processing-----------------

          else
          {
           M_LED=1; //set work mode state
           for(i=0;i<MODEM_FRAME;i++) md_ptr[i]>>=4; //decrease PCM level to 12 bit suitable for modem

           if(B_BTN && (!DUP_BTN))
           {
            snc=0;
            adc_l_value=0;
            Q_LEVEL=0;
           }
           else
           {
            if(pls) adc_l_value=Demodulate_p(md_ptr, bits, 0); //demodulate line input, returns value for fine tuning recording sample rate
            else adc_l_value=Demodulate_b(md_ptr, bits, 0);
            snc=bits[11]&0x40; //get flag of carrier detected
            //compute BER status from modem internal statistic
            i=bits[12]; //8 MSB of BER
            i<<=4; //put to place
            i+=(bits[13]&0x0F); //4 LSB of BER
            i/=32; //fit to 4 bits
            if(i>15) i=15; //saturate
            i=15-i; //revert BER to Quality
            Q_LEVEL=i; //set baseband quality state
           }

           l_rate=setLineRate(adc_l_value); //fine tune recording sample rate

           if(B_BTN) bits[13]|=0x80; ///set button flag
           if(snc) Q_LED=1; else Q_LED=0; //set state of carrier detected
           if(vad) bits[13]|=0x40; else bits[13]&=(~0x40);// bits[12]+=2;	//set vad flag

           if(snc) //carrier detected
           {  //increment packets only in good sync because later we can't rewind counter back but can move forward
            if(syncnt<SYNCNT) syncnt++; //increment good packets counter to limit value
            i=check_pkt(bits); //check for control packets, set ctrl flag to MSB of bits[11], returns 1-voice, 0 control
            if(i) melpe_s(sp_ptr, bits); //decode received 9 bytes data to 540 samples into outbuf+540
            else memset(sp_ptr, 0, SPEECH_FRAME*sizeof(short)); //or set silency if control packet was received
           }
           else //no carrier
           {
            if(syncnt) //if no carrier but counter is positive
            {
             check_pkt(bits); //check packet despite no carrier (mostly for increment packet counter in cntr)
             syncnt--; //decrement good packet counter down to 0
            }
            for(i=0;i<MODEM_FRAME;i++) md_ptr[i]<<=4;  //set back PCM 16 bits level
            line2speech(md_ptr, sp_ptr); //convert 720 *KHz line samples to 540 6KHz speaker samples
           }

           //set silency on transmitting in simplex mode
           if(B_BTN && (!DUP_BTN)) memset(sp_ptr, 0, SPEECH_FRAME*sizeof(short));

           if(beep) //beep to speaker in work mode
           {
            Tone(sp_ptr, SPEECH_FRAME/12); //pley 90mS tone
            beep>>=1; //shift tone value to next
           }
           else if(silency) memset(sp_ptr, 0, SPEECH_FRAME*sizeof(short)); //set silency before start of TX
           work_ber(bits, l_rate, lbuf); //check packet for ber, output log
          } //end of received modem frame processing in work mode

          lbuf[255]=0;  //terminate string is modem statistic
          uprintf("%s", lbuf); //output over COM port

          //check hardware inputs over COM port          c=GetCTSDSR();  //get state of HW control over UART          if(c&1) B_BTN|=0x02; else B_BTN&=(~0x02); //PTT (0-active)          if(c&2) DIR_BTN|=0x02; else DIR_BTN&=(~0x02); //Direct transparent mode (0-active)
		  
          play_Spc(outbuf); //play voice or carrier over speaker
         } //end of process recorded mike
         
	return job; //returns flag of some job was processed
}


//---------------------------------------------------------------------------
//                               USER INTERFACE
//---------------------------------------------------------------------------

//change PTT state by user
void set_ptt(unsigned char val)
{
 if(val) B_BTN|=0x01; else B_BTN&=(~0x01);
}

//change Duplex mode by user
void set_duplex(unsigned char val)
{
 DUP_BTN=val;
 SetDTR(DUP_BTN); //set DTR=0 on Duplex (not disable  receiving during transmitting
}

//change Direct mode by User
void set_direct(unsigned char val)
{
 if(val) DIR_BTN|=0x01; else DIR_BTN&=(~0x01);
}

//change modem type by user
void set_modem(unsigned char val)
{
 pls=val;
}

//change manti-vad trick by user
void set_avad(unsigned char val)
{
 avd=val;
 setavad(avd);
}

//get currnet state by user
unsigned int get_status(void)
{
 unsigned int val=0;

 //set flags
 if(M_LED) val|=1; //work mode flag
 if(K_LED) val|=2; //having key flag
 if(Q_LED) val|=4; //carrier lock flag
 if(V_LED) val|=8; //VAD active flag

 if(B_BTN) val|=16; //PTT active flag
 if(DIR_BTN) val|=32; //Direct mode flag
 if(DUP_BTN) val|=64; //Duplex mode flag
 if(F_PORT) val|=128; //Port opened flag


 val<<=8;
 val|=M_LEVEL; //Mike input level
 val<<=8;
 val|=S_LEVEL; //Line input level
 val<<=8;
 val|=Q_LEVEL; //Modem input carrier quality level

 return val;
}

//stop DSP processing by user
void stop_th(void)
{
 ClosePort(); //close COM port
 stop_Voice(); //stop Headset audio device
 stop_Line(); //stop Line audio device
}

//init DSP processing by user
void start_th(char* com, char* sec, char* voice_in, char* voice_out, char* line_in, char* line_out)
{

   int i;
   char c;

   Q_LED=0; //lock indicator
   M_LED=0; //mode indicator = work
   V_LED=0; //vad indicator
   K_LED=0; //having key indicator

   Q_LEVEL=0; //quality level (BER on RX, VAD on TX)
   S_LEVEL=0; //signal level (LineIn on RX, MikeIn on TX)
   M_LEVEL=0; //Mike level
   B_BTN=0;  //PTT (1-on)
   F_PORT=0; //COM port (1-opened)


   DIR_BTN=0; //TX direct mode: no switch to
   DUP_BTN=0; //RX duplex


   key=0; //flag of having their key (0-no key, 1-have key untill button will be presed, -1 -  have key)
   snc=0; //flag of their carrier detected (0- no carrier, 1-carrier detected and synchronized)
   btn=0; //flag of button pressed (0-button released, 1-pressed)
   vad=0; //flag of our speech active (0-silency, 1-speech)

   work=0;  //current device status: 0-IKE (after run), 1-WORK mode (after key agreeded)
   vad_cnt=0; //counter of sync will be senden in start of transmit
   silency=0; //flag of play silency over speaker
   block=0; //flag of turn off speacker amplifier
   syncnt=0; //counter of frames will be

  //self test crypto engine by test vector
    init_ctrl(); //init control module
    if(!testcrp()) while(1){}; //check crypto math validity
    setrand_pc((unsigned char*)workbuf, sizeof(workbuf)); //set SID for PRNG with entropy collected by TRNG

    //apply common secret string
    if(sec[0]=='#') k=atoi(sec+1);
    else if(sec[0]) k=crc32((unsigned char*)sec, strlen(sec)); //compute authentication key
    else k=0; //empty string
    setkey(k); //setup new session keypair or goto work mode for empty key

    //get COM port number from dev name 'COMxx'
    i=0;
    c=com[3]; //CM port number '0' - '9'
    if((c>'0')&&(c<='9'))
    {
     i=c-'0'; //0-9
     c=com[4]; //check number >9, get low digit
     if((c>'0')&&(c<='9')) 
     {
      i*=10; //set previous digit as a hight
      i=i+c-'0'; //add low digit 0-9
     }
     else if(c) i=0; //COM not used
    }
    if(!InitPort(i)) //open COM port
    {
     F_PORT=1;  //set flag by result
     SetRTS(B_BTN); //set RTS out to 1 (inactive)
     SetDTR(DUP_BTN); //set DTR out to 1 (inactive)
    }
    
    melpe_i(); //initialize melp codec
    init_Snd(voice_in, voice_out, line_in, line_out); //init audio devices
    setLineRate(0); //set Line recording rate tune value to 0
    start_Voice(); //start Headset audio devices
    start_Line();  //start Line audio devices
}


