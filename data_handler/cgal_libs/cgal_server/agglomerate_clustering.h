#ifndef AGGLOMERATE_CLUSTERING_H_
#define AGGLOMERATE_CLUSTERING_H_

#include "eseman_commons.h"

inline int changeMerge(int i, int N){
  if (i < 0) {
    return (i+1) * -1;
  } else {
    return (i - 1) + N;
  }
}

// this is a one dimensional agglomerate clustering for event sequences.
// Start and end events will be stored in a vector, even number index will hold the start event and odd number index will hold the end event.
class EventAgglomerateClustering {
  private:
    std::vector<EventDict> data;
    int* merge = NULL;
    double* height = NULL;
    int* node_size = NULL;
    double* start_events = NULL;
    double* end_events = NULL;
    int npoints = 0;

    int searchEvent(int64_t tb, int begin_index);
    void findClusters(int64_t start_t, int64_t end_t, int64_t bin_size, int c_node, std::vector<int64_t> &results);

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
      delete[] start_events;
      delete[] end_events;
    }

    void insertDataIntoTree(double start_time, double end_time, string primitive_name);
    void buildAggCluster();
    vector<double> binnedRangeQuery(int64_t time_begin, int64_t time_end, uint64_t bins, int hrd);

    void getDataSize() {
      PRINTLOG("Data size: " << data.size());
    }
    std::string track;
};

class AgglomerateClusters {
  private:
    std::map<std::string, EventAgglomerateClustering> agglomerate_clusters;

  public:
    int horizontal_resolution_divisor = 1;
  AgglomerateClusters() {
    // Initialization logic for AgglomerateClusters
  }
  ~AgglomerateClusters() {
    agglomerate_clusters.clear();
    // Memory cleanup logic for AgglomerateClusters
  }
  void insertDataIntoTree(double start_time, double end_time, string track, string primitive_name);
  void buildAllAggClusters();
  LocDict binnedRangeQuery(int64_t time_begin, 
    int64_t time_end, 
    uint64_t location_begin, 
    uint64_t location_end, 
    uint64_t bins);
};
#endif