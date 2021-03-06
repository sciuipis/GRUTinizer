#include <Globals.h>
#include "TGRUTint.h"

#include <fstream>
#include <iostream>

#include "Getline.h"
#include "TClass.h"
#include "TGFileDialog.h"
#include "TFile.h"
#include "TH1.h"
#include "TInterpreter.h"
#include "TMode3.h"
#include "TPython.h"
#include "TROOT.h"
#include "TString.h"
#include "TTree.h"
#include "TUnixSystem.h"

#include "GRootGuiFactory.h"
#include "ProgramPath.h"
#include "TDetectorEnv.h"
#include "TGRUTOptions.h"
#include "TGRUTUtilities.h"

//extern "C" G__value G__getitem(const char* item);
//#include "FastAllocString.h"
//char* G__valuemonitor(G__value buf, G__FastAllocString& temp);

extern void PopupLogo(bool);
extern void WaitLogo();

ClassImp(TGRUTint)

TGRUTint *TGRUTint::fTGRUTint = NULL;
TEnv    *TGRUTint::fGRUTEnv  = NULL;
TObject *gResponse = NULL;


TGRUTint *TGRUTint::instance(int argc,char** argv, void *options, int numOptions, bool noLogo, const char *appClassName) {
  if(!fTGRUTint) {
    fTGRUTint = new TGRUTint(argc,argv,options,numOptions,true,appClassName);
    fTGRUTint->Init();
  }
  return fTGRUTint;
}


TGRUTint::TGRUTint(int argc, char **argv,void *options, Int_t numOptions, Bool_t noLogo,const char *appClassName)
  :TRint(appClassName, &argc, argv, options, numOptions,noLogo),
   fCommandServer(NULL), fGuiTimer(NULL), fCommandTimer(NULL), fRootFilesOpened(0),
   fIsTabComplete(false), fPipeline(NULL) {

  fGRUTEnv = gEnv;
  GetSignalHandler()->Remove();
  TGRUTInterruptHandler *ih = new TGRUTInterruptHandler();
  ih->Add();
  //TH1::SetDefaultSumw2();

  SetPrompt("GRizer [%d] ");
  TGRUTOptions::Get(argc, argv);
}


TGRUTint::~TGRUTint() {
  if(fCommandTimer){
    delete fCommandTimer;
  }
  if(fGuiTimer){
    delete fGuiTimer;
  }
}


void TGRUTint::Init() {
  if(TGRUTOptions::Get()->ShouldExit()){
    Terminate();
    return;
  }

  std::string grutpath = getenv("GRUTSYS");

  gInterpreter->AddIncludePath(Form("%s/include",grutpath.c_str()));

  if(TGRUTOptions::Get()->ShowLogo()){
    PopupLogo(false);
    WaitLogo();
  }

  ApplyOptions();
}

/*********************************/

bool TGRUTInterruptHandler::Notify() {
  static int timespressed  = 0;
  timespressed++;
  if(timespressed>3) {
    printf("\n" DRED BG_WHITE  "   Quit hitting me... Force exiting. :( " RESET_COLOR "\n");
    exit(1);
  }
  printf("\n" DRED BG_WHITE  "   Control-c was pressed.   " RESET_COLOR "\n");
  TGRUTint::instance()->Terminate();
  return true;
}

void TGRUTint::ApplyOptions() {
  TGRUTOptions* opt = TGRUTOptions::Get();

  if(!false) { //this can be change to something like, if(!ClassicRoot)
     LoadGRootGraphics();
  }

  TDetectorEnv::Get(opt->DetectorEnvironment().c_str());

  SetupPipeline();

  if(!opt->StartGUI()) {
    for(auto& filename : opt->RootInputFiles()){
      OpenRootFile(filename);
    }
  }

  if(opt->StartGUI()){
    std::string script_filename = program_path() + "/../util/grut-view.py";
    std::ifstream script(script_filename);
    std::string script_text((std::istreambuf_iterator<char>(script)),
                            std::istreambuf_iterator<char>());
    TPython::Exec(script_text.c_str());


    std::string default_gui_config =  gEnv->GetValue("GRUT.GuiSetup","");
    if(default_gui_config.length() &&
       !opt->GuiSaveSetFiles().size()){
      TPython::Exec(Form("window.LoadGuiFile(\"%s\")",default_gui_config.c_str()));
    }

    for(auto& filename : opt->RootInputFiles()){
      TPython::Exec(Form("window.LoadRootFile(\"%s\")",filename.c_str()));
    }
    for(auto& filename : opt->GuiSaveSetFiles()){
      TPython::Exec(Form("window.LoadGuiFile(\"%s\")",filename.c_str()));
    }
    fGuiTimer = new TTimer("TPython::Exec(\"update()\");",100);
    fGuiTimer->TurnOn();
  }

  for(auto& filename : opt->MacroInputFiles()){
    RunMacroFile(filename);
  }

  if(fPipeline->CanStart()){
    fPipeline->Start();
  }

  if(opt->ExitAfterSorting()){
    while(!fPipeline->IsFinished()){
      std::cout << "\r" << fPipeline->Status() << std::flush;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << std::endl;
    fPipeline->Write();

    this->Terminate();
  } else if(opt->CommandServer()) {
    fCommandServer = new TGRUTServer(opt->CommandPort());
    fCommandServer->Start();
    fCommandTimer = new TTimer("",10);
    fCommandTimer->TurnOn();
  }
}

void TGRUTint::SetupPipeline() {
  TGRUTOptions* opt = TGRUTOptions::Get();

  fPipeline = new TPipeline;
  for(auto& filename : opt->RawInputFiles()){
    fPipeline->AddRawDataFile(filename);
  }

  if(opt->OutputFile().length()){
    fPipeline->SetOutputRootFile(opt->OutputFile());
  }

  if(opt->CompiledHistogramFile().length()) {
    fPipeline->SetHistogramLibrary(opt->CompiledHistogramFile());
  }

  if(opt->SortTree()){
    for(auto& filename : opt->RootInputFiles()){
      fPipeline->AddInputRootFile(filename);
    }
  }

  if(opt->InputRing().length()){
    fPipeline->SetInputRing(opt->InputRing());
  }

  fPipeline->SetIsOnline(opt->IsOnline());

  fPipeline->Initialize();
}

void TGRUTint::DefaultFunction(){
  static int i = 0;
  std::cout << "I am a default function." << std::endl;
  std::cout << "I have been called " << i++ << " times before" << std::endl;
}

TFile* TGRUTint::OpenRootFile(const std::string& filename, TChain *chain){
  if(!file_exists(filename)){
    std::cerr << "File \"" << filename << "\" does not exist" << std::endl;
    return NULL;
  }

  const char* command = Form("TFile *_file%i = new TFile(\"%s\",\"read\")",
                             fRootFilesOpened, filename.c_str());
  ProcessLine(command);

  TFile *file = (TFile*)gROOT->GetListOfFiles()->FindObject(filename.c_str());
  if(file){
    std::cout << "\tfile " << file->GetName() << " opened as _file" << fRootFilesOpened << std::endl;
  }

  fRootFilesOpened++;
  return file;
}

void TGRUTint::RunMacroFile(const std::string& filename){
  if(!file_exists(filename)){
    std::cerr << "File \"" << filename << "\" does not exist" << std::endl;
    return;
  }

  const char* command = Form(".x %s", filename.c_str());
  ProcessLine(command);
}

void TGRUTint::HandleFile(const std::string& filename){
  if(!file_exists(filename)){
    std::cerr << "File \"" << filename << "\" does not exist" << std::endl;
    return;
  }

  TGRUTOptions* opt = TGRUTOptions::Get();

  kFileType filetype = opt->DetermineFileType(filename);
  switch(filetype){
    case NSCL_EVT:
    case GRETINA_MODE2:
    case GRETINA_MODE3:
      //TODO, make this part work.
      break;

    case ROOT_DATA:
      OpenRootFile(filename);
      break;

    case ROOT_MACRO:
      RunMacroFile(filename);
      break;

    default:
      std::cerr << "Unknown file type for " << filename << std::endl;
  }
}

Int_t TGRUTint::TabCompletionHook(char* buf, int* pLoc, std::ostream& out){
  fIsTabComplete = true;
  auto result = TRint::TabCompletionHook(buf, pLoc, out);
  fIsTabComplete = false;
  return result;
}


Long_t TGRUTint::ProcessLine(const char* line, Bool_t sync,Int_t *error) {
  // If you print while fIsTabComplete is true, you will break tab complete.
  // Any diagnostic print statements should be done after this if statement.
  if(fIsTabComplete){
    return TRint::ProcessLine(line, true, error);
  }
  TString sline(line);
  if(!sline.Length()){
    return 0;
  }
  sline.ReplaceAll("TCanvas","GCanvas");

  if(sline.Contains("for") ||
     sline.Contains("while") ||
     sline.Contains("if") ||
     sline.Contains("{") ||
     sline.Contains("TPython")){
    return TRint::ProcessLine(sline.Data(), true, error);
  }

  if(!sline.CompareTo("clear")) {
    return TRint::ProcessLine(".! clear", true);
  }

  Ssiz_t index;

  if((index = sline.Index(';')) != kNPOS){
    TString first = sline(0,index);
    first = first.Strip(TString::kTrailing);
    long res = ProcessLine(first.Data(),     sync, error);
    if(sline.Length() > index){
      TString second = sline(index+1, sline.Length()-index).Data();
      second = second.Strip(TString::kLeading);
      res += ProcessLine(second.Data(), sync, error);
    }
    return res;
  }


  long result =  TRint::ProcessLine(sline.Data(),true,error);
  return result;
}


TString TGRUTint::ReverseObjectSearch(TString &input) {

  int end = 0;
  int start = input.Length();
  TString output;
  for (int i = start; i > 0; i--) {
    if (input[i] == '.') {
      return TString(input(0,i));
    }
    if (input[i] == '>' && i > 0 && input[i-1] == '-') {
      return TString(input(0,i-1));
    }
  }
  return TString("");
};

void TGRUTint::Terminate(Int_t status){
  if(fCommandServer) {
    fCommandServer->Stop();
    fCommandServer->Delete();
    fCommandServer = NULL;
  }

  if(TGRUTOptions::Get()->StartGUI()){
    TPython::Exec("on_close()");
  }

  fPipeline->Stop();

  //Be polite when you leave.
  printf(DMAGENTA "\nbye,bye\t" DCYAN "%s" RESET_COLOR  "\n",getlogin());

  if((clock()%60) == 0){
    printf("DING!");
    fflush(stdout);
    gSystem->Sleep(500);
    printf("\r              \r");
    fflush(stdout);
  }

  TRint::Terminate(status);
}

void TGRUTint::LoadGRootGraphics() {
  GRootGuiFactory::Init();
}


void TGRUTint::OpenFileDialog() {
  TGFileInfo file_info;
  const char *filetypes[] = { "ROOT File", "*.root",
                              "Macro File", "*.C",
                              "GRETINA data file","*.dat",
                              "NSCL data","*.evt",
                              "Calibrtaion file","*.cal",
                              0,0 };
  file_info.fFileTypes = filetypes;
  new TGFileDialog(gClient->GetRoot(),0,kFDOpen,&file_info);
  if(file_info.fFilename)  {
    HandleFile(file_info.fFilename);
  }
}

TObject* TGRUTint::DelayedProcessLine(std::string message){
  std::lock_guard<std::mutex> any_command_lock(fCommandWaitingMutex);

  {
    std::lock_guard<std::mutex> lock(fCommandListMutex);
    fLinesToProcess.push(message);
  }

  TTimer::SingleShot(0,"TGRUTint",this,"DelayedProcessLine_ProcessItem()");

  std::unique_lock<std::mutex> lock(fResultListMutex);
  while(fCommandResults.empty()){
    fNewResult.wait(lock);
  }

  TObject* result = fCommandResults.front();
  fCommandResults.pop();
  return result;
}

void TGRUTint::DelayedProcessLine_ProcessItem(){
  std::string message;
  {
    std::lock_guard<std::mutex> lock(fCommandListMutex);
    if(fLinesToProcess.empty()){
      return;
    }
    message = fLinesToProcess.front();
    fLinesToProcess.pop();
  }

  std::cout << "Received remote command \"" << message << "\"" << std::endl;
  this->ProcessLine(message.c_str());
  Getlinem(EGetLineMode::kInit,((TRint*)gApplication)->GetPrompt());

  {
    std::lock_guard<std::mutex> lock(fResultListMutex);
    fCommandResults.push(gResponse);
    gResponse = NULL;
  }

  fNewResult.notify_one();
}


int TGRUTint::GetNPipelines() {
  return 1;
}

TPipeline* TGRUTint::GetPipeline(int i) {
  if(i==0){
    return fPipeline;
  } else {
    return NULL;
  }
}

int GetNPipelines() {
  return TGRUTint::instance()->GetNPipelines();
}

TPipeline* GetPipeline(int i) {
  return TGRUTint::instance()->GetPipeline(i);
}
