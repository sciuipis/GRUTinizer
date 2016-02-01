
#include <TROOT.h>

#include <TS800Selector.h>
#include <GCanvas.h>

TS800Selector::TS800Selector() { 
  pid  = 0; 
  s800 = 0;
} 


void TS800Selector::Begin(TTree *tree) {
  tree->SetBranchAddress("TS800",&s800);
  pid = new TH2I("s800pid","s800pid",2000,0,2000,4000,0,8000);
}

void TS800Selector::Init(TTree*)      { printf("Init has been called.\n"); }

bool TS800Selector::Notify()          { printf("Notify has been called.\n");  return true;}

bool TS800Selector::Process(Long64_t) { 
  pid->Fill(s800->GetCorrTOF_OBJTAC(),s800->GetIonChamber().GetSum()); 
  return true;
}



void TS800Selector::Terminate()       { 
  if(!gPad || !gPad->IsEditable()) {
    gROOT->MakeDefCanvas();  
  } else {
    gPad->GetCanvas()->Clear();
  }
  pid->Draw("colz");

}










