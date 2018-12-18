//---------------------------------------------------------------------------

#include <vcl.h>      
#pragma hdrstop

#include "comtest.h"
#include "comport.h"
#include "TH/main.h"
#include "audio_dev.h"

#include "Unit1.h"

#define CONFIG_FILE "jp_config.ini"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

TStringList* List = new TStringList();  //List for load error codes from text file
AnsiString S; //string

char d_comport[8]={0};
char d_mike[8]={0};
char d_spk[8]={0};
char d_linein[8]={0};
char d_lineout[8]={0};
char d_key[32]={0};


signed char jmp[22]={0,}; //1 - jumper set
signed char mout[4]={0,0,1,1};//group outs on/off

const signed char zin[22]={0,2,2,4,4,6,6, 1,1,3,3,5,5,7, 0,1,2,3,4,5,6,7}; //commected inputs
const signed char zout[22]={0,0,2,2,3,3,1, 6,4,4,7,7,5,5, 6,0,4,2,7,3,5,1}; //comnected outputs


//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//Create Thread for  DSP audio processing
//---------------------------------------------------------------------------

    class MyThread: public TThread   //derive our thread class from TThread
    {
       public:
          __fastcall MyThread( int aParam ); //optionally pass parameter to our class
     
       protected:
          void __fastcall Execute(); //thread body
     
       private:
          int param;
    };
//---------------------------------------------------------------------------
    __fastcall MyThread::MyThread( int aParam ): TThread( true )  //constructor
    {
       param = aParam; //save parameter
       Resume(); //run thread
    }
//---------------------------------------------------------------------------
    void __fastcall MyThread::Execute() //DSP task
    {
     int jb;

     start_th(d_comport, d_key, d_mike, d_spk, d_linein, d_lineout); //init DSP
     while(!Terminated) //loop
     {
      jb=th(); //process DSP
      if(!jb) Sleep(0); //suspend thread  if no DSP jobs
     }
     stop_th(); //deinit DSP
    }

 //---------------------------------------------------------------------------

 MyThread *Thread = NULL; //create pointer to our thread (not run now)



 int setJumper(int jp)
 {
  int i,k=0;
  int ji, jo;
  if((jp<0)||(jp>21)) return 0;
  jmp[jp]=0; //clear jumper

  ji=zin[jp]; //input connected to jumper will be set
  jo=zout[jp]; //output connected to jumper will be set
  for(i=0;i<22;i++)
  {
   if(i==jp) continue; //skip himself
   if(!jmp[i]) continue; //skip open jumpers
   if(ji==zin[i]) break; //conflict in input pin
   if(jo==zout[i]) break; //conflict in output pin
  }
  if(i==22) //no conflicts
  {
   jmp[jp]=1; //set jumper closed
   k=1; //set flag OK
  }

  return k;
 }


 //=======================================================================
 //                        GUI
 //=======================================================================

//Press PTT button
void __fastcall TForm1::BitBtnPTTMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
 if((!Thread)||(Button!=mbLeft)) return;
 //BitBtnPTT->Font->Color=clGreen;
 set_ptt(1);
}

//Release PTT button
//---------------------------------------------------------------------------
void __fastcall TForm1::BitBtnPTTMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
 if((!Thread)||(Button!=mbLeft)) return;
 //BitBtnPTT->Font->Color=clBlack;
 set_ptt(0);
}
//---------------------------------------------------------------------------
//Hold PTT button (for full duplex or continuous transmitting)
void __fastcall TForm1::Hold1Click(TObject *Sender)
{
 if(!Thread) return; //check Thread is run
 //BitBtnPTT->Font->Color=clGreen;
 set_ptt(1);
}
//---------------------------------------------------------------------------
//Release PTT button after hold
void __fastcall TForm1::Release1Click(TObject *Sender)
{
 if(!Thread) return; //check Thread is run
 //BitBtnPTT->Font->Color=clBlack;
 set_ptt(0);
}
//---------------------------------------------------------------------------
//set duplex mode for Thread
void __fastcall TForm1::CheckBoxDupClick(TObject *Sender)
{
 if(!Thread) return; //check Thread is run
 if(CheckBoxDup->Checked) set_duplex(1); else set_duplex(0);
}
//---------------------------------------------------------------------------
//set direct mode for Thread
void __fastcall TForm1::CheckBoxDirClick(TObject *Sender)
{
 if(!Thread) return; //check Thread is run
 if(CheckBoxDir->Checked) set_direct(1); else set_direct(0);
}
//---------------------------------------------------------------------------

