#include "eseman_kdt.h"

void test_event_tracks() {
    StringIndexMapper StringIndexMapper;
    StringIndexMapper.insert("12-14");
    StringIndexMapper.insert("11-35");
    StringIndexMapper.insert("12-51");
    StringIndexMapper.insert("10-20");
    StringIndexMapper.insert("25-15");

    PRINTLOG("StringIndexMapper size: " << StringIndexMapper.size());
    PRINTLOG("Track at index 4: " << StringIndexMapper[1]);
    string trackName = "12-352";
    PRINTLOG("Index of " << trackName << ": " << StringIndexMapper.get_track_index(trackName));

    // Custom sort: order by first number, then by the number after the dash
    StringIndexMapper.order_tracks([](const string& a, const string& b) {
        auto parse = [](const string& s) -> pair<int, int> {
            size_t dash = s.find('-');
            int first = stoi(s.substr(0, dash));
            int second = (dash != string::npos) ? stoi(s.substr(dash + 1)) : 0;
            return {first, second};
        };
        auto pa = parse(a);
        auto pb = parse(b);
        return pa < pb;
    });
    PRINTLOG("Tracks ordered.");
    StringIndexMapper.print_tracks();
    PRINTLOG("Index of " << trackName << ": " << StringIndexMapper.get_track_index(trackName));
}

// ESEmanNode functions
vector<string> EsemanNode::getAttributeKeys() {
    vector<string> keys;
    for (const auto& pair : attribute_lists) {
        keys.push_back(pair.first);
    }
    return keys;
}

bool EsemanNode::hasAttribute(const string& key) const {
    return attribute_lists.find(key) != attribute_lists.end();
}

void EsemanNode::addAttribute(const string& key, const int attr_index) {
    if(!hasAttribute(key)) {
      attribute_lists[key] = unordered_set<size_t>();
    }
    attribute_lists[key].insert(attr_index);
    // int v_index = attr_index / 64;
    // int v_bit = attr_index % 64;
    // uint64_t mask = 1ULL << v_bit;
    // while (attribute_lists[key].size() <= v_index) {
    //   attribute_lists[key].push_back(0);
    // }
    // uint64_t& value = attribute_lists[key][v_index];
    // value |= mask;
}

