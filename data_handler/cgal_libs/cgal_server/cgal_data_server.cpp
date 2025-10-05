#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include "curl_get.cpp"
#include "agglomerate_clustering.h"
#include "eseman_kdt.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#define MSG_SIZE_IN_BYTE 16

using namespace std;

typedef map<string, int64_t> Primtive_mapping;
typedef vector<string> Primitive_reverse_mapping;

const string KDTREE("kd_tree");
const string SGTREE("segment_tree");
const string AGCLUSTER("agglomerative_clustering");
const string ESEMAN("eseman_kdt");
const string GETDATAINRANGE("GetDataInRange");
const string SHUTDOWNSERVER("ShutDownServer");
const string GETEVENTATTRIBUTE("GetEventAttribute");
const string GETCHILDREN("GetChildren");

void sendOverTheSocket(int new_socket, const char *json){
    int64_t maxSocketBuffer = 1000000;
    int64_t cJsonSize = strlen(json);
    int cstart = 0;
    char t[30];

    while(cJsonSize > 0) {
        int64_t jsonSize = min(maxSocketBuffer, cJsonSize);
        sprintf(t, "%016lu", jsonSize);
        if(DEBUG) printf("msg leng string %s\n", t);
        send(new_socket, t, MSG_SIZE_IN_BYTE, 0);
        send(new_socket, &json[cstart], jsonSize, 0);
        cJsonSize -= jsonSize;
        cstart += jsonSize;
    }
    cJsonSize = -1;
    sprintf(t, "%016ld", cJsonSize);
    if(DEBUG) printf("msg leng string %s\n", t);
    send(new_socket, t, MSG_SIZE_IN_BYTE, 0);
    if(DEBUG) printf("Hello message sent\n");
}

Document rcvOverTheSocket(int new_socket){
    char* buffer = new char[MSG_SIZE_IN_BYTE];
    ssize_t bytes_read = read(new_socket, buffer, MSG_SIZE_IN_BYTE);
    if (bytes_read < 0) {
        free(buffer);
        if(DEBUG) printf("read failed");
        Document d;
        return d;
    }
    
    int msg_size = atoi(buffer);
    free(buffer);
    
    if(DEBUG) printf("%d\n", msg_size);
    Document d;
    if(!msg_size) return d;
    
    buffer = (char *)malloc(msg_size+1);
    memset(buffer, 0, msg_size+1);
    bytes_read = read(new_socket, buffer, msg_size);
    if (bytes_read < 0) {
        free(buffer);
        if(DEBUG) printf("read failed");
        return d;
    }
    
    if(DEBUG) printf("%s\n", buffer);
    d.Parse(buffer);
    free(buffer);
    return d;
}

Document agcGetAttributeQuery(AgglomerateClusters *agc, uint64_t cTime, uint64_t cLocation, uint64_t tree_build_time) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    string new_result = agc->findNearestEvent(cTime, cLocation);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    cout << tree_build_time << "," << "AGC," << "ds_attribute," 
        << cTime << "," << cLocation << "," 
        << agc->horizontal_resolution_divisor << ","
        << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() 
        << endl;

    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    if(new_result.length()>0) {
        Value val(kObjectType);
        val.SetString(new_result.c_str(), static_cast<SizeType>(new_result.length()), allocator);
        document.AddMember("event_id", val, allocator);
    }
    return document;
}

Document esemanGetAttributeQuery(EseManKDT *emk, uint64_t cTime, uint64_t cLocation, uint64_t tree_build_time) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    string new_result = emk->findNearestEvent(cTime, cLocation);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    if(!new_result.empty())
        cout << tree_build_time << "," << "ESEMAN," << "ds_attribute," 
            << cTime << "," << cLocation << ","
            << emk->horizontal_resolution_divisor << ","
            << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()
            << endl;

    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    if(new_result.length()>0) {
        Value val(kObjectType);
        val.SetString(new_result.c_str(), static_cast<SizeType>(new_result.length()), allocator);
        document.AddMember("event_id", val, allocator);
    }
    return document;
}

Document convertLocDictToDocument(LocDict locDict) {
    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    for ( const auto &myPair : locDict ) {
        Value a(kArrayType);
        uint64_t bins = myPair.second.size();
        for(uint64_t c_bin = 0; c_bin < bins; c_bin++) {
            Value val(kObjectType);
            string d_string = to_string(myPair.second[c_bin]);
            val.SetString(d_string.c_str(), static_cast<SizeType>(d_string.length()), allocator);
            a.PushBack(val, allocator);
        }
        Value lval(kObjectType);
        string loc_string = to_string(myPair.first);
        lval.SetString(loc_string.c_str(), static_cast<SizeType>(loc_string.length()), allocator);
        document.AddMember(lval, a, allocator);
    }
    return document;
}