//Start/stop DSP Thread
void __fastcall TForm1::CheckBoxRunClick(TObject *Sender)
{
 if(CheckBoxRun->Checked && (!Thread))  //start thread
 {
  int i;
  AnsiString S;
   //clear GUI
  ShapeLock->Brush->Color=clSilver;
  ShapeReady->Brush->Color=clSilver;
  ShapeKey->Brush->Color=clSilver;
  ShapeVAD->Brush->Color=clSilver;
  CheckBoxDir->Checked=false;
  CheckBoxDup->Checked=false;
  ProgressBarM->Position=0;
  ProgressBarL->Position=0;
  ProgressBarQ->Position=0;
  BitBtnPTT->Font->Color=clBlack;

  //apply user settings to values
  strncpy(d_key, EditKey->Text.c_str(), sizeof(d_key));
  i=ComboBoxMike->ItemIndex;
  if(i<1) d_mike[0]=0; else strncpy(d_mike, IntToStr(i-1).c_str(), sizeof(d_mike));
  i=ComboBoxSpk->ItemIndex;
  if(i<1) d_spk[0]=0; else strncpy(d_spk, IntToStr(i-1).c_str(), sizeof(d_spk));
  i=ComboBoxLineIn->ItemIndex;
  if(i<1) d_linein[0]=0; else strncpy(d_linein, IntToStr(i-1).c_str(), sizeof(d_linein));
  i=ComboBoxLineOut->ItemIndex;
  if(i<1) d_lineout[0]=0; else strncpy(d_lineout, IntToStr(i-1).c_str(), sizeof(d_lineout));
  i=ComboBoxCom->ItemIndex;
  S=ComboBoxCom->Items->operator [](ComboBoxCom->ItemIndex);
  if(i<1) d_comport[0]=0; else strncpy(d_comport, S.c_str(), sizeof(d_comport));

  //run DSP thread
  Thread = new MyThread(0); //run thread for COM port reading
  if(CheckBoxAvad->Checked) set_avad(1); else set_avad(0);
  if(RadioButtonPls->Checked) set_modem(1); else set_modem(0);
 }

 if((!CheckBoxRun->Checked) && Thread) //stop thread
 {
  Thread->Terminate();  //stop thread
  Thread=NULL;

  //clear GUI
  ShapeLock->Brush->Color=clSilver;
  ShapeReady->Brush->Color=clSilver;
  ShapeKey->Brush->Color=clSilver;
  ShapeVAD->Brush->Color=clSilver;
  CheckBoxDir->Checked=false;
  CheckBoxDup->Checked=false;
  ProgressBarM->Position=0;
  ProgressBarL->Position=0;
  ProgressBarQ->Position=0;
  BitBtnPTT->Font->Color=clBlack;
 }

}
//---------------------------------------------------------------------------
//On timer: get status from Thread and apply to GUI
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
 unsigned int st=0;

 if(Thread) st=get_status();  //get status if thread run
 //apply current state to GUI
 ProgressBarQ->Position=st&0xFF;
 st>>=8;
 ProgressBarL->Position=st&0xFF;
 st>>=8;
 ProgressBarM->Position=st&0xFF;
 st>>=8;
 if(st&4) ShapeLock->Brush->Color=ShapeLock->Pen->Color; else ShapeLock->Brush->Color=clSilver;
 if(st&1) ShapeReady->Brush->Color=ShapeReady->Pen->Color; else ShapeReady->Brush->Color=clSilver;
 if(st&2) ShapeKey->Brush->Color=ShapeKey->Pen->Color; else ShapeKey->Brush->Color=clSilver;
 if(st&8) ShapeVAD->Brush->Color=ShapeVAD->Pen->Color; else ShapeVAD->Brush->Color=clSilver;

 if(Thread) CheckBoxRun->Font->Color=clGreen; else CheckBoxRun->Font->Color=clBlack;
 if(st&16) BitBtnPTT->Font->Color=clGreen; else BitBtnPTT->Font->Color=clBlack;
 if(st&32) CheckBoxDir->Font->Color=clGreen; else CheckBoxDir->Font->Color=clBlack;
 if(st&64) CheckBoxDup->Font->Color=clGreen; else CheckBoxDup->Font->Color=clBlack;
 if(st&128) LabelHW->Visible=true; else LabelHW->Visible=false;
 
 }
//---------------------------------------------------------------------------

