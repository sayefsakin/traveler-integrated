#include <chrono>

#include "agglomerate_clustering.h"
#include "fstream"

#include "hclust-cpp/fastcluster.h"

inline double distance_event(const double& s11, const double& s12, const double& s21, const double& s22) {
    if (s12 < s21)
      return s21 - s12;
    return s11 - s22;
  }

void EventAgglomerateClustering::insertDataIntoTree(double start_time, double end_time) {
    if (start_time > end_time) {
        PRINTLOG("Invalid time range: start_time > end_time");
        return;
    }
    data.push_back(start_time);
    data.push_back(end_time);
}

void EventAgglomerateClustering::buildAggCluster() {
    // Implement the agglomerate clustering algorithm here
    // This is a placeholder for the actual clustering logic
    PRINTLOG("Building Agglomerate Clusters...");

    int i,j,k;
    const int opt_method = HCLUST_METHOD_SINGLE;

    npoints = data.size() / 2;
    // computation of condensed distance matrix
    double* distmat = new double[(npoints*(npoints-1))/2];
    k = 0;
    for (i=0; i<npoints; i++) {
        for (j=i+1; j<npoints; j++) {
        distmat[k] = distance_event(data[i*2], data[(i*2)+1], data[j*2], data[(j*2)+1]);
        k++;
        }
    }

    // clustering call
    merge = new int[2*(npoints-1)];
    height = new double[npoints-1];
    node_size = new int[2*(npoints-1)];
    start_events = new double[2*(npoints-1)];
    end_events = new double[2*(npoints-1)];
    hclust_fast(npoints, distmat, opt_method, merge, height, node_size);

    //
    int left_node_index, right_node_index, root_node_index;
    for (i=0; i<npoints-1; i++) {
      left_node_index = changeMerge(merge[i], npoints);
      if(left_node_index < npoints) {
        start_events[left_node_index] = data[left_node_index*2];
        end_events[left_node_index] = data[(left_node_index*2)+1];
      }

      right_node_index = changeMerge(merge[i+npoints-1], npoints);
      if(right_node_index < npoints) {
        start_events[right_node_index] = data[right_node_index*2];
        end_events[right_node_index] = data[(right_node_index*2)+1];
      }

      if(end_events[left_node_index] > start_events[right_node_index]) {
        std::swap(left_node_index, right_node_index);
      }
      
      root_node_index = i + npoints;
      start_events[root_node_index] = start_events[left_node_index];
      end_events[root_node_index] = end_events[right_node_index];
    }
    
    delete[] distmat;
    PRINTLOG("Building Agglomerate Clusters Done!");
}

int EventAgglomerateClustering::searchEvent(int64_t tb, int begin_index) {
  int left = begin_index, right = npoints-1;
  int s_begin = 0;
  while(left < right) {
    int mid = (left + right) / 2;
    if(data[(mid*2)+1] < tb) {
      left = mid + 1;
    } else if(data[mid*2] > tb) {
      right = mid - 1;
    } else {
      s_begin = mid;
      break;
    }
  }
  if (left >= right) {
    s_begin = left+1;
  }
  if(s_begin > npoints-1) s_begin = npoints-1;
  if(s_begin < 0) s_begin = 0;
  return s_begin;
}

// search logic
// if bin size is less than cluster length, then go down
// else return the start end point of the current cluster
// dfs on the start and end time query
void EventAgglomerateClustering::findClusters(int64_t start_t, int64_t end_t, int64_t bin_size, int c_node, std::vector<int64_t> &results) {
  int root_node = c_node + npoints;
  int64_t start_time = (int64_t)start_events[root_node];
  int64_t end_time = (int64_t)end_events[root_node];
  if(start_time >= end_t || end_time <= start_t) return;
  if(bin_size >= (end_time - start_time + 1)) {
    results.push_back(start_time);
    results.push_back(end_time);
    PRINTLOG("Cluster: " << root_node << ", Start: " << start_time << ", End: " << end_time);
    return;
  }
  if(root_node < npoints) {
    if(start_time < start_t) {
      start_time = start_t;
    }
    if(end_time > end_t) {
      end_time = end_t;
    }
    results.push_back(start_time);
    results.push_back(end_time);
    PRINTLOG("Cluster-Leaf: " << root_node << ", Start: " << start_time << ", End: " << end_time);
    return;
  }
  // this is a compound node
  int left_node_index = changeMerge(merge[c_node], npoints);
  int right_node_index = changeMerge(merge[c_node+npoints-1], npoints);
  if(end_events[left_node_index] > start_events[right_node_index]) {
    std::swap(left_node_index, right_node_index);
  }
  findClusters(start_t, end_t, bin_size, left_node_index - npoints, results);
  findClusters(start_t, end_t, bin_size, right_node_index - npoints, results);
}

