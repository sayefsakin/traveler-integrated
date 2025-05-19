#ifndef ESEMAN_KDT_H_
#define ESEMAN_KDT_H_


#include <iostream>
#include <utility>
#include <cstring>
#include <string>
#include <set>
#include <map>
#include <cstdint>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>

using namespace std;

#ifndef _DEBUG
#define PRINTLOG(x) 
#else
#define PRINTLOG(x) std::cout << x << std::endl
#endif

typedef std::unordered_map<std::string, size_t>             String_to_index;
typedef std::map<uint64_t, std::vector<double>>             LocDict;

class EventTracks {
private:
  const string OUT_OF_RANGE = "Index out of range";
  std::vector<std::string> tracks;
  String_to_index track_to_index;
public:
  // Constructor takes an optional custom comparator lambda for ordering
  EventTracks(){}
  ~EventTracks() {
    // PRINTLOG("EventTracks destructor called");
    tracks.clear();
    track_to_index.clear();
  }
  const std::string& operator[](size_t idx) const {
    return at(idx);
  }

  // Access string by insertion order
  const std::string& at(size_t idx) const {
    if (idx >= tracks.size()) {
      return OUT_OF_RANGE;
    }
    return tracks.at(idx);
  }

  // Insert a string if not present, preserving insertion order
  void insert(const std::string& s) {
    if (track_to_index.find(s) == track_to_index.end()) {
      track_to_index[s] = tracks.size();
      tracks.push_back(s);
    }
  }

  // Returns the index of the string if found, else return size()
  size_t get_track_index(const std::string& s) {
    if (track_to_index.find(s) == track_to_index.end()) {
      PRINTLOG("Track not found: " << s);
      return tracks.size();
    }
    return track_to_index[s];
  }

  // Number of unique strings
  size_t size() const {
    return tracks.size();
  }

  void order_tracks(const std::function<bool(const std::string&, const std::string&)>& comp) {
    std::sort(tracks.begin(), tracks.end(), comp);
    // Update track_to_index to reflect new order
    for (size_t i = 0; i < tracks.size(); ++i) {
      track_to_index[tracks[i]] = i;
    }
  }

  void print_tracks() const {
    for (const auto& track : tracks) {
      std::cout << track << " ";
    }
    std::cout << std::endl;
  }
};


inline uint64_t getESEBinSize(int64_t time_begin, int64_t time_end, uint64_t bins){
  return (uint64_t)floor((double)(time_end - time_begin) / (double)bins);
}

inline int getESEBinNumber(int64_t time_begin, int64_t time_end, uint64_t bins, int64_t ctime) {
  uint64_t bin_size = getESEBinSize(time_begin, time_end, bins);
  if(ctime < time_begin || ctime > time_end) return -1;
  return (int)floor((double)(ctime - time_begin) / (double)bin_size);
}

inline std::string generate_uuid() {
  static std::random_device              rd;
  static std::mt19937                    gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  static std::uniform_int_distribution<> dis2(8, 11);

  std::stringstream ss;
  int i;
  ss << std::hex;
  for (i = 0; i < 8; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (i = 0; i < 4; i++) {
    ss << dis(gen);
  }
  ss << "-4"; // version 4
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  ss << dis2(gen); // variant
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << "-";
  for (i = 0; i < 12; i++) {
    ss << dis(gen);
  }
  return ss.str();
}

class EsemanNode {
public:
  double start_time;
  double end_time;
  size_t start_track;
  size_t end_track;
  EsemanNode* left_child;
  EsemanNode* right_child;

  EsemanNode()
        : start_time(0), end_time(0), start_track(0), end_track(0),
          left_child(nullptr), right_child(nullptr) {}

    EsemanNode(double s_time, double e_time, size_t location)
        : start_time(s_time), end_time(e_time),
          start_track(location), end_track(location),
          left_child(nullptr), right_child(nullptr) {}

  ~EsemanNode() {
    // Don't delete children here - let EseManKDT handle deletion
    left_child = nullptr;
    right_child = nullptr;
  }
};

class EseManKDT {
private:
    EventTracks event_tracks;
    std::vector<std::vector<double>> event_data_values;
    std::vector<EsemanNode*> event_data_nodes;
    EsemanNode* constructKDTPerTrack(size_t start_index, size_t end_index, size_t track_index);
    void printKDTDotRecursive(EsemanNode* node, std::ofstream& dotFile);
    vector<double> binnedRangeQueryPerTrack(int64_t time_begin, 
                                        int64_t time_end,
                                        size_t track_index,
                                        uint64_t bins);
    void findClusters(int64_t start_t, int64_t end_t, int64_t bin_size, const EsemanNode* c_node, std::vector<int64_t> &results);
    void deleteTree(EsemanNode* node);

public:
    int horizontal_resolution_divisor = 1;
    EseManKDT() {
        // Constructor logic if needed
    }
    
    ~EseManKDT() {
        for(auto node : event_data_nodes) {
            deleteTree(node);
        }
        event_data_values.clear();
        event_data_nodes.clear();
    }

  void insertDataIntoTree(double start_time, double end_time, string track);
  void buildKDT();
  void printKDTDotPerTrack(size_t track_index);
  void printKDTDot();

  LocDict binnedRangeQuery(int64_t time_begin, 
                                        int64_t time_end, 
                                        uint64_t location_begin, 
                                        uint64_t location_end, 
                                        uint64_t bins);
};

#endif