//update hardware configuration, load and apply config
void __fastcall TForm1::ButtonUpdate0Click(TObject *Sender)
{
 //update COM
 int portlist[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
 char dev[256];
 int i;
 int n=0;



 //--------------update COM ports-------------

 ComboBoxCom->Items->Clear();
 ComboBoxCom->Items->Add("Not selected");
 ComboBoxCom->ItemIndex=0;
 
 SearchCom(portlist); //get list using QueryDosDevice method


 for(i=0;i<16;i++)
 {
  if(portlist[i])
  {
   n++;
   ComboBoxCom->Items->Add("COM"+IntToStr(portlist[i]));
  }
 }

 //--------------update Audio inputs-------------

 ComboBoxMike->Items->Clear();
 ComboBoxMike->Items->Add("Not selected");
 ComboBoxMike->ItemIndex=0;


 ComboBoxLineIn->Items->Clear();
 ComboBoxLineIn->Items->Add("Not selected");
 ComboBoxLineIn->ItemIndex=0;

 n=get_audio_dev(0, -1, dev);
 if(n)
 {
  dev[255]=0;
  ComboBoxMike->Items->Add("0: "+AnsiString(dev));
  ComboBoxLineIn->Items->Add("0: "+AnsiString(dev));
  for(i=0;i<n;i++)
  {
   get_audio_dev(0, i, dev);
   dev[255]=0;
   ComboBoxMike->Items->Add(IntToStr(i+1)+": "+AnsiString(dev));
   ComboBoxLineIn->Items->Add(IntToStr(i+1)+": "+AnsiString(dev));
  }
 }

 //--------------update Audio outputs-------------
 ComboBoxSpk->Items->Clear();
 ComboBoxSpk->Items->Add("Not selected");
 ComboBoxSpk->ItemIndex=0;

 ComboBoxLineOut->Items->Clear();
 ComboBoxLineOut->Items->Add("Not selected");
 ComboBoxLineOut->ItemIndex=0;

 n=get_audio_dev(1, -1, dev);
 if(n)
 {
  dev[255]=0;
  ComboBoxSpk->Items->Add("0: "+AnsiString(dev));
  ComboBoxLineOut->Items->Add("0: "+AnsiString(dev));
  for(i=0;i<n;i++)
  {
   get_audio_dev(1, i, dev);
   dev[255]=0;
   ComboBoxSpk->Items->Add(IntToStr(i+1)+": "+AnsiString(dev));
   ComboBoxLineOut->Items->Add(IntToStr(i+1)+": "+AnsiString(dev));
  }
 }

 //load config from file
 List->Clear();
  try
  {
   List->LoadFromFile( CONFIG_FILE );
  }
  catch (...)
  {
   List->Clear();
  }

 //apply Mike
 if(List->Count>0) S=List->Strings[0]; else S="0";
 i=StrToIntDef(S, 0);
 if(i<0) i=0;
 if(i>=ComboBoxMike->Items->Count) i=0;
 if(!ComboBoxMike->Items->Count) i=-1;
 ComboBoxMike->ItemIndex=i;
 n=i;
 //apply Line In
 if(List->Count>1) S=List->Strings[1]; else S="0";
 i=StrToIntDef(S, 0);
 if((i==n)&&(i>0))
 {
  ShowMessage("Line input cannot be the same of Mike and will be disabled");
  i=-1;
 }
 if(i<0) i=0;
 if(i>=ComboBoxLineIn->Items->Count) i=0;
 if(!ComboBoxLineIn->Items->Count) i=-1;
 ComboBoxLineIn->ItemIndex=i;

 //apply Speaker
 if(List->Count>2) S=List->Strings[2]; else S="0";
 i=StrToIntDef(S, 0);
 if(i<0) i=0;
 if(i>=ComboBoxSpk->Items->Count) i=0;
 if(!ComboBoxSpk->Items->Count) i=-1;
 ComboBoxSpk->ItemIndex=i;
 n=i;

 //apply Line Out
 if(List->Count>3) S=List->Strings[3]; else S="0";
 i=StrToIntDef(S, 0);
 if((i==n)&&(i>0))
 {
  ShowMessage("Line output cannot be the same of Speaker and will be disabled");
  i=-1;
 }
 if(i<0) i=0;
 if(i>=ComboBoxLineOut->Items->Count) i=0;
 if(!ComboBoxLineOut->Items->Count) i=-1;
 ComboBoxLineOut->ItemIndex=i;

 //apply COMport
 if(List->Count>4) S=List->Strings[4]; else S="0";
 i=StrToIntDef(S, 0);
 if(i<0) i=-0;
 if(i>=ComboBoxCom->Items->Count) i=0;
 if(!ComboBoxCom->Items->Count) i=-1;
 ComboBoxCom->ItemIndex=i;

 //apply secret
 if(List->Count>5) S=List->Strings[5]; else S="";
 EditKey->Text=S;

 //apply jumpers
 if(List->Count>6) S=List->Strings[6]; else S="";
 memset(dev, 0, 22);
 strncpy(dev, S.c_str(), 24);
 if(dev[0]=='1') if(setJumper(0)) cb0->Checked=true; else cb0->Checked=false;
 if(dev[1]=='1') if(setJumper(1)) cb1->Checked=true; else cb1->Checked=false;
 if(dev[2]=='1') if(setJumper(2)) cb2->Checked=true; else cb2->Checked=false;
 if(dev[3]=='1') if(setJumper(3)) cb3->Checked=true; else cb3->Checked=false;
 if(dev[4]=='1') if(setJumper(4)) cb4->Checked=true; else cb4->Checked=false;
 if(dev[5]=='1') if(setJumper(5)) cb5->Checked=true; else cb5->Checked=false;
 if(dev[6]=='1') if(setJumper(6)) cb6->Checked=true; else cb6->Checked=false;
 if(dev[7]=='1') if(setJumper(7)) cb7->Checked=true; else cb7->Checked=false;
 if(dev[8]=='1') if(setJumper(8)) cb8->Checked=true; else cb8->Checked=false;
 if(dev[9]=='1') if(setJumper(9)) cb9->Checked=true; else cb9->Checked=false;
 if(dev[10]=='1') if(setJumper(10)) cb10->Checked=true; else cb10->Checked=false;
 if(dev[11]=='1') if(setJumper(11)) cb11->Checked=true; else cb11->Checked=false;
 if(dev[12]=='1') if(setJumper(12)) cb12->Checked=true; else cb12->Checked=false;
 if(dev[13]=='1') if(setJumper(13)) cb13->Checked=true; else cb13->Checked=false;
 if(dev[14]=='1') if(setJumper(14)) cb14->Checked=true; else cb14->Checked=false;
 if(dev[15]=='1') if(setJumper(15)) cb15->Checked=true; else cb15->Checked=false;
 if(dev[16]=='1') if(setJumper(16)) cb16->Checked=true; else cb16->Checked=false;
 if(dev[17]=='1') if(setJumper(17)) cb17->Checked=true; else cb17->Checked=false;
 if(dev[18]=='1') if(setJumper(18)) cb18->Checked=true; else cb18->Checked=false;
 if(dev[19]=='1') if(setJumper(19)) cb19->Checked=true; else cb19->Checked=false;
 if(dev[20]=='1') if(setJumper(20)) cb20->Checked=true; else cb20->Checked=false;
 if(dev[21]=='1') if(setJumper(21)) cb21->Checked=true; else cb21->Checked=false;



}
//---------------------------------------------------------------------------
 //save config
void __fastcall TForm1::ButtonSave0Click(TObject *Sender)
{       //add config to List and save to file
 int i;
 AnsiString S="";
 for(i=0;i<22;i++) if(jmp[i]) S=S+"1"; else S=S+"0";  //jumper positions

 List->Clear();
 List->Add(IntToStr(ComboBoxMike->ItemIndex));
 List->Add(IntToStr(ComboBoxLineIn->ItemIndex));
 List->Add(IntToStr(ComboBoxSpk->ItemIndex));
 List->Add(IntToStr(ComboBoxLineOut->ItemIndex));
 List->Add(IntToStr(ComboBoxCom->ItemIndex));
 List->Add(EditKey->Text);
 List->Add(S);
 List->SaveToFile( CONFIG_FILE );
 ShowMessage("Configuration saved");

}
//---------------------------------------------------------------------------
//On change Mike selecting
void __fastcall TForm1::ComboBoxMikeChange(TObject *Sender)
{
  if((ComboBoxMike->ItemIndex==ComboBoxLineIn->ItemIndex)&&(ComboBoxMike->ItemIndex>0))
  {
   ShowMessage("Mike cannot be the same of Line input");
   ComboBoxMike->ItemIndex = 0;
  }
}
//---------------------------------------------------------------------------
 //On change LineIn  selecting
void __fastcall TForm1::ComboBoxLineInChange(TObject *Sender)
{
 if((ComboBoxMike->ItemIndex==ComboBoxLineIn->ItemIndex)&&(ComboBoxMike->ItemIndex>0))
  {
   ShowMessage("Line input cannot be the same of Mike");
   ComboBoxLineIn->ItemIndex = 0;
  }
}
//---------------------------------------------------------------------------
 //On change  Speaker  selecting
void __fastcall TForm1::ComboBoxSpkChange(TObject *Sender)
{
 if((ComboBoxSpk->ItemIndex==ComboBoxLineOut->ItemIndex)&&(ComboBoxSpk->ItemIndex>0))
  {
   ShowMessage("Speaker cannot be the same of Line output");
   ComboBoxSpk->ItemIndex = 0;
  }
}
//---------------------------------------------------------------------------
 //On Change LineOut   selecting
void __fastcall TForm1::ComboBoxLineOutChange(TObject *Sender)
{
  if((ComboBoxSpk->ItemIndex==ComboBoxLineOut->ItemIndex)&&(ComboBoxSpk->ItemIndex>0))
  {
   ShowMessage("Line output cannot be the same of Speaker");
   ComboBoxLineOut->ItemIndex = 0;
  }
}
//---------------------------------------------------------------------------
//On start application
void __fastcall TForm1::FormCreate(TObject *Sender)
{
 ButtonUpdate0Click(NULL); //load config
}
//---------------------------------------------------------------------------
//On exit application
void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose)
{
 if(Thread) //stop thread if runs
 {
  Thread->Terminate();
  Thread=NULL;
 }
}
//---------------------------------------------------------------------------



