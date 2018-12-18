//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>    
#include <ExtCtrls.hpp>
#include <TabNotBk.hpp>
#include <Menus.hpp>
#include <Buttons.hpp>
#include <Graphics.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
        TEdit *EditKey;
        TLabel *Label1;
        TButton *Button1;
        TComboBox *ComboBoxCom;
        TLabel *Label2;
        TButton *Button2;
        TComboBox *ComboBoxLineIn;
        TComboBox *ComboBoxLineOut;
        TLabel *Label4;
        TButton *Button4;
        TButton *Button5;
        TComboBox *ComboBoxMike;
        TComboBox *ComboBoxSpk;
        TButton *Button6;
        TButton *Button7;
        TProgressBar *ProgressBarL;
        TProgressBar *ProgressBarQ;
        TLabel *Label5;
        TLabel *Label7;
        TProgressBar *ProgressBarM;
        TLabel *Label6;
        TCheckBox *CheckBoxRun;
        TCheckBox *CheckBoxDup;
        TCheckBox *CheckBoxDir;
        TShape *ShapeKey;
        TShape *ShapeReady;
        TShape *ShapeVAD;
        TShape *ShapeLock;
        TLabel *Label9;
        TLabel *Label10;
        TLabel *Label11;
        TLabel *Label12;
        TPopupMenu *PopupMenu1;
        TMenuItem *Hold1;
        TMenuItem *Release1;
        TBitBtn *BitBtnPTT;
        TTimer *Timer1;
        TButton *ButtonUpdate1;
        TButton *ButtonSave1;
        TLabel *Label8;
        TLabel *Label13;
        TButton *ButtonUpdate2;
        TButton *ButtonSave2;
        TLabel *Label3;
        TButton *ButtonUpdate3;
        TButton *ButtonSave3;
        TButton *ButtonUpdate0;
        TButton *ButtonSave0;
        TMemo *Memo1;
        TLabel *LabelHW;
        TCheckBox *CheckBoxAvad;
        TRadioButton *RadioButtonPsk;
        TRadioButton *RadioButtonPls;
        TLabel *Label14;
        TTabbedNotebook *TabbedNotebook;
        TStaticText *StaticText1;
        TStaticText *StaticText2;
        TStaticText *StaticText3;
        TStaticText *StaticText4;
        TStaticText *StaticText5;
        TStaticText *StaticText6;
        TStaticText *StaticText7;
        TStaticText *StaticText8;
        TStaticText *StaticText9;
        TStaticText *StaticText10;
        TStaticText *StaticText11;
        TStaticText *StaticText12;
        TStaticText *StaticText13;
        TStaticText *StaticText14;
        TStaticText *StaticText15;
        TStaticText *StaticText16;
        TCheckBox *cb0;
        TCheckBox *cb1;
        TCheckBox *cb2;
        TCheckBox *cb3;
        TCheckBox *cb4;
        TCheckBox *cb5;
        TCheckBox *cb6;
        TCheckBox *cb7;
        TCheckBox *cb8;
        TCheckBox *cb9;
        TCheckBox *cb10;
        TCheckBox *cb11;
        TCheckBox *cb12;
        TCheckBox *cb13;
        TCheckBox *cb14;
        TCheckBox *cb15;
        TCheckBox *cb16;
        TCheckBox *cb17;
        TCheckBox *cb18;
        TCheckBox *cb19;
        TCheckBox *cb20;
        TCheckBox *cb21;
        TButton *ButtonCode;
        TLabel *Label15;
        void __fastcall BitBtnPTTMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
        void __fastcall BitBtnPTTMouseUp(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
        void __fastcall Hold1Click(TObject *Sender);
        void __fastcall Release1Click(TObject *Sender);
        void __fastcall CheckBoxDupClick(TObject *Sender);
        void __fastcall CheckBoxDirClick(TObject *Sender);
        void __fastcall CheckBoxRunClick(TObject *Sender);
        void __fastcall Timer1Timer(TObject *Sender);
        void __fastcall ButtonUpdate0Click(TObject *Sender);
        void __fastcall ButtonSave0Click(TObject *Sender);
        void __fastcall ComboBoxMikeChange(TObject *Sender);
        void __fastcall ComboBoxLineInChange(TObject *Sender);
        void __fastcall ComboBoxSpkChange(TObject *Sender);
        void __fastcall ComboBoxLineOutChange(TObject *Sender);
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
        void __fastcall CheckBoxAvadClick(TObject *Sender);
        void __fastcall RadioButtonPlsClick(TObject *Sender);
        void __fastcall RadioButtonPskClick(TObject *Sender);
        void __fastcall cb0Click(TObject *Sender);
        void __fastcall ButtonCodeClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
 