vector<double> EventAgglomerateClustering::binnedRangeQuery(int64_t time_begin, int64_t time_end, uint64_t bins) {
  vector<double> results(bins);
  uint64_t bin_size(getAGCBinSize(time_begin, time_end, bins));
  PRINTLOG("Got AGC binned range query");

  int* labels = new int[npoints];
  
  // cutree_cdist(npoints, merge, height, (double)bin_size, labels);
  // int s_begin = searchEvent(time_begin, 0);
  // int s_end = searchEvent(time_end, s_begin);

  // vector<double> data_short_list;
  // data_short_list.push_back(data[s_begin*2]);
  // for(int i = s_begin+1; i <= s_end; i++) {
  //   if(labels[i] == labels[i-1]) continue;
  //   data_short_list.push_back(data[((i-1)*2)+1]);
  //   data_short_list.push_back(data[(i*2)]);
  // }
  // data_short_list.push_back(data[(s_end*2)+1]);

  vector<int64_t> data_short_list;
  findClusters(time_begin, time_end, (int64_t)bin_size, npoints-2, data_short_list);

  for(long unsigned int i = 0; i < data_short_list.size(); i+=2) {
    int64_t start_time = data_short_list[i];
    int64_t end_time = data_short_list[i+1]; 
    
    if(end_time < time_begin || start_time > time_end) continue;


    if(start_time < time_begin) start_time = time_begin;
    if(start_time > time_end) continue;
    if(end_time < time_begin) continue;
    if(end_time > time_end) end_time = time_end;
    int64_t startingBin = getAGCBinNumber(time_begin, time_end, bins, start_time);
    int64_t endingBin = getAGCBinNumber(time_begin, time_end, bins, end_time);
    if(startingBin < 0 || endingBin < 0) continue;

    for(int64_t bin_it = startingBin+1;
    bin_it < endingBin && bin_it < (int64_t)bins && results[bin_it] < 0.5; 
    bin_it++)
    results[bin_it] = 1.0;
    
    if(startingBin < (int64_t)bins && results[startingBin] < 0.5) 
      results[startingBin] = (start_time % bin_size)?0.5:1.0;
    if(endingBin < (int64_t)bins && results[endingBin] < 0.5)
      results[endingBin] = (end_time % bin_size)?0.5:1.0;
  }
  
  data_short_list.clear();
  delete[] labels;
  return results;
}

void AgglomerateClusters::insertDataIntoTree(double start_time, double end_time, string track) {
  if(agglomerate_clusters.find(track) == agglomerate_clusters.end()) {
    agglomerate_clusters[track] = EventAgglomerateClustering();
    agglomerate_clusters[track].track = track;
  }
  agglomerate_clusters[track].insertDataIntoTree(start_time, end_time);
}

void AgglomerateClusters::buildAllAggClusters() {
  for(auto it = agglomerate_clusters.begin(); it != agglomerate_clusters.end(); it++) {
    it->second.buildAggCluster();
    PRINTLOG("building agglomerate cluster for: " << it->first);
  }
}

LocDict AgglomerateClusters::binnedRangeQuery(int64_t time_begin, 
                                              int64_t time_end, 
                                              uint64_t location_begin, 
                                              uint64_t location_end, 
                                              uint64_t bins){
  LocDict locDict;
  PRINTLOG("Got AGC binned range query");
  std::chrono::steady_clock::time_point clock_begin = std::chrono::steady_clock::now();
  for (uint64_t c_loc = location_begin; c_loc <= location_end; c_loc++) {
    string c_loc_str = to_string(c_loc);
    if(agglomerate_clusters.find(c_loc_str) == agglomerate_clusters.end()) {
      PRINTLOG("Track not found in agglomerate clusters " << c_loc_str);
      continue;
    }
    
    locDict[c_loc] = agglomerate_clusters[c_loc_str].binnedRangeQuery(time_begin, time_end, bins);
  }
  std::chrono::steady_clock::time_point clock_end = std::chrono::steady_clock::now();
  // for (const auto& myPair : locDict) {
  //   cout << myPair.first << " = ";
  //   copy (myPair.second.begin(), myPair.second.end(), ostream_iterator<double>(cout,"\n") );
  //   cout << endl;
  // }
  cout << "AGC," << "ds_window";
  cout << "," << time_begin << "," << time_end << "," << std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_begin).count() <<
    endl;
  return locDict;
}

// int main() {
//     PRINTLOG("Agglomerate Clustering Starting!");
//     EventAgglomerateClustering agglomerate_clustering;

//     string input_file_path = "/mnt/c/Users/sayef/IdeaProjects/traveler-integrated/data_handler/cgal_libs/cgal_server/location_data/";
//     fstream input_file(input_file_path + "9.location");
//     if (!input_file.is_open()) {
//         PRINTLOG("Failed to open input file");
//         return -1;
//     }
//     uint64_t start_time, end_time;
//     while(input_file >> start_time >> end_time) {
//         agglomerate_clustering.insertDataIntoTree(start_time, end_time);
//     }
//     input_file.close();
//     agglomerate_clustering.getDataSize();//6666739
//     PRINTLOG("Data inserted into the cluster");

//     agglomerate_clustering.buildAggCluster();
//     vector<double> results = agglomerate_clustering.binnedRangeQuery(1074655386, 1246477086, 100);
//     PRINTLOG("Results: ");
//     for (const auto& result : results) {
//         cout << setprecision(1) << result << " ";
//     }
//     cout << endl;
//     PRINTLOG("Agglomerate Clustering Finished!");
//     return 0;
// }