void EseManKDT::insertDataIntoTree(double start_time, double end_time, string track, string primitive_name, string interval_id) {
    size_t track_index = event_tracks.get_track_index(track);
    if(track_index > event_tracks.size()) {
        PRINTLOG("Track index out of range: " << track_index);
        return;
    } else if(track_index == event_tracks.size()) {
        event_tracks.insert(track);
        track_index = event_tracks.get_track_index(track);
        event_data_values.push_back(EventDictList());
        event_data_nodes.push_back(nullptr);
    }
    event_data_values[track_index].push_back(EventDict{{"time",start_time},{"primitive", primitive_name},{"ID", interval_id}});
    event_data_values[track_index].push_back(EventDict{{"time",end_time},{"primitive", primitive_name},{"ID", interval_id}});
    
    if(event_data_attributes.find("primitive") == event_data_attributes.end()) {
        event_data_attributes.insert(make_pair("primitive", StringIndexMapper()));
    }
    event_data_attributes["primitive"].insert(primitive_name);

    if(event_data_attributes.find("ID") == event_data_attributes.end()) {
        event_data_attributes.insert(make_pair("ID", StringIndexMapper()));
    }
    event_data_attributes["ID"].insert(interval_id);
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

bool EseManKDT::checkFilterSatisfied(const EsemanNode* node, const EventDict& filter) {
    try {
        for (const auto& [key, value] : filter) {
            if (!(node->hasAttribute(key))) return false;
            if (!node->attribute_lists.at(key).count(get<size_t>(value))) return false;
        }
    } catch (...) {
        return true;
    }
    return true;
}
bool EseManKDT::checkFiltersSatisfied(const EsemanNode* node) {
    for (const auto& filter : filters) {
        if (!checkFilterSatisfied(node, filter)) return false;
    }
    return true;
}

// This is following only the sliding midpoint rule.
EsemanNode* EseManKDT::constructKDTPerTrack(size_t start_index, size_t end_index, size_t track_index) {
    EventDictList& data_vector = event_data_values[track_index];
    if (start_index >= data_vector.size() || end_index >= data_vector.size() || start_index >= end_index) return nullptr;
    
    EsemanNode* cur_node = new EsemanNode(getEventTime(data_vector[start_index]), getEventTime(data_vector[end_index]), track_index);
    if (start_index + 1 == end_index) {
        for (const auto& [key, indexes] : data_vector[start_index]) {
            if (key == "time") continue;
            size_t attr_index = event_data_attributes[key].get_track_index(get<string>(indexes));
            cur_node->addAttribute(key, attr_index);
        }
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
        if(getEventTime(data_vector[i+1]) - getEventTime(data_vector[i]) > max_distance) {
            max_distance = getEventTime(data_vector[i+1]) - getEventTime(data_vector[i]);
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
    if (cur_node->left_child) {
        for (const auto& [key, indexes] : cur_node->left_child->attribute_lists) {
            for (size_t index : indexes) {
                cur_node->addAttribute(key, index);
            }
        }
    }
    if (cur_node->right_child) {
        for (const auto& [key, indexes] : cur_node->right_child->attribute_lists) {
            for (size_t index : indexes) {
                cur_node->addAttribute(key, index);
            }
        }
    }
    return cur_node;
}

// search logic
// if bin size is less than cluster length, then go down
// else return the start end point of the current cluster
// dfs on the start and end time query
void EseManKDT::findClusters(int64_t start_t, int64_t end_t, int64_t bin_size, const EsemanNode* c_node, vector<int64_t> &results) {
    if(c_node == nullptr || !checkFiltersSatisfied(c_node)) return;
    int64_t start_time = (int64_t)c_node->start_time;
    int64_t end_time = (int64_t)c_node->end_time;
    if(start_time >= end_t || end_time <= start_t) return;
    if(bin_size >= (end_time - start_time + 1)) {
        if(return_attribute_key != "") {
            if(!c_node->hasAttribute(return_attribute_key)) {
                PRINTLOG("Attribute not found for key: " << return_attribute_key);
                return;
            }
            results.push_back((int64_t)(*c_node->attribute_lists.at(return_attribute_key).begin()));
        } else {
            results.push_back(start_time);
            results.push_back(end_time);
        }
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
        if(return_attribute_key != "") {
            if(!c_node->hasAttribute(return_attribute_key)) {
                PRINTLOG("Attribute not found for key: " << return_attribute_key);
                return;
            }
            results.push_back((int64_t)(*c_node->attribute_lists.at(return_attribute_key).begin()));
        } else {
            results.push_back(start_time);
            results.push_back(end_time);
        }
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
    uint64_t bin_size(getBinSize(time_begin, time_end, bins));

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
        int64_t startingBin = getBinNumber(time_begin, time_end, bins, start_time);
        int64_t endingBin = getBinNumber(time_begin, time_end, bins, end_time);
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

    try {
        for (size_t i = 0; i < filters.size(); i++) {
            for (const auto& [key, value] : filters[i]) {
                filters[i][key] = event_data_attributes[key].get_track_index(get<string>(value));
            }
        }
    } catch (...) {
        PRINTLOG("Error in converting filter attributes to indices");
    }
    
    chrono::steady_clock::time_point clock_begin = chrono::steady_clock::now();
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
    chrono::steady_clock::time_point clock_end = chrono::steady_clock::now();

    filters.clear(); // automatically clear filters after query

    cout << "ESEMAN," << "ds_window";
    cout << "," << time_begin << "," << time_end << "," << chrono::duration_cast<chrono::microseconds>(clock_end - clock_begin).count() <<
    endl;
    return locDict;
}

string EseManKDT::findNearestEvent(uint64_t cTime, uint64_t cLocation) {
  string ret_result("");
  string c_loc_str = to_string(cLocation);
  size_t track_index = event_tracks.get_track_index(c_loc_str);
  

  int64_t result = -1;
  return_attribute_key = "ID";
  vector<int64_t> data_short_list;

  uint64_t bin_size(getBinSize(cTime, cTime+1, 1));
  findClusters(cTime, cTime+1, (int64_t)bin_size, event_data_nodes[track_index], data_short_list);
  if(data_short_list.size() > 0) result = data_short_list[0];
  if(result >= 0) ret_result = event_data_attributes["ID"][result];
  data_short_list.clear();
  return_attribute_key = "";
  return ret_result;
}

void EseManKDT::printKDTDotPerTrack(size_t track_index) {
    ofstream dotFile("track_" + to_string(track_index) + ".dot");
    dotFile << "digraph G {" << endl;
    dotFile << "  label = \"Track " << event_tracks[track_index] << "\";" << endl;
    printKDTDotRecursive(event_data_nodes[track_index], dotFile);
    dotFile << "}" << endl;
    dotFile.close();
}

void EseManKDT::printKDTDotRecursive(EsemanNode* node, ofstream& dotFile) {
    if (!node) return;

    dotFile << "  \"" << node->start_time << "," << node->end_time << "\" [label=\"["
            << node->start_time << "," << node->end_time << "]\"];" << endl;

    if (node->left_child) {
        dotFile << "  \"" << node->start_time << "," << node->end_time << "\" -> \""
                << node->left_child->start_time << "," << node->left_child->end_time << "\";" << endl;
        printKDTDotRecursive(node->left_child, dotFile);
    }

    if (node->right_child) {
        dotFile << "  \"" << node->start_time << "," << node->end_time << "\" -> \""
                << node->right_child->start_time << "," << node->right_child->end_time << "\";" << endl;
        printKDTDotRecursive(node->right_child, dotFile);
    }
}

void EseManKDT::printKDTDot() {
    for(size_t i = 0; i < event_data_values.size(); ++i) {
        printKDTDotPerTrack(i);
    }
}

void EseManKDT::buildKDT() {
    for (size_t i = 0; i < event_data_values.size(); ++i) {
        if (event_data_values[i].empty()) continue;

        // No need to sort, cosidering the data will be come in order of time.
        // sort(event_data_values[i].begin(), event_data_values[i].end(), 
        //     [](const EventDict& a, const EventDict& b) {
        //         return stod(a.at("time")) < stod(b.at("time"));
        //     });
        if(event_data_nodes[i] != nullptr) {
            PRINTLOG("Deleting old KDT for track index: " << event_tracks[i]);
            deleteTree(event_data_nodes[i]);
        }
        if(is_vertical_split == false) {
            event_data_nodes[i] = constructKDTPerTrack(0, event_data_values[i].size() - 1, i);
            eseman_node_uuids.push_back(event_data_nodes[i]->uuid);
        }
        PRINTLOG("Constructing KDT for track index: " << event_tracks[i]);
        event_data_values[i].clear();
    }
    event_data_values.clear();
    // if(is_vertical_split)
    //     event_data_nodes[0] = constructKDTPerTrack(0, event_data_values[i].size() - 1, i);
}

void EseManKDT::saveNodeToFile(const EsemanNode* node) {
    if (!node) return;

    // Create file using node's UUID
    ofstream file(node_storage_base_path + "/" + node->uuid);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << node->uuid << endl;
        return;
    }

    // Save node data
    file << doubleToStringZeroPrecision(node->start_time) << " " << doubleToStringZeroPrecision(node->end_time) << " "
            << node->start_track << " " << node->end_track << "\n";

    // Save attributes
    file << node->attribute_lists.size() << "\n";
    for (const auto& attr : node->attribute_lists) {
        file << attr.first << " " << attr.second.size() << "\n";
        for (int val : attr.second) {
        file << val << " ";
        }
        file << "\n";
    }

    // Save child UUIDs for reference
    file << (node->left_child ? node->left_child->uuid : "NULL") << "\n";
    file << (node->right_child ? node->right_child->uuid : "NULL") << "\n";
    file.close();

    // Recursively save children
    if (node->left_child) saveNodeToFile(node->left_child);
    if (node->right_child) saveNodeToFile(node->right_child);
}

EsemanNode* EseManKDT::loadNodeFromFile(const string& uuid) {
    if (uuid == "NULL") return nullptr;

    ifstream file(node_storage_base_path + "/" + uuid);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << uuid << endl;
        return nullptr;
    }

    EsemanNode* node = new EsemanNode();
    node->uuid = uuid;

    // Load node data
    file >> node->start_time >> node->end_time 
            >> node->start_track >> node->end_track;

    // Load attributes
    int attr_count;
    file >> attr_count;
    file.ignore();

    for (int i = 0; i < attr_count; i++) {
        string key;
        int size;
        file >> key >> size;
        
        for (int j = 0; j < size; j++) {
            size_t val;
            file >> val;
            node->attribute_lists[key].insert(val);
        }
        
        file.ignore();
    }

    // Load child nodes recursively
    string left_uuid, right_uuid;
    file >> left_uuid >> right_uuid;

    node->left_child = loadNodeFromFile(left_uuid);
    node->right_child = loadNodeFromFile(right_uuid);

    return node;
}

void EseManKDT::cleanNodesFromMemory() {
    // Save event_data_attributes to file
    ofstream attr_file(node_storage_base_path + "/event_data_attributes.dat");
    if (attr_file.is_open()) {
        // Write number of attributes
        attr_file << event_data_attributes.size() << "\n";
        for (const auto& [key, mapper] : event_data_attributes) {
            // Write key and number of tracks
            attr_file << key << " " << mapper.size() << "\n";
            // Write each track
            for (size_t i = 0; i < mapper.size(); i++) {
                attr_file << mapper[i] << "\n";
            }
        }
        attr_file.close();
    }
    // Save event_tracks to file
    ofstream tracks_file(node_storage_base_path + "/event_tracks.dat");
    if (tracks_file.is_open()) {
        tracks_file << event_tracks.size() << "\n";
        for (size_t i = 0; i < event_tracks.size(); i++) {
            tracks_file << event_tracks[i] << "\n";
        }
        tracks_file.close();
    }
    // Save eseman_node_uuids to file
    ofstream uuid_file(node_storage_base_path + "/eseman_node_uuids.dat");
    if (uuid_file.is_open()) {
        uuid_file << eseman_node_uuids.size() << "\n";
        for (const auto& uuid : eseman_node_uuids) {
            uuid_file << uuid << "\n";
        }
        uuid_file.close();
    }
    // Save each node to file
    for (auto node : event_data_nodes) {
        saveNodeToFile(node);
        deleteTree(node);
    }
    event_data_nodes.clear();
    event_data_attributes.clear();
    event_tracks.cleanMemory();
    eseman_node_uuids.clear();
}

void EseManKDT::reloadNodesFromFile() {
    // Load event_data_attributes from file
    event_data_attributes.clear();
    ifstream attr_file(node_storage_base_path + "/event_data_attributes.dat");
    if (attr_file.is_open()) {
        int attr_count;
        attr_file >> attr_count;
        for (int i = 0; i < attr_count; i++) {
            string key;
            int track_count;
            attr_file >> key >> track_count;
            event_data_attributes.insert(make_pair(key, StringIndexMapper()));
            for (int j = 0; j < track_count; j++) {
                string track;
                attr_file >> track;
                event_data_attributes[key].insert(track);
            }
        }
        attr_file.close();
    }

    // Load event_tracks from file
    ifstream tracks_file(node_storage_base_path + "/event_tracks.dat");
    if (tracks_file.is_open()) {
        int track_count;
        tracks_file >> track_count;
        for (int i = 0; i < track_count; i++) {
            string track;
            tracks_file >> track;
            event_tracks.insert(track);
        }
        tracks_file.close();
    }

    // Load eseman_node_uuids from file
    eseman_node_uuids.clear();
    ifstream uuid_file(node_storage_base_path + "/eseman_node_uuids.dat");
    if (uuid_file.is_open()) {
        int uuid_count;
        uuid_file >> uuid_count;
        for (int i = 0; i < uuid_count; i++) {
            string uuid;
            uuid_file >> uuid;
            eseman_node_uuids.push_back(uuid);
        }
        uuid_file.close();
    }
    
    // Load each node from file
    event_data_nodes.clear();
    for (string node_uid : eseman_node_uuids) {
        EsemanNode* node = loadNodeFromFile(node_uid);
        if (node) {
            event_data_nodes.push_back(node);
            PRINTLOG("Loaded node with UUID: " << node->uuid);
        } else {
            PRINTLOG("Failed to load node with UUID: " << node_uid);
        }
    }
}

void test_KDT_build() {
    EseManKDT *kdt = new EseManKDT();
    // // kdt.insertDataIntoTree(5.0, 6.0, "12");
    // // kdt.insertDataIntoTree(1.0, 2.0, "12");
    // // kdt.insertDataIntoTree(1100.0, 1110.0, "12");
    // // kdt.insertDataIntoTree(11.0, 12.0, "12");
    // // kdt.insertDataIntoTree(14.0, 18.0, "12");


    // string input_file_path = "/mnt/c/Users/sayef/IdeaProjects/traveler-integrated/data_handler/cgal_libs/cgal_server/location_data/";
    // fstream input_file(input_file_path + "9.location");
    // if (!input_file.is_open()) {
    //     PRINTLOG("Failed to open input file");
    //     return;
    // }
    // uint64_t start_time, end_time;
    // while(input_file >> start_time >> end_time) {
    //     string primitive_name = "first";
    //     if (start_time == 74755483) {
    //         primitive_name = "second";
    //     }
    //     string interval_id = "100000";
    //     if (start_time == 74755483) {
    //         interval_id = "320000";
    //     }
    //     kdt->insertDataIntoTree(start_time, end_time, "9", primitive_name, interval_id);
    // }
    // input_file.close();

    // fstream input_file1(input_file_path + "1.location");
    // if (!input_file1.is_open()) {
    //     PRINTLOG("Failed to open input file");
    //     return;
    // }
    // while(input_file1 >> start_time >> end_time) {
    //     kdt->insertDataIntoTree(start_time, end_time, "1", "first", "iid_1");
    // }
    // input_file1.close();

    // fstream input_file2(input_file_path + "14.location");
    // if (!input_file2.is_open()) {
    //     PRINTLOG("Failed to open input file");
    //     return;
    // }
    // while(input_file2 >> start_time >> end_time) {
    //     string primitive_name = "first";
    //     if (start_time == 74755483) {
    //         primitive_name = "second";
    //     }
    //     kdt->insertDataIntoTree(start_time, end_time, "14", primitive_name, "iid_1");
    // }
    // input_file2.close();

    // kdt->buildKDT();
    // // kdt.printKDTDot();
    // // kdt.printKDTDot();
    // // kdt.binnedRangeQuery(1, 1200, 
    // //                     12, 12, 
    // //                     100);
    // // kdt->addPrimitiveFilter("first");
    // // kdt->addIDFilter("320000");
    // kdt->binnedRangeQuery(-1305029698, 2753780939, 
    //                         9, 9, 
    //                         10);
    // string i_id = kdt->findNearestEvent(83188392, 9);
    // cout << "Interval ID: " << i_id << endl;
    int cm;
    // cin >> cm;
    kdt->node_storage_base_path = "/mnt/d/tmp/eseman_nodes";
    // kdt->cleanNodesFromMemory();
    // PRINTLOG("ESEman KDT cleaned from memory, now reloading from file");
    // cin >> cm;
    kdt->reloadNodesFromFile();
    PRINTLOG("Reloading finished");
    cin >> cm;
    kdt->binnedRangeQuery(-1305029698, 2753780939, 
                            9, 9, 
                            10);
    string i_id = kdt->findNearestEvent(83188390, 9);
    cout << "Interval ID: " << i_id << endl;
}

// void find_bitwise_insertion_index(int num) {
//     int first = num / 64;
//     int second = num % 64;
//     PRINTLOG("Finding bitwise insertion index for number: " << first << " " << second);

// }

// void test_bitwise_insertion() {
//     srand(time(nullptr));  // Seed with current time
//     int max_limit = 1000;// INT_MAX;
//     vector<int> numbers = {0, 32, 63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1023, 1024, 1025};
//     // for (int i = 0; i < 10; i++) {
//     //     numbers.push_back(rand() % max_limit);  // Generate numbers in range [0, INT_MAX)
//     // }
//     for (int num : numbers) {
//         PRINTLOG("Number: " << num);
//         find_bitwise_insertion_index(num);
//     }

// }

#ifdef TESTING
int main() {
    PRINTLOG("hello inside eseman kdt");
    // test_event_tracks();
    PRINTLOG("Printing resutls from RAM before cleaning");
    test_KDT_build();
    // test_bitwise_insertion();
    PRINTLOG("Eseman KDT finished!");
    return 0;
}
#endif