Document binnedAGCSearchQuery( AgglomerateClusters *agc,
    int64_t time_begin,
    int64_t time_end,
    vector<string> &locations,
    uint64_t bins, string primitive) {

    if(primitive.length()>0) {
        agc->addPrimitiveFilter(primitive);
    }
    LocDict lResults = agc->binnedRangeQuery(time_begin, time_end, locations, bins);
    Document d = convertLocDictToDocument(lResults);
    lResults.clear();
    return d;
}

Document binnedESEMANSearchQuery( EseManKDT *emk,
    int64_t time_begin,
    int64_t time_end,
    vector<string> &locations,
    uint64_t bins, string primitive) {

    if(primitive.length()>0) {
        emk->addPrimitiveFilter(primitive);
    }
    LocDict lResults = emk->binnedRangeQuery(time_begin, time_end, locations, bins);
    Document d = convertLocDictToDocument(lResults);
    lResults.clear();
    return d;
}

Document processReceivedRequest(AgglomerateClusters *agglomerateClusters,
                                EseManKDT *emk,
                                Document *d,
                                int64_t minId, int64_t maxId,
                                int64_t minTime, int64_t maxTime,
                                uint64_t minLocation, uint64_t maxLocation,
                                Primtive_mapping pm, uint64_t tree_build_time) {
    Document queryResults;
    if(!(*d).HasMember("db_store")) {
        if(DEBUG) cout << "please provide db_store for ds request" << endl;
        return queryResults;
    }
    string ds_request((*d)["db_store"].GetString());

    if(!(*d).HasMember("command")) {
        if(DEBUG) cout << "please provide a command for ds request" << endl;
        return queryResults;
    }
    string qCommand((*d)["command"].GetString());

    if(qCommand == GETDATAINRANGE) {

        int64_t time_begin = stol(((*d)["begin"].GetString()));
        int64_t time_end = stol(((*d)["end"].GetString()));

        uint64_t bins = 1;
        vector<string> locationsList;
        string primitive("");
        if((*d).HasMember("locations")) {
            for (SizeType i = 0; i < (*d)["locations"].Size(); i++){
                locationsList.push_back((*d)["locations"][i].GetString());
            }
            if(DEBUG) cout << locationsList.size() << endl;
        }
        if((*d).HasMember("bins")) {
            bins = stoul(((*d)["bins"].GetString()));
        }
        if((*d).HasMember("primitive")) {
            primitive = ((*d)["primitive"].GetString());
        }

        cout << tree_build_time << ",";
        if(ds_request == AGCLUSTER) {
            if(DEBUG) cout << "got agglomerate cluster request" << endl;
            return binnedAGCSearchQuery(agglomerateClusters, time_begin, time_end, locationsList, bins, primitive);
        } else if(ds_request.rfind(ESEMAN, 0) == 0) {
            if(DEBUG) cout << "got eseman cluster request" << endl;
            return binnedESEMANSearchQuery(emk, time_begin, time_end, locationsList, bins, primitive);
        } else {
            if(DEBUG) cout << "invalid ds request" << endl;
        }

    } else if(qCommand == GETEVENTATTRIBUTE) {

        uint64_t cTime = stol(((*d)["time"].GetString())) < 0 ? 0 : stoul(((*d)["time"].GetString()));
        uint64_t cLocation = stol((*d)["location"].GetString());

        if(ds_request.rfind(ESEMAN, 0) == 0) {
            if(DEBUG) cout << "got ESEMAN get attribute request" << endl;
            return esemanGetAttributeQuery(emk, cTime, cLocation, tree_build_time);
        } else {
            if(DEBUG) cout << "invalid ds request" << endl;
        }
    } else if(qCommand == SHUTDOWNSERVER) {
        if(DEBUG) cout << "got shut down request" << endl;

        Document document;
        document.SetObject();
        Document::AllocatorType& allocator = document.GetAllocator();
        string new_result = qCommand;
        if(new_result.length()>0) {
            Value val(kObjectType);
            val.SetString(new_result.c_str(), static_cast<SizeType>(new_result.length()), allocator);
            document.AddMember("event_type", val, allocator);
        }
        return document;
    }
    return queryResults;
}

