#include "TCagra.h"

#include <iostream>
#include <fstream>

#include "TANLEvent.h"
#include "TChannel.h"

#include "TGEBEvent.h"

#include <random>
#include <chrono>


std::map<int,TVector3> TCagra::detector_positions;


TCagra::TCagra(){
  Clear();
}

TCagra::~TCagra() {

}

void TCagra::Copy(TObject& obj) const {
  TDetector::Copy(obj);

  TCagra& detector = (TCagra&)obj;
  detector.cagra_hits = cagra_hits;
}

void TCagra::InsertHit(const TDetectorHit& hit){
  cagra_hits.emplace_back((TCagraHit&)hit);
  fSize++;
}

unsigned int GetRandomCAGRAChannel(const int detnum, const char leaf) {
  // Board ids  101-108 90deg Yale clovers + BGO
  //            109,110 135deg Yale clovers + BGO
  //            111-114 135deg IMP clovers + BGO
  // Channel ids
  //            Yale    1 digitizer per clover
  //                    0-3 core signals
  //                    4 - L, 5 - M, 6 - R
  //                    7 - BGO
  //
  //            IMP     2 digitizers per clover
  //                    0,5 - core signals
  //                    1-4,6-9, 4 signals
  static std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
  static std::uniform_int_distribution<> board(101,114);
  static std::uniform_real_distribution<float> channel(0,1);


  unsigned int board_id = 0;
  unsigned int chan_id = 0; // channel(mt);
  unsigned int address = 0xeeeeeeee;

  if (detnum == -1 && leaf == -1) {
    // asking for a random detector (clover and leaf)
    do {
      board_id = board(mt);
      if (board_id > 110) {
        chan_id = channel(mt)*10;
      } else {
        chan_id = channel(mt)*8;
      }
      address = ((1<<24) + (board_id << 8) + chan_id);

      std::cout << "Parent: ";
      std::cout << TChannel::Get(address)->GetArrayPosition() << " ";
      std::cout << TChannel::Get(address)->GetArraySubposition() << " ";
      std::cout << TChannel::Get(address)->GetSegment() << std::endl;
    } while (TChannel::Get(address)->GetSegment() != 0);



  } else {

    // asking for a segment of specified detector
    do {
      board_id = board(mt);
      if (board_id > 110) {
        chan_id = channel(mt)*10;
      } else {
        chan_id = channel(mt)*8;
      }
      address = ((1<<24) + (board_id << 8) + chan_id);

      std::cout << "  Segment: ";
      std::cout << TChannel::Get(address)->GetArrayPosition() << " ";
      std::cout << TChannel::Get(address)->GetArraySubposition() << " ";
      std::cout << TChannel::Get(address)->GetSegment() << std::endl;;

    } while (!(TChannel::Get(address)->GetArrayPosition() == detnum &&
               *TChannel::Get(address)->GetArraySubposition() == leaf)
      );
    std::cout << "Found!" << std::endl;

  }



  //std::cout << board_id << " " << chan_id << std::endl;
  //std::cout << std::hex << ((1<<24) + (board_id << 8) + chan_id) << std::endl;
  return address;


}

