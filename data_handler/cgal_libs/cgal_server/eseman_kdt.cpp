#include "eseman_kdt.h"
#include <fstream>
#include <chrono>

void test_event_tracks() {
    EventTracks eventTracks;
    eventTracks.insert("12-14");
    eventTracks.insert("11-35");
    eventTracks.insert("12-51");
    eventTracks.insert("10-20");
    eventTracks.insert("25-15");

    PRINTLOG("EventTracks size: " << eventTracks.size());
    PRINTLOG("Track at index 4: " << eventTracks[1]);
    string trackName = "12-352";
    PRINTLOG("Index of " << trackName << ": " << eventTracks.get_track_index(trackName));

    // Custom sort: order by first number, then by the number after the dash
    eventTracks.order_tracks([](const std::string& a, const std::string& b) {
        auto parse = [](const std::string& s) -> std::pair<int, int> {
            size_t dash = s.find('-');
            int first = std::stoi(s.substr(0, dash));
            int second = (dash != std::string::npos) ? std::stoi(s.substr(dash + 1)) : 0;
            return {first, second};
        };
        auto pa = parse(a);
        auto pb = parse(b);
        return pa < pb;
    });
    PRINTLOG("Tracks ordered.");
    eventTracks.print_tracks();
    PRINTLOG("Index of " << trackName << ": " << eventTracks.get_track_index(trackName));
}

void EseManKDT::insertDataIntoTree(double start_time, double end_time, string track) {
    size_t track_index = event_tracks.get_track_index(track);
    if(track_index > event_tracks.size()) {
        PRINTLOG("Track index out of range: " << track_index);
        return;
    } else if(track_index == event_tracks.size()) {
        event_tracks.insert(track);
        track_index = event_tracks.get_track_index(track);
        event_data_values.push_back(vector<double>());
        event_data_nodes.push_back(nullptr);
    }
    event_data_values[track_index].push_back(start_time);
    event_data_values[track_index].push_back(end_time);
}

void EseManKDT::deleteTree(EsemanNode* node) {
    if (!node) return;
    if (node->left_child) {
        deleteTree(node->left_child);
        node->left_child = nullptr;
    }
    if (node->right_child) {
        deleteTree(node->right_child);
        node->right_child = nullptr;
    }
    delete node;
}

// This is following only the sliding midpoint rule.
EsemanNode* EseManKDT::constructKDTPerTrack(size_t start_index, size_t end_index, size_t track_index) {
    vector<double>& data_vector = event_data_values[track_index];
    if (start_index >= data_vector.size() || end_index >= data_vector.size() || start_index >= end_index) return nullptr;
    
    EsemanNode* cur_node = new EsemanNode(data_vector[start_index], data_vector[end_index], track_index);
    if (start_index + 1 == end_index) {
        return cur_node;
    }
    // sliding midpoint rule
    // double mid_point = data_vector[start_index] + (data_vector[end_index] - data_vector[start_index]) / 2.0;
    // size_t mid_index = upper_bound(data_vector.begin() + start_index, data_vector.begin() + end_index + 1, mid_point) - data_vector.begin();
    // if(mid_index%2 == 1)
    //     mid_index--;
    // if(mid_index == start_index) mid_index = start_index + 2;
    // if(mid_index >= end_index) return cur_node;

    // sliding midpoint of max distance rule
    double max_distance = 0;
    size_t mid_index = start_index+2;
    for(size_t i = start_index+1; i < end_index; i+=2) {
        if(data_vector[i+1] - data_vector[i] > max_distance) {
            max_distance = data_vector[i+1] - data_vector[i];
            mid_index = i+1;
        }
    }

    if(cur_node->left_child != nullptr) {
        PRINTLOG("Left child deleted");
        delete cur_node->left_child;
        cur_node->left_child = nullptr;
    }
    if(cur_node->right_child != nullptr) {
        PRINTLOG("Right child deleted");
        delete cur_node->right_child;
        cur_node->right_child = nullptr;
    }
    cur_node->left_child = constructKDTPerTrack(start_index, mid_index-1, track_index);
    cur_node->right_child = constructKDTPerTrack(mid_index, end_index, track_index);
    return cur_node;
}

