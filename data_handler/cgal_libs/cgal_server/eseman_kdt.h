#ifndef ESEMAN_KDT_H_
#define ESEMAN_KDT_H_

#include "eseman_commons.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

inline string generate_uuid() {
  return boost::uuids::to_string(boost::uuids::random_generator()());
}

class EsemanNode {
private:
public:
  string        uuid = generate_uuid();
  double        start_time;
  double        end_time;
  size_t        start_track;
  size_t        end_track;
  EsemanNode*   left_child;
  EsemanNode*   right_child;
  AttributeList attribute_lists;

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
    for (auto& pair : attribute_lists) {
      pair.second.clear();
    }
    attribute_lists.clear();
  }

  vector<string> getAttributeKeys();
  bool hasAttribute(const string& key) const;
  void addAttribute(const string& key, const int attr_index);

};

class EseManKDT {
private:
  StringIndexMapper                event_tracks;
  vector<EventDictList>            event_data_values;
  vector<EsemanNode*>              event_data_nodes;
  vector<string>                   eseman_node_uuids;
  AttributeDict                    event_data_attributes;
  string                           return_attribute_key = "";
  EventDictList                    filters;
  int                              max_depth_to_load = 14;

  bool checkFilterSatisfied(const EsemanNode* node, const EventDict& filter);
  bool checkFiltersSatisfied(const EsemanNode* node);

  string findNodeInTimeRange(string uuid, double s_time, double e_time);
  EsemanNode* constructKDTPerTrack(size_t start_index, size_t end_index, size_t track_index);
  void printKDTDotRecursive(EsemanNode* node, ofstream& dotFile);
  vector<double> binnedRangeQueryPerTrack(int64_t time_begin, 
                                      int64_t time_end,
                                      size_t track_index,
                                      uint64_t bins);
  void findClusters(int64_t start_t, int64_t end_t, int64_t bin_size, const EsemanNode* c_node, vector<int64_t> &results, int depth);
  void deleteTree(EsemanNode* node);

  void saveNodeToFile(const EsemanNode* node);
  EsemanNode* loadNodeFromFile(const string& uuid, int depth);

public:
  int horizontal_resolution_divisor = 1;
  int vertical_resolution_divisor = 1;
  bool is_vertical_split = false;
  string node_storage_base_path = ".";
  string dataset_id = "default_dataset";
  
  EseManKDT() {
      // Constructor logic if needed
  }
  
  ~EseManKDT() {
      for(auto node : event_data_nodes) {
          deleteTree(node);
      }
      event_data_values.clear();
      event_data_nodes.clear();
      event_data_attributes.clear();
      eseman_node_uuids.clear();
  }

  void insertDataIntoTree(double start_time, double end_time, string track, string primitive_name, string interval_id);
  void buildKDT();
  void printKDTDotPerTrack(size_t track_index);
  void printKDTDot();

  void cleanNodesFromMemory(bool is_store_existing);
  bool reloadNodesFromFile(bool is_load_attributes, double s_time, double e_time);

  void addPrimitiveFilter(string primitive_filter) {
    for (const auto& filter : filters) {
      if(getEventPrimitive(filter) == primitive_filter) return;
    }
    filters.push_back(EventDict{{"primitive", primitive_filter}});
  }
  void addIDFilter(string id_filter) {
    for (const auto& filter : filters) {
      if(getEventID(filter) == id_filter) return;
    }
    filters.push_back(EventDict{{"ID", id_filter}});
  }
  void clearPrimitiveFilters() {
    filters.clear();
  }

  LocDict binnedRangeQuery(int64_t time_begin, int64_t time_end, 
                          uint64_t location_begin, uint64_t location_end, 
                          uint64_t bins);
  string findNearestEvent(uint64_t cTime, uint64_t cLocation);
};

#endif