void __fastcall TForm1::CheckBoxAvadClick(TObject *Sender)
{

 if(CheckBoxAvad->Checked)
 {
  CheckBoxAvad->Font->Color=clGreen;
  if(Thread) set_avad(1);
 }
 else
 {
  CheckBoxAvad->Font->Color=clBlack;
  if(Thread) set_avad(0);
 }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::RadioButtonPlsClick(TObject *Sender)
{
 RadioButtonPls->Font->Color=clGreen;
 RadioButtonPsk->Font->Color=clBlack;
 if(Thread) set_modem(1);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::RadioButtonPskClick(TObject *Sender)
{
 RadioButtonPls->Font->Color=clBlack;
 RadioButtonPsk->Font->Color=clGreen;
 if(Thread) set_modem(0);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::cb0Click(TObject *Sender)
{
 int j;

 j=StrToInt(((TCheckBox*)Sender)->Name.SubString(3,2));
 if(((TCheckBox*)Sender)->Checked)
 {
  if(!setJumper(j)) ((TCheckBox*)Sender)->Checked=false;
 }
 else jmp[j]=0;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ButtonCodeClick(TObject *Sender)
{
 int i,j,k;
 unsigned int b=1;
 unsigned int d=0xFFFFFF;



 //itteration 1
 mout[0]=0; mout[1]=0; mout[2]=1; mout[3]=1; //initial output groups setting
 for(i=0;i<22;i++) //scan all possible positions
 {
  if(!jmp[i]) continue; //skip open jumpers
  k=zout[i]; //connected output
  k>>=1; //output group
  if(mout[k]) continue; //skip if groups set to 1
  k=zin[i]; //connected input
  k=7-k; //bit number
  k+=16; //shift for this iteration
  d&=~(b<<k); //clear corresponds bit of key
 }



 //itteration 2
 mout[1]^=1; mout[2]^=1;  //initial output groups setting
 for(i=0;i<22;i++) //scan all possible positions
 {
  if(!jmp[i]) continue; //skip open jumpers
  k=zout[i]; //connected output
  k>>=1; //output group
  if(mout[k]) continue; //skip if groups set to 1
  k=zin[i]; //connected input
  k=7-k; //bit number
  k+=8; //shift for this iteration
  d&=~(b<<k); //clear corresponds bit of key
 }


 //itteration 3
 mout[2]^=1; mout[3]^=1;  //initial output groups setting
 for(i=0;i<22;i++) //scan all possible positions
 {
  if(!jmp[i]) continue; //skip open jumpers
  k=zout[i]; //connected output
  k>>=1; //output group
  if(mout[k]) continue; //skip if groups set to 1
  k=zin[i]; //connected input
  k=7-k; //bit number
  k+=0; //shift for this iteration
  d&=~(b<<k); //clear corresponds bit of key
 }


 EditKey->Text="#"+IntToStr(d);

}
//---------------------------------------------------------------------------