void startServerListening(AgglomerateClusters *agglomerateClusters,
                        EseManKDT *emk,
                        uint64_t tree_build_time,
                        int64_t minId, int64_t maxId,
                        int64_t minTime, int64_t maxTime,
                        uint64_t minLocation, uint64_t maxLocation,
                        Primtive_mapping pm) {
    int PORT = 8081;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
                sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
            sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server is now listening" << endl;
    cout << "tree build time (micros),ds type,ds query type,ds query begin,ds query end,hrd,ds query time (micros)" << endl;
    ofstream myfile ("cgal_server_check");
    if (myfile.is_open())
    {
        myfile << "This is a line.\n";
        myfile.close();
    }
    while(1) {
        if ((new_socket
                = accept(server_fd, (struct sockaddr*)&address,
                        (socklen_t*)&addrlen))
                < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
        
        Document d = rcvOverTheSocket(new_socket);
        Document queryResults = processReceivedRequest(
            agglomerateClusters,
            emk,
            &d, minId, maxId, 
            minTime, maxTime,
            minLocation, maxLocation, pm, tree_build_time
        );
        if(DEBUG) cout << "received request processing done" << endl;
        StringBuffer qbuffer;
        Writer<StringBuffer> qwriter(qbuffer);
        queryResults.Accept(qwriter);
        sendOverTheSocket(new_socket, qbuffer.GetString());


        // closing the connected socket
        close(new_socket);
        if(queryResults.HasMember("event_type") && string(queryResults["event_type"].GetString()) == SHUTDOWNSERVER) {
            break;
        }
    }

    cout << "Server not listening and shut down" << endl;
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
}

int main(int argc, char *argv[])
{
    UrlParser urlparser;
    //urlparser.urlString = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/intervalHistograms?bins=100";
    //urlparser.baseUrl = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/primitives";
    urlparser.baseUrl = "http://localhost:8000";
    //urlparser.datasetId = "772c7330-d4eb-485b-866a-3b315063f9af";// "589ca754-ef75-426c-8d51-841cc61dc84a";//"9b9d5286-736c-481a-ba29-0f871979967c";
    //urlparser.datasetId = "8b3289c9-a740-4091-a56d-e4d55af526b5";//kmeans
    urlparser.datasetId = "589ca754-ef75-426c-8d51-841cc61dc84a"; // dgemm
    urlparser.travelerApi = "intervals";//"primitives";

    if(argc>1) urlparser.datasetId = argv[1];
    bool is_build_dataset = true;
    if(argc>2) is_build_dataset = (string(argv[2]) == "true");

    char *pds = getenv("PROFILED_DS");
    char *pds2 = getenv("PIXEL_WINDOW");
    char *pds3 = getenv("LMDB_DATA_BACKUP_LOCATION");
    string profiled_ds = pds == NULL ? string("summed_area_table") : string(pds);
    int horizontal_resolution_divisor = pds2 == NULL ? 1 : atoi(pds2);
    string data_backup_location = pds3 == NULL ? string("/mnt/d/traveler_dataset_backups") : string(pds3);

    char *pds4 = getenv("BASE_URL");
    urlparser.baseUrl = pds4 == NULL ? string("http://localhost:8000") : string(pds4);
/*
    urlparser.urlParameters.Parse(R""""({
                                      "begin":"813481624",
                                      "end":"7644595297"
                                      })"""");
*/

    if(profiled_ds == string("summed_area_table")){ cout << "Summed area table only." << endl; return 0;}

    int totalIntervals = 0;
    Primtive_mapping primitiveMapping;
    string cPrimitive;
    int64_t totalPrimitives = 0;
    int64_t cPrimitiveNumber = -1;
    primitiveMapping[""] = -1;

    AgglomerateClusters *agglomerateClusters = nullptr;
    EseManKDT *esemanKDT = nullptr;

    if(profiled_ds == AGCLUSTER) {
        agglomerateClusters = new AgglomerateClusters();
        agglomerateClusters->horizontal_resolution_divisor = horizontal_resolution_divisor;
    } else if(profiled_ds.rfind(ESEMAN, 0) == 0) {
        esemanKDT = new EseManKDT();
        esemanKDT->horizontal_resolution_divisor = horizontal_resolution_divisor;
        if (profiled_ds.size() >= 4 && profiled_ds.compare(profiled_ds.size() - 4, 4, "twod") == 0) {
            esemanKDT->is_vertical_split = true;
        }
        esemanKDT->setDatasetID(urlparser.datasetId);
        esemanKDT->node_storage_base_path = data_backup_location;
    }

    // map<uint64_t, unique_ptr<BinnedKDT>> locationKDT;

    int64_t minId = numeric_limits<int64_t>::max();
    int64_t maxId = 0;
    int64_t minTime = numeric_limits<int64_t>::max();
    int64_t maxTime = 0;
    uint64_t minLocation = 1000000000;
    uint64_t maxLocation = 0;
    uint64_t tree_build_time = 0;

    if( profiled_ds.rfind(ESEMAN, 0) == 0 && !is_build_dataset) {
        esemanKDT->openReadOnlyLMDB();
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if(esemanKDT->reloadNodesFromFile(true)) {
            cout << "ESEMAN dataset loaded from disk" << endl;
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
            std::cout << "EseMAN KDT load from disk time = " << tree_build_time << "[microseconds]" << std::endl;
            
            startServerListening(agglomerateClusters, esemanKDT,
                                tree_build_time, minId, maxId, minTime, maxTime,
                                minLocation, maxLocation, primitiveMapping);
            esemanKDT->closeReadOnlyLMDB();
        } else {
            cout << "ESEMAN dataset not found on disk" << endl;
        }
        return 0;
    }

    Document fetchedData = urlparser.fetchContentFromURL();
    if(fetchedData.IsNull() || kArrayType != fetchedData.GetType()) { if(DEBUG) cout << "nothing is in the content" << endl; return 0;}

    for (auto& v : fetchedData.GetArray()) {
        cPrimitive = v.GetObject()["Primitive"].GetString();
        cPrimitiveNumber = -1;
        Primtive_mapping::iterator lb = primitiveMapping.lower_bound(cPrimitive);
        if(lb != primitiveMapping.end() && !(primitiveMapping.key_comp()(cPrimitive, lb->first))) {
            cPrimitiveNumber = lb->second;
        } else {
            cPrimitiveNumber = totalPrimitives;
            primitiveMapping.insert(lb, Primtive_mapping::value_type(cPrimitive, totalPrimitives++));
        }
        // int64_t parent_id = 0;
        // if(v.GetObject()["parent"].IsString() && strcmp(v.GetObject()["parent"].GetString(),""))
        //     parent_id = stol(v.GetObject()["parent"].GetString());

        uint64_t locationId = stoul(v.GetObject()["Location"].GetString());
        minLocation = min(minLocation, locationId);
        maxLocation = max(maxLocation, locationId);
        minId = min(minId, cPrimitiveNumber);
        maxId = max(maxId, cPrimitiveNumber);
        minTime = min(minTime, (int64_t)v.GetObject()["enter"]["Timestamp"].GetInt64());
        maxTime = max(maxTime, (int64_t)v.GetObject()["leave"]["Timestamp"].GetInt64());

        if(profiled_ds == AGCLUSTER) {
            agglomerateClusters->insertDataIntoTree((double)v.GetObject()["enter"]["Timestamp"].GetInt64(),
                                                    (double)v.GetObject()["leave"]["Timestamp"].GetInt64(),
                                                    v.GetObject()["Location"].GetString(),
                                                    cPrimitive,
                                                    v.GetObject()["intervalId"].GetString());
        } else if(profiled_ds.rfind(ESEMAN, 0) == 0) {
            esemanKDT->insertDataIntoTree((double)v.GetObject()["enter"]["Timestamp"].GetInt64(),
                                          (double)v.GetObject()["leave"]["Timestamp"].GetInt64(),
                                          v.GetObject()["Location"].GetString(),
                                          cPrimitive,
                                          v.GetObject()["intervalId"].GetString());
        } else {
            cout << "Invalid data structure" << endl;
            return EXIT_FAILURE;
        }
        totalIntervals++;
        if(totalIntervals % 2500 == 0)
            cout << ".";
        if(totalIntervals % 100000 == 0)
            cout << " processed " << totalIntervals << " intervals" << endl;
    }
    cout << endl;
    if(profiled_ds == AGCLUSTER) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        agglomerateClusters->buildAllAggClusters();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        std::cout << "Agglomerate Cluster build time = " << tree_build_time << "[microseconds]" << std::endl;
        cout << "Tree build done" << endl << "Total interval count: " << totalIntervals <<  ", Primitive count: " << cPrimitiveNumber << endl;

        startServerListening(
            agglomerateClusters,
            esemanKDT,
            tree_build_time, 
            minId, maxId, 
            minTime, maxTime,
            minLocation, maxLocation, 
            primitiveMapping
        );
    } else if(profiled_ds.rfind(ESEMAN, 0) == 0) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        esemanKDT->buildKDT();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        std::cout << "EseMAN KDT build time = " << tree_build_time << "[microseconds]" << std::endl;

        // esemanKDT->cleanNodesFromMemory(true);
        cout << "Tree build done" << endl << "Total interval count: " << totalIntervals <<  ", Primitive count: " << cPrimitiveNumber << endl;
    }
    return EXIT_SUCCESS;
}