// search logic
// if bin size is less than cluster length, then go down
// else return the start end point of the current cluster
// dfs on the start and end time query
void EseManKDT::findClusters(int64_t start_t, int64_t end_t, int64_t bin_size, const EsemanNode* c_node, std::vector<int64_t> &results) {
    if(c_node == nullptr) return;
    int64_t start_time = (int64_t)c_node->start_time;
    int64_t end_time = (int64_t)c_node->end_time;
    if(start_time >= end_t || end_time <= start_t) return;
    if(bin_size >= (end_time - start_time + 1)) {
        results.push_back(start_time);
        results.push_back(end_time);
        PRINTLOG("Cluster: " << " Start: " << start_time << ", End: " << end_time);
        return;
    }
    if(c_node->left_child == nullptr && c_node->right_child == nullptr) {
        if(start_time < start_t) {
            start_time = start_t;
        }
        if(end_time > end_t) {
            end_time = end_t;
        }
        results.push_back(start_time);
        results.push_back(end_time);
        PRINTLOG("Cluster-Leaf: " << " Start: " << start_time << ", End: " << end_time);
        return;
    }
    // this is a compound node
    findClusters(start_t, end_t, bin_size, c_node->left_child, results);
    findClusters(start_t, end_t, bin_size, c_node->right_child, results);
}

vector<double> EseManKDT::binnedRangeQueryPerTrack(int64_t time_begin, 
                                        int64_t time_end,
                                        size_t track_index,
                                        uint64_t bins){
    vector<double> results(bins);
    uint64_t bin_size(getESEBinSize(time_begin, time_end, bins));

    vector<int64_t> data_short_list;
    findClusters(time_begin, time_end, (int64_t)bin_size*horizontal_resolution_divisor, event_data_nodes[track_index], data_short_list);

    for(long unsigned int i = 0; i < data_short_list.size(); i+=2) {
        int64_t start_time = data_short_list[i];
        int64_t end_time = data_short_list[i+1]; 
        
        if(end_time < time_begin || start_time > time_end) continue;


        if(start_time < time_begin) start_time = time_begin;
        if(start_time > time_end) continue;
        if(end_time < time_begin) continue;
        if(end_time > time_end) end_time = time_end;
        int64_t startingBin = getESEBinNumber(time_begin, time_end, bins, start_time);
        int64_t endingBin = getESEBinNumber(time_begin, time_end, bins, end_time);
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
    return results;
}

LocDict EseManKDT::binnedRangeQuery(int64_t time_begin, 
                                        int64_t time_end, 
                                        uint64_t location_begin, 
                                        uint64_t location_end, 
                                        uint64_t bins){
    LocDict locDict;
    PRINTLOG("Got EseMan KDT binned range query");
    std::chrono::steady_clock::time_point clock_begin = std::chrono::steady_clock::now();

    for (uint64_t c_loc = location_begin; c_loc <= location_end; c_loc++) {
        string c_loc_str = to_string(c_loc);
        size_t track_index = event_tracks.get_track_index(c_loc_str);
        if(track_index == event_tracks.size()) {
            PRINTLOG("Track not found in event tracks " << c_loc_str);
            continue;
        }
        locDict[c_loc] = binnedRangeQueryPerTrack(time_begin, time_end, track_index, bins);
        PRINTLOG("Track index: " << track_index << " " << event_tracks[track_index]);
    }
    std::chrono::steady_clock::time_point clock_end = std::chrono::steady_clock::now();

    cout << "ESEMAN," << "ds_window";
    cout << "," << time_begin << "," << time_end << "," << std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_begin).count() <<
    endl;
    return locDict;
}

void EseManKDT::printKDTDotPerTrack(size_t track_index) {
    std::ofstream dotFile("track_" + std::to_string(track_index) + ".dot");
    dotFile << "digraph G {" << std::endl;
    dotFile << "  label = \"Track " << event_tracks[track_index] << "\";" << std::endl;
    printKDTDotRecursive(event_data_nodes[track_index], dotFile);
    dotFile << "}" << std::endl;
    dotFile.close();
}

void EseManKDT::printKDTDotRecursive(EsemanNode* node, std::ofstream& dotFile) {
    if (!node) return;

    dotFile << "  \"" << node->start_time << "," << node->end_time << "\" [label=\"["
            << node->start_time << "," << node->end_time << "]\"];" << std::endl;

    if (node->left_child) {
        dotFile << "  \"" << node->start_time << "," << node->end_time << "\" -> \""
                << node->left_child->start_time << "," << node->left_child->end_time << "\";" << std::endl;
        printKDTDotRecursive(node->left_child, dotFile);
    }

    if (node->right_child) {
        dotFile << "  \"" << node->start_time << "," << node->end_time << "\" -> \""
                << node->right_child->start_time << "," << node->right_child->end_time << "\";" << std::endl;
        printKDTDotRecursive(node->right_child, dotFile);
    }
}

