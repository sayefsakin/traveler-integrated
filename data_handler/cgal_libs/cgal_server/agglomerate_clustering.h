#ifndef AGGLOMERATE_CLUSTERING_H_
#define AGGLOMERATE_CLUSTERING_H_

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstdint>

using namespace std;

typedef std::map<uint64_t, std::vector<double>>             LocDict;

#ifndef _DEBUG
#define PRINTLOG(x) 
#else
#define PRINTLOG(x) std::cout << x << std::endl
#endif

inline uint64_t getAGCBinSize(int64_t time_begin, int64_t time_end, uint64_t bins){
  return (uint64_t)floor((double)(time_end - time_begin) / (double)bins);
}

inline int getAGCBinNumber(int64_t time_begin, int64_t time_end, uint64_t bins, int64_t ctime) {
  uint64_t bin_size = getAGCBinSize(time_begin, time_end, bins);
  if(ctime < time_begin || ctime > time_end) return -1;
  return (int)floor((double)(ctime - time_begin) / (double)bin_size);
}

// this is a one dimensional agglomerate clustering for event sequences.
// Start and end events will be stored in a vector, even number index will hold the start event and odd number index will hold the end event.
class EventAgglomerateClustering {
  private:
    std::vector<double> data;
    int* merge = NULL;
    double* height = NULL;
    int* node_size = NULL;
    int npoints = 0;

    int searchEvent(int64_t tb, int begin_index);

  public:
    EventAgglomerateClustering() {
      // Initialization logic for AgglomerateClustering
    }

    ~EventAgglomerateClustering() {
      // Memory cleanup logic for AgglomerateClustering
      data.clear();
      delete[] merge;
      delete[] height;
      delete[] node_size;
    }

    void insertDataIntoTree(double start_time, double end_time);
    void buildAggCluster();
    vector<double> binnedRangeQuery(int64_t time_begin, int64_t time_end, uint64_t bins);

    void getDataSize() {
      PRINTLOG("Data size: " << data.size());
    }
    std::string track;
};

class AgglomerateClusters {
  private:
    std::map<std::string, EventAgglomerateClustering> agglomerate_clusters;

  public:
  AgglomerateClusters() {
    // Initialization logic for AgglomerateClusters
  }
  ~AgglomerateClusters() {
    agglomerate_clusters.clear();
    // Memory cleanup logic for AgglomerateClusters
  }
  void insertDataIntoTree(double start_time, double end_time, string track);
  void buildAllAggClusters();
  LocDict binnedRangeQuery(int64_t time_begin, 
    int64_t time_end, 
    uint64_t location_begin, 
    uint64_t location_end, 
    uint64_t bins);
};
#endif