int TCagra::BuildHits(std::vector<TRawEvent>& raw_data){
  static std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
  static std::uniform_real_distribution<float> random(0,1);

  unsigned int count = 0;
  for (auto& event : raw_data) {
    SetTimestamp(event.GetTimestamp());

    auto buf = event.GetPayloadBuffer();
    TANLEvent anl(buf);

    unsigned int address = ( (1<<24) +
                             (anl.GetBoardID() << 8) +
                             anl.GetChannel() );



    static int prev_detnum = -1;
    static char prev_leaf = -1;

    if (random(mt) < 0.2 || count == 0) {
      // pick new clover and leaf
      address = GetRandomCAGRAChannel(-1,-1);
      prev_detnum = TChannel::Get(address)->GetArrayPosition();
      prev_leaf = *TChannel::Get(address)->GetArraySubposition();

    } else {
      // use previous clover and leaf
      address = GetRandomCAGRAChannel(prev_detnum,prev_leaf);



    }


    TChannel* chan = TChannel::GetChannel(address);

    auto boardnum = (address&0x0000ff00)>>8;
    auto channum = (address&0x000000ff)>>0;

    static int lines_displayed = 0;
    if(!chan){
      if(lines_displayed < 10) {
        // std::cout << "Unknown (board id, channel): ("
        //           << anl.GetBoardID() << ", " << anl.GetChannel()
        //           << ")" << std::endl;
        std::cout << "Unknown (board id, channel): ("
                  << boardnum << ", " << channum
                  << ")" << std::endl;
      } else if(lines_displayed==1000){
        std::cout << "I'm going to stop telling you that the channel was unknown,"
                  << " you should probably stop the program." << std::endl;
      }
      lines_displayed++;

      continue;
    }


    int detnum = chan->GetArrayPosition(); // clover number
    char leaf = *chan->GetArraySubposition(); // clover number
    int segnum = chan->GetSegment(); // segment number

    // Get a hit, make it if it does not exist
    TCagraHit* hit = NULL;
    for(auto& ihit : cagra_hits){
      if(ihit.GetDetnum() == detnum && ihit.GetLeaf() == leaf){
        hit = &ihit;
        break;
      }
    }


    if(hit == NULL){
      cagra_hits.emplace_back();
      hit = &cagra_hits.back();
      fSize++;
    }




    if(segnum==0){
      hit->SetAddress(address);
      hit->SetTimestamp(event.GetTimestamp());
      hit->SetDiscTime(anl.GetCFD());
      hit->SetCharge(anl.GetEnergy());
      hit->SetTrace(anl.GetTrace());
      hit->SetPreRise(anl.GetPreE());
      hit->SetPostRise(anl.GetPostE());
      hit->SetFlags(anl.GetFlags());
      hit->SetBaseSample(anl.GetBaseSample());
      std::cout << "Count: " << count << " Address: " << std::hex << hit->Address() << std::dec << std::endl;
    } else {
      TCagraSegmentHit& seg = hit->MakeSegmentByAddress(address);
      seg.SetCharge(anl.GetEnergy());
      seg.SetTimestamp(event.GetTimestamp());
      std::cout << "Count: " << count << " Seg. Address: " << std :: hex << seg.Address() << std::dec << std::endl;
      // TODO: the following need implementation
      //seg.SetDiscTime(anl.GetCFD());
      //seg->SetPreRise(anl.GetPreE());
      //seg->SetPostRise(anl.GetPostE());
      //seg->SetFlags(anl.GetFlags());
      //seg->SetBaseSample(anl.GetBaseSample());
      seg.SetTrace(anl.GetTrace());
    }
    count++;
  }


  //TCagraHit hit;
  //hit.BuildFrom(buf);
  //hit.SetTimestamp(event.GetTimestamp());
  //InsertHit(hit);
  std::cout << "Done." << std::endl;
  std::cin.get();

  return Size();
}
TVector3 TCagra::GetSegmentPosition(int detnum, char subpos, int segnum) {
  if(detnum < 1 || detnum > 16 || segnum < 0 || segnum > 2 ||
     subpos < 0x41 || subpos > 0x44){
    return TVector3(std::sqrt(-1),std::sqrt(-1),std::sqrt(-1));
  }
  LoadDetectorPositions();

  int index = (detnum << 16) + (((int)subpos) << 8) + segnum;

  if (detector_positions.count(index)>0) {
    return detector_positions.at(index);
  } else {
    return TVector3(0.,0.,0.);
  }

  // double segment_height = 1.0;
  // double perp_distance = 1.5;

  // // Middle of the segment
  // double segment_phi = 3.1415926535/4.0;
  // double segment_z = segment_height/2.0;

  // double crystal_phi = segment_phi + (segnum-2)*3.1415926/2.0;
  // double crystal_z = segment_z + ((segnum-1)/4)*segment_height;

  // TVector3 crystal_pos(1,0,0);
  // crystal_pos.SetZ(crystal_z);
  // crystal_pos.SetPhi(crystal_phi);
  // crystal_pos.SetPerp(perp_distance);

  // TVector3 global_pos = CrystalToGlobal(detnum, crystal_pos);


  // return global_pos;
}

void TCagra::LoadDetectorPositions() {
  static bool loaded = false;
  if(loaded){
    return;
  }
  loaded = true;

  //std::string filename = std::string(getenv("GRUTSYS")) + "/../config/SeGA_rotations.txt";
  std::string filename = std::string(getenv("GRUTSYS")) + "/config/CAGRA_positions.txt";

  //Read the locations from file.
  std::ifstream infile(filename);

  if(!infile){
    std::cout << "Cagra positions file \"" << filename << "\""
              << " does not exist, skipping" << std::endl;
    return;
  }

  std::string line;
  while (std::getline(infile,line)) {
    //Parse the line
    int nclover, nsegment;
    char ncrystal;
    double x,y,z;
    int extracted = sscanf(line.c_str(),"clover.%02d.%c.%01d.vec: %lf %lf %lf",
                           &nclover,&ncrystal,&nsegment,&x,&y,&z);
    if (extracted!=6) {
      continue;
    }

    int index = (nclover << 16) + (((int)ncrystal) << 8) + nsegment;


    //Pack into the vector of transformations.
    TVector3 vec = TVector3(x,y,z);
    detector_positions[index] = vec;
  }
}

void TCagra::Print(Option_t *opt) const { }

void TCagra::Clear(Option_t *opt) {
  cagra_hits.clear();
}
