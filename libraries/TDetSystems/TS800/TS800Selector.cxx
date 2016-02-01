
#include <TROOT.h>

#include <TS800Selector.h>
#include <GCanvas.h>

TS800Selector::TS800Selector() { 
  pid  = 0; 
  s800 = 0;
  fChain = 0;
} 


void TS800Selector::Init(TTree *tree)      { 
  fChain = (TChain*)tree;
  fChain->SetBranchAddress("TS800",&s800);
  printf("Init has been called.\n"); 
}

void TS800Selector::Begin(TTree *tree) {
  Init(tree);
  pid = new TH2I("s800pid","s800pid",2000,0,2000,4000,0,8000);
}


bool TS800Selector::Notify() { 
  printf("Notify has been called.\n");  
  return true;
}

Bool_t TS800Selector::Process(Long64_t entry) { 
  printf("process called.\n");
  fChain->GetTree()->GetEntry(entry);
  pid->Fill(s800->GetCorrTOF_OBJTAC(),s800->GetIonChamber().GetSum()); 
  return true;
}

Bool_t TS800Selector::ProcessCut(Long64_t entry) {
  //printf("process cut called.\n");
  return true;
}

void TS800Selector::ProcessFill(Long64_t entry) {
  //printf("process fill called.\n");
  fChain->GetTree()->GetEntry(entry);
  pid->Fill(s800->GetCorrTOF_OBJTAC(),s800->GetIonChamber().GetSum()); 
}

void TS800Selector::Terminate()       { 
  if(!gPad || !gPad->IsEditable()) {
    gROOT->MakeDefCanvas();  
  } else {
    gPad->GetCanvas()->Clear();
  }
  pid->Draw("colz");

}