void EseManKDT::printKDTDot() {
    for(size_t i = 0; i < event_data_values.size(); ++i) {
        printKDTDotPerTrack(i);
    }
}

void EseManKDT::buildKDT() {
    // vector<std::string> older_event_tracks;
    // for (size_t j = 0; j < event_tracks.size(); ++j) {
    //     older_event_tracks.push_back(event_tracks[j]);
    //     PRINTLOG("Track index: " << j << " Track String: " << event_tracks[j] << " Track Value: " << event_data_values[j][0] << " " << event_data_values[j][1]);
    // }
    // event_tracks.order_tracks([](const std::string& a, const std::string& b) {
    //     return std::stoi(a) < std::stoi(b);
    // });
    // vector<vector<double>> temp_event_data_values = event_data_values;
    // vector<size_t> track_mapping(event_tracks.size());
    // for (size_t i = 0; i < event_tracks.size(); ++i) {
    //     track_mapping[i] = event_tracks.get_track_index(older_event_tracks[i]);
    // }
    // for (size_t i = 0; i < event_tracks.size(); ++i) {
    //     event_data_values[i] = temp_event_data_values[track_mapping[i]];
    // }
    // PRINTLOG("After sorting:");
    // for (size_t j = 0; j < event_tracks.size(); ++j) {
    //     PRINTLOG("Track index: " << j << " Track String: " << event_tracks[j] << " Track Value: " << event_data_values[j][0] << " " << event_data_values[j][1]);
    // }


    for (size_t i = 0; i < event_data_values.size(); ++i) {
        if (event_data_values[i].empty()) continue;
        std::sort(event_data_values[i].begin(), event_data_values[i].end());
        if(event_data_nodes[i] != nullptr) {
            PRINTLOG("Deleting old KDT for track index: " << event_tracks[i]);
            deleteTree(event_data_nodes[i]);
        }
        event_data_nodes[i] = constructKDTPerTrack(0, event_data_values[i].size() - 1, i);
        PRINTLOG("Constructing KDT for track index: " << event_tracks[i]);
    }
    // older_event_tracks.clear();
    // temp_event_data_values.clear();
}

void test_KDT_build() {
    EseManKDT *kdt = new EseManKDT();
    // kdt.insertDataIntoTree(5.0, 6.0, "12");
    // kdt.insertDataIntoTree(1.0, 2.0, "12");
    // kdt.insertDataIntoTree(1100.0, 1110.0, "12");
    // kdt.insertDataIntoTree(11.0, 12.0, "12");
    // kdt.insertDataIntoTree(14.0, 18.0, "12");


    string input_file_path = "/mnt/c/Users/sayef/IdeaProjects/traveler-integrated/data_handler/cgal_libs/cgal_server/location_data/";
    fstream input_file(input_file_path + "9.location");
    if (!input_file.is_open()) {
        PRINTLOG("Failed to open input file");
        return;
    }
    uint64_t start_time, end_time;
    while(input_file >> start_time >> end_time) {
        kdt->insertDataIntoTree(start_time, end_time, "9");
    }
    input_file.close();

    fstream input_file1(input_file_path + "1.location");
    if (!input_file1.is_open()) {
        PRINTLOG("Failed to open input file");
        return;
    }
    while(input_file1 >> start_time >> end_time) {
        kdt->insertDataIntoTree(start_time, end_time, "1");
    }
    input_file1.close();

    fstream input_file2(input_file_path + "14.location");
    if (!input_file2.is_open()) {
        PRINTLOG("Failed to open input file");
        return;
    }
    while(input_file2 >> start_time >> end_time) {
        kdt->insertDataIntoTree(start_time, end_time, "14");
    }
    input_file2.close();

    kdt->buildKDT();
    // kdt.printKDTDot();
    // kdt.printKDTDot();
    // kdt.binnedRangeQuery(1, 1200, 
    //                     12, 12, 
    //                     100);
    kdt->binnedRangeQuery(-1305029698, 2753780939, 
                            9, 9, 
                            10);
}

#ifdef TESTING
int main() {
    PRINTLOG("hello inside eseman kdt");
    // test_event_tracks();
    test_KDT_build();
    PRINTLOG("Eseman KDT finished!");
    return 0;
}
#endif