#include <fstream>
#include <cstdio>

#include "TFile.h"
#include "TClass.h"
#include "TKey.h"
#include "TSpectrum.h"
#include "TH1.h"
#include "TList.h"
#include "TH2.h"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TPaveText.h"
#include "TRandom.h"

using namespace std;


//#include <map>


TH2F* master = new TH2F("master","master",
                         200,0,200,8000,0,8000);

TH2F* masterall = new TH2F("masterall","masterall",
                            4000,0,4000,8000,0,8000);

TH2F* masterallcore = new TH2F("masterallcore","masterallcore",
                               1000,0,1000,8000,0,8000);

TList *FitRawCo(TH1 *hist,bool draw=false,bool is_core=false) { //,TChannel *chan=0) {
  TList *list = 0;
  if(!hist)
    return list;
  list = new TList;
  list->Add(hist);
  TH1 *bg = hist->ShowBackground(100,"");
  bg->SetNameTitle(Form("%s_bg",hist->GetName()),Form("%s_bg",hist->GetName()));
  list->Add(bg);
  TH1 *newhist = (TH1*)hist->Clone(Form("%s_bgsub",hist->GetName()));
  newhist->SetTitle(Form("%s_bgsub",hist->GetName()));
  newhist->Add(bg,-1);
  list->Add(newhist);

  if(is_core){
    hist->GetXaxis()->SetRangeUser(500,hist->GetXaxis()->GetXmax());
    newhist->GetXaxis()->SetRangeUser(500,newhist->GetXaxis()->GetXmax());
  } else {
    hist->GetXaxis()->SetRangeUser(2000,hist->GetXaxis()->GetXmax());
    newhist->GetXaxis()->SetRangeUser(2000,newhist->GetXaxis()->GetXmax());
  }
  TSpectrum s;
  s.Search(newhist,2.0,"",.3);

  if(s.GetNPeaks()!=2)
    printf("Raw Co fit failed.");

  double highx = s.GetPositionX()[0];
  double highy = s.GetPositionY()[0];
  double lowx  = s.GetPositionX()[1];
  double lowy  = s.GetPositionY()[1];

  if(lowx>highx) {
    double temp = lowx;
    lowx = highx; highx = temp;
    temp = lowy;
    lowy = highy; highy = temp;
  }

  TF1 cofit("co60","gaus(0)+gaus(3)");
  cofit.SetNpx(5000);

  cofit.SetParameter(0,lowy);
  cofit.SetParameter(1,lowx);
  cofit.SetParameter(2,2.0);
  cofit.SetParameter(3,highy);
  cofit.SetParameter(4,highx);
  cofit.SetParameter(5,2.0);

  newhist->Sumw2(true);

  newhist->Fit(&cofit);


  //TCanvas *c = new TCanvas;
  //c->Divide(1,2);
  //c->cd(1);
  //hist->Draw();
  //c->cd(2);
  //newhist->Draw();

  TGraphErrors *graph = new TGraphErrors(3);
  graph->SetNameTitle(Form("%s_graph",hist->GetName()),Form("%s_graph",hist->GetName()));

 ///////// 
  graph->SetPoint(2,0.0,0.0);
  graph->SetPointError(2,6,0);
 ///////// 
  
  graph->SetPoint(0,cofit.GetParameter(1),1173.1);
  graph->SetPointError(0,TMath::Sqrt(cofit.GetParameter(0)),0);
  graph->SetPoint(1,cofit.GetParameter(4),1332.5);
  graph->SetPointError(1,TMath::Sqrt(cofit.GetParameter(3)),0);
  //graph->Print();

  TF1 linfit("linfit","pol1(0)");
  linfit.SetParameter(0,0);
  linfit.SetParameter(1,(1332.5-1173.1)/(cofit.GetParameter(4)-cofit.GetParameter(1)));

  //new TCanvas;
  //graph->Draw("AC*");
  graph->Fit(&linfit);
  list->Add(graph);

  printf("i\nhist: %s\n",hist->GetName());
  printf("\t1173.1:\tFHWM(%.03f)\n",cofit.GetParameter(2)*2.35*linfit.GetParameter(1));
  printf("\t1332.5:\tFHWM(%.03f)\n\n",cofit.GetParameter(5)*2.35*linfit.GetParameter(1));

  if(draw) {
    TCanvas *c = new TCanvas;
    c->Divide(1,3);
    c->cd(1);
    hist->Draw();
    c->cd(2);
    newhist->Draw();
    c->cd(3);
    TPaveText *text=new TPaveText(0.2,0.7,.4,.9,"brNDC");
    text->AddText(Form("y = %.06fx + %.03f\n",linfit.GetParameter(1),linfit.GetParameter(0)));
    graph->Draw();
    text->Draw();
  }

  //if(chan) {
  //  chan->DestroyEnergyCoeff();
  //  chan->AddEnergyCoeff(linfit.GetParameter(0));
  //  chan->AddEnergyCoeff(linfit.GetParameter(1));
  //}
  return list;
}

/*
TH2 *MakeFWHMSummary(TH2 *hist,int hole,int cryl){
  if(!hist)
    return 0;
  TH2 *summary = new TH2F(Form("h%02i_%i_summary",hole,cryl),
                          Form("h%02i_%i_summary",hole,cryl),
                                40,0,40,1000,0,10);

  int MaxChan = hist->GetXaxis()->GetXmax();
  //std::cout << " Max : " << Max << std::endl;
  //TH1D *Hist_1D[40];
  //TH1D *Hist_1D;
  //Hist_1D = new TH1D[Max];
  for(int i=1; i<=MaxChan; i++){
    std::string HistName = Form("%s_%i",hist->GetName(),i);
    printf("%s\n",HistName.c_str());
    TList *fit_list = FitRawCo(hist->ProjectionY(HistName.c_str(),i,i));

    TH1 *h    = fit_list->FindObject(Form("%s_%i_bgsub",hist->GetName(),i));
    TGraph* g = fit_list->FindObject(Form("%s_%i_graph",hist->GetName(),i));

    TF1 *source = h->GetFunction("co60");
    TF1 *calib  = g->GetFunction("linfit");
    double fwhm = 0.000;
    if(source && calib) {
      fwhm = fabs(source->GetParameter(5)*2.35*calib->GetParameter(1));
    }
    summary->Fill(i,fwhm);
  }
  return summary;
}
*/

TList *MakeSummary(TH2 *hist,int qnum){
  if(!hist)
    return 0;
  TList *list = new TList;
  //if(hole>6)
  //  return list;

  std::ofstream ofile(Form("detmap_Q%d.txt",qnum));

  TH2 *summary = new TH2F(Form("q%i_summary",qnum),
                          Form("q%i_summary",qnum),
                                160,1,161,1000,0,10);
  list->Add(summary);

  TH2 *cal = new TH2F(Form("q%i_cal",qnum),
                      Form("q%i_cal",qnum),
                            160,1,161,8000,0,8000);
  list->Add(cal);

  int MaxChan = hist->GetXaxis()->GetXmax();
  for(int i=1; i<=MaxChan; i++){
    //if((i%10)!=0)
    //  continue;

    std::string HistName = Form("%s_%i",hist->GetName(),i);
    printf("%s\n",HistName.c_str());
    TH1D *raw = hist->ProjectionY(HistName.c_str(),i,i);

    TList *fit_list;
    if((i%10)==0) {
      fit_list = FitRawCo(raw,true, true);
    } else {
      fit_list = FitRawCo(raw,false, false);
    }
    list->Add(fit_list);

    TH1 *h    = (TH1*)fit_list->FindObject(Form("%s_%i_bgsub",hist->GetName(),i));
    TGraph* g = (TGraph*)fit_list->FindObject(Form("%s_%i_graph",hist->GetName(),i));

    TF1 *source = h->GetFunction("co60");
    //printf("num functions: %i",g->GetListOfFunctions()->GetSize());
    //printf("name functions: %s",g->GetListOfFunctions()->At(0)->GetName());
    //TF1 *calib  = g->GetFunction("linfit");
    TF1 *calib = (TF1*)g->FindObject("linfit");
    calib->SetName(Form("linfit_q%d_%04i",qnum,i));
    double fwhm = 0.000;
    if(source && calib) {
      fwhm = fabs(source->GetParameter(5)*2.35*calib->GetParameter(0));
    }
    summary->Fill(i,fwhm);

    calib->Print();
    double slope  = (double)calib->GetParameter(1);
    double offset = (double)calib->GetParameter(0);
    //printf("raw->GetXaxis()->GetNbins()  =  %i\n",raw->GetXaxis()->GetNbins());
    for(int x=1;x<=raw->GetXaxis()->GetNbins();x++) {
      double counts = raw->GetBinContent(x);
      for(int y=0;y<counts;y++) {
        double value  = (double)(raw->GetXaxis()->GetBinLowEdge(x)) + gRandom->Uniform(raw->GetXaxis()->GetBinWidth(x));
        double energy = slope*value + offset;

        //printf("%i %i %.02f %.02f\n",i,counts,value,energy);
        cal->Fill(i,energy);
        if(i%40==10) {
          master->Fill(qnum*4+i/40,energy);
        }
        if(i%10==0) {
          masterallcore->Fill(qnum*16 + i/10,energy);
        }
        masterall->Fill(qnum*160 + i,energy);
      }
    }
    ofile << offset << "\t" << slope << endl;


  }

  ofile.close();

  return list;
}



















//  //Address [ GetHole() | GetCrystal() | GetSegmentId()];
//  std::string channame = Form("Hole%02i_X%i_C%02i",);
//  //TChannel *chan = new TChannel()






/*
int function(const char *filename) {

  TFile *file = new TFile(filename);
  TList *outlist = new TList;
  //TIter fileiter(file->GetListOfKeys());
  for(int x=0;x<file->GetListOfKeys()->GetSize();x++) {
    TKey *key = (TKey*)(file->GetListOfKeys()->At(x));
    if(TClass::GetClass(key->GetClassName())->InheritsFrom("TH2")) {
       TH2 *h = (TH2*)key->ReadObj();
       string name = h->GetName();
       int hole = atoi(name.substr(4,2).c_str());
       int cryl = atoi(name.substr(7,1).c_str());
       printf("h->GetName() = %i\t%i\n",hole,cryl);
       outlist->Add(MakeFWHMSummary(h,hole,cryl));
    }
  }
  TFile *out = new TFile("myoutfile.root","recreate");
  outlist->Sort();
  outlist->Write();
  out->Close();
}
*/

int function(const char *filename) {

  TFile *file = new TFile(filename);
  TList *outlist = new TList;
  //TIter fileiter(file->GetListOfKeys());
  TFile *out = new TFile("Co_outfile.root","recreate");
  for(int x=0;x<file->GetListOfKeys()->GetSize();x++) {
    TKey *key = (TKey*)(file->GetListOfKeys()->At(x));
    if(TClass::GetClass(key->GetClassName())->InheritsFrom("TH2")) {
       TH2 *h = (TH2*)key->ReadObj();
       string name = h->GetName();
       //int hole = atoi(name.substr(4,2).c_str());
       //int cryl = atoi(name.substr(7,1).c_str());
       //if(hole>6)
       //  break;
       int qnum = atoi(name.substr(1,1).c_str());

        //printf("h->GetName() = %i\t%i\n",hole,cryl);
       outlist = MakeSummary(h,qnum);
       out->mkdir(Form("Q%i",qnum));
       out->cd(Form("Q%i",qnum));
       outlist->Sort();
       outlist->Write();
    }
    out->cd("/");
  }
  master->Write();
  masterallcore->Write();
  masterall->Write();
  out->Close();
}

#ifndef __CINT__

int main(int argc, char **argv) {
  function(argv[1]);
}

#endif
