#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Random.h>
#include <CGAL/Orthtree.h>
#include <CGAL/Quadtree.h>

#include <CGAL/Kd_tree.h>
#include <CGAL/Search_traits_2.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/Fuzzy_iso_box.h>

#include <CGAL/Cartesian.h>
#include <CGAL/Segment_tree_k.h>
#include <CGAL/Range_segment_tree_traits.h>

#include <CGAL/property_map.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include "curl_get.cpp"
#include "optimized_binned_kdt.h"
#include "agglomerate_clustering.h"
#include "eseman_kdt.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#define MSG_SIZE_IN_BYTE 16

using namespace std;

// Type Declarations
typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_2 Point_2;
typedef std::vector<Point_2> Point_vector;

typedef CGAL::Quadtree<Kernel, Point_vector> Quadtree;
// typedef CGAL::Orthtrees::Preorder_traversal Preorder_traversal;

typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point_d;
typedef CGAL::Search_traits_3<K> Traits;

typedef CGAL::Sliding_midpoint<Traits> SlidingTraits;
typedef CGAL::Sliding_fair<Traits> SlidingFairTraits;
typedef CGAL::Sliding_midpoint<Traits> MidpointMaxSpreadTraits;

typedef CGAL::Kd_tree<Traits, MidpointMaxSpreadTraits> Kdtree;
typedef CGAL::Fuzzy_iso_box<Traits> Fuzzy_iso_box;

typedef boost::tuple<string, string> BString_and_string;
typedef CGAL::Cartesian<double> Kert;
typedef CGAL::Segment_tree_map_traits_3<Kert, string> KertTraits;
typedef CGAL::Segment_tree_3<KertTraits > Segment_tree_3_type;


typedef KertTraits::Interval Interval;
typedef KertTraits::Pure_interval Pure_interval;
typedef KertTraits::Key Key;
typedef pair<Point_d, Point_d> Point_pairs;


typedef map<string, int64_t> Primtive_mapping;
typedef vector<string> Primitive_reverse_mapping;

typedef CGAL::Orthogonal_k_neighbor_search<Traits> Kd_tree_search;
typedef Kd_tree_search::Tree NNKdtree;

const string KDTREE("kd_tree");
const string SGTREE("segment_tree");
const string AGCLUSTER("agglomerative_clustering");
const string ESEMAN("eseman_kdt");
const string GETDATAINRANGE("GetDataInRange");
const string GETEVENTATTRIBUTE("GetEventAttribute");
const string GETCHILDREN("GetChildren");

Pure_interval getPurIntervalFromPoint(Point_d p, Point_d q) {
    return Pure_interval(Key(p.x(),(p.y()*2)-1, (p.z()*2)-1), Key(q.x(),q.y()*2,q.z()*2));
}
Point_pairs getPointIntervalFromPureInterval(Pure_interval pi) {
    Point_d p(pi.first.x(), (pi.first.y()+1)/2, (pi.first.z()+1)/2);
    Point_d q(pi.second.x(), pi.second.y()/2, pi.second.z()/2);
    return Point_pairs(p,q);
}

void testSearchQueries(Kdtree *kdtree, Segment_tree_3_type *Segment_tree_2, int minId, int maxId, string datasetId) {

    list<Point_d> result;
    Point_2 p(236941312, 2);
    Point_2 q(236941313, 2);
    //Point_2 q(255840252, 13);
    Point_2 r(224244492, 12);

    cout << minId << " " << maxId << endl;
    Point_d pd(p.x(), p.y(), minId);
    Point_d qd(q.x(), q.y(), maxId);
    //Point_d rd(224244492, 12);

    if(DEBUG) cout << "doing window query (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    // Searching an exact range
    // using default value 0.0 for epsilon fuzziness parameter
    // Fuzzy_box exact_range(r); replaced by
    std::chrono::steady_clock::time_point kdt_begin = std::chrono::steady_clock::now();
    uint64_t two = 1;
    while(1) {
        Point_d nqd(q.x()+two, q.y(), maxId);
        Fuzzy_iso_box exact_range(pd,nqd);
        kdtree->search( back_inserter( result ), exact_range);
        if(DEBUG) cout << "kd tree points are with size: " << result.size() <<  " " << two << endl;
        if(DEBUG) copy (result.begin(), result.end(), ostream_iterator<Point_d>(cout,"\n") );
        if(DEBUG) cout << endl;
        if(result.size()>0 || two > (uint64_t)(1<<31)) break;
        two <<= 1;
    }
    std::chrono::steady_clock::time_point kdt_end = std::chrono::steady_clock::now();
    std::cout << "KD Tree Splitting" << ", " << datasetId << ", ";
    std::cout << "Fair, " << std::chrono::duration_cast<std::chrono::microseconds>(kdt_end - kdt_begin).count() << std::endl;

    // Kd_tree_search search((*nnkdtree), pd, 1);
    //cout << "nearest neighbor " << (search.end()-1)->first << endl;
    //for(Kd_tree_search::iterator it = search.begin(); it != search.end(); ++it)
    //    std::cout << it->first << " "<< std::sqrt(it->second) << std::endl;
/*
    if(DEBUG) cout << "KD Tree" << endl;
    if(DEBUG) cout << kdtree << endl;
*/
    vector<Interval> OutputList1;
    Interval a=Interval(Pure_interval(Key(p.x(),(p.y()*2)-1,0), Key(q.x(),q.y()*2,0)),"z");
    Segment_tree_2->window_query(a,std::back_inserter(OutputList1));
    vector<Interval>::iterator j = OutputList1.begin();
    if(DEBUG) cout << "\nwindow_query with segment tree result size: " << OutputList1.size() << endl;;
    while(j!=OutputList1.end()){
        Point_pairs pp = getPointIntervalFromPureInterval((*j).first);
        if(DEBUG) cout << pp.first << " " << pp.second 
        <<  " id " << (*j).second
        << endl;
        j++;
    }
}

void sendOverTheSocket(int new_socket, const char *json){
    int64_t maxSocketBuffer = 100000;
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
    char *buffer;
    buffer = (char *)malloc(MSG_SIZE_IN_BYTE);
    read(new_socket, buffer, MSG_SIZE_IN_BYTE);
    int msg_size = atoi(buffer);
    if(DEBUG) printf("%d\n", msg_size);
    Document d;
    if(!msg_size) return d;
    buffer = (char *)malloc(msg_size+1);
    memset(buffer, 0, msg_size+1);
    read(new_socket, buffer, msg_size);
    if(DEBUG) printf("%s\n", buffer);
    d.Parse(buffer);
    memset(buffer, 0, msg_size+1);
    buffer = NULL;
    return d;
}

Document kdTreeGetAttributeQuery(BinnedKDT *binnedKDT, BinnedKDT *neighborKDT, uint64_t cTime, uint64_t cLocation) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    string new_result = binnedKDT->findNearestInterval(cTime, cLocation);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    cout << "KDT," << "ds_attribute," << cTime << "," << cLocation << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
    endl;

    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    if(new_result.length()>0) {
        neighborKDT->getNeighborQuery(stol(new_result));
        Value val(kObjectType);
        val.SetString(new_result.c_str(), static_cast<SizeType>(new_result.length()), allocator);
        document.AddMember("event_id", val, allocator);
    }
    return document;
}

Document agcGetAttributeQuery(AgglomerateClusters *agc, uint64_t cTime, uint64_t cLocation) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    string new_result = agc->findNearestEvent(cTime, cLocation);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    cout << "AGC," << "ds_attribute," << cTime << "," << cLocation << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
    endl;

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

Document esemanGetAttributeQuery(EseManKDT *emk, uint64_t cTime, uint64_t cLocation) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    string new_result = emk->findNearestEvent(cTime, cLocation);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    cout << "ESEMAN," << "ds_attribute," << cTime << "," << cLocation << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
    endl;

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

Document binnedKDTreeSearchQuery( BinnedKDT *tree,
                        int64_t time_begin,
                        int64_t time_end,
                        uint64_t location_begin,
                        uint64_t location_end,
                        uint64_t bins,
                        int64_t primitive) {

    LocDict lResults = tree->binnedRangeQuery(time_begin, time_end, location_begin, location_end, bins, primitive);
    Document d = convertLocDictToDocument(lResults);
    lResults.clear();
    return d;
}

Document binnedAGCSearchQuery( AgglomerateClusters *agc,
    int64_t time_begin,
    int64_t time_end,
    uint64_t location_begin,
    uint64_t location_end,
    uint64_t bins, string primitive) {

    if(primitive.length()>0) {
        agc->addPrimitiveFilter(primitive);
    }
    LocDict lResults = agc->binnedRangeQuery(time_begin, time_end, location_begin, location_end, bins);
    Document d = convertLocDictToDocument(lResults);
    lResults.clear();
    return d;
}

Document binnedESEMANSearchQuery( EseManKDT *emk,
    int64_t time_begin,
    int64_t time_end,
    uint64_t location_begin,
    uint64_t location_end,
    uint64_t bins, string primitive) {

    if(primitive.length()>0) {
        emk->addPrimitiveFilter(primitive);
    }
    LocDict lResults = emk->binnedRangeQuery(time_begin, time_end, location_begin, location_end, bins);
    Document d = convertLocDictToDocument(lResults);
    lResults.clear();
    return d;
}

Document kdTreeSearchQuery( Kdtree *kdtree,
                        int64_t time_begin,
                        int64_t time_end,
                        uint64_t location_begin,
                        uint64_t location_end,
                        uint64_t minId,
                        uint64_t maxId) {
    vector<Point_d> result;
    Point_d p(time_begin, location_begin, minId);
    Point_d q(time_end, location_end, maxId);

    if(DEBUG) cout << "doing window query (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    // Searching an exact range
    // using default value 0.0 for epsilon fuzziness parameter
    // Fuzzy_box exact_range(r); replaced by
    Fuzzy_iso_box exact_range(p,q);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    kdtree->search( back_inserter( result ), exact_range);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    //std::cout << "KD Tree window query time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[ms]" << std::endl;
    cout << "KDT OLD," << "ds_window," << time_begin << "," << time_end << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
    endl;
    if(DEBUG) cout << "kd tree points are with size: " << result.size() << endl;
    if(DEBUG) copy (result.begin(), result.end(), ostream_iterator<Point_d>(cout,"\n") );
    if(DEBUG) cout << endl;

    Document document;
    document.SetObject();

    Value a(kArrayType);

    Document::AllocatorType& allocator = document.GetAllocator();

    for(unsigned i = 0; i < result.size(); i++) {
        Value obj(kObjectType);
        Value val(kObjectType);

        string time_string = to_string(result[i].x());

        val.SetString(time_string.c_str(), static_cast<SizeType>(time_string.length()), allocator);
        obj.AddMember("time", val, allocator);

        string location_string = to_string(result[i].y());
        val.SetString(location_string.c_str(), static_cast<SizeType>(location_string.length()), allocator);
        obj.AddMember("location", val, allocator);

        string iid_string = to_string(result[i].z());
        val.SetString(iid_string.c_str(), static_cast<SizeType>(iid_string.length()), allocator);
        obj.AddMember("interval_id", val, allocator);

        a.PushBack(obj, allocator);
    }

    document.AddMember("data", a, allocator);
    Value obj2(kObjectType);
    Value val(kObjectType);

    string begin_string = to_string(time_begin);
    val.SetString(begin_string.c_str(), static_cast<SizeType>(begin_string.length()), allocator);
    obj2.AddMember("begin", val, allocator);

    string end_string = to_string(time_end);
    val.SetString(end_string.c_str(), static_cast<SizeType>(end_string.length()), allocator);
    obj2.AddMember("end", val, allocator);

    document.AddMember("metadata", obj2, allocator);
/*
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    const char *jsonString = strbuf.GetString();
    if(DEBUG) cout << jsonString << endl;
*/
    return document;
}

void sgmntTreeGetNeighborQuery(Segment_tree_3_type *neighborSGT, int64_t parent_id,
                                int64_t minTime, int64_t maxTime,
                                uint64_t minLocation, uint64_t maxLocation
) {
    vector<Interval> OutputList1;
    Point_d p(minTime, minLocation, parent_id);
    Point_d q(maxTime, maxLocation, parent_id);

    if(DEBUG) cout << "doing window query with segment tree (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    Interval a = Interval(getPurIntervalFromPoint(p,q),"z");

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    neighborSGT->window_query(a,std::back_inserter(OutputList1));
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    // cout << "Children: ";
    // vector<Interval>::iterator j = OutputList1.begin();
    // while(j!=OutputList1.end()){
    //     string intervalId((*j).second);
    //     cout << ", " << intervalId;
    //     j++;
    // }
    // cout << endl;
    cout << "0,SGT," << "ds_neighbor," << parent_id << "," << OutputList1.size() << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
    endl;
}

Document sgmntTreeGetAttributeQuery(
                        Segment_tree_3_type *Segment_tree_3,
                        Segment_tree_3_type *neighborSGT,
                        uint64_t cTime, uint64_t cLocation,
                        int64_t minId, int64_t maxId,
                        int64_t minTime, int64_t maxTime,
                        uint64_t minLocation, uint64_t maxLocation
    ) {

    vector<Interval> OutputList1;
    Point_d p((double)cTime, (double)cLocation, minId);
    Point_d q((double)cTime+1, (double)cLocation, maxId);

    if(DEBUG) cout << "doing window query with segment tree (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    Interval a = Interval(getPurIntervalFromPoint(p,q), "z");
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    Segment_tree_3->window_query(a,std::back_inserter(OutputList1));
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    cout << "SGT," << "ds_attribute," << cTime << "," << cLocation << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << endl;

    vector<Interval>::iterator j = OutputList1.begin();
    if(DEBUG) cout << "\n get attribute query with segment tree result size: " << OutputList1.size() << endl;;

    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();

    while(j!=OutputList1.end()){
        Value val(kObjectType);
        string begin_string((*j).second);
        sgmntTreeGetNeighborQuery(neighborSGT, stol(begin_string),
                                minTime, maxTime, minLocation, maxLocation);
        val.SetString(begin_string.c_str(), static_cast<SizeType>(begin_string.length()), allocator);
        document.AddMember("event_id", val, allocator);
        break;
        j++;
    }
/*
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);

    const char *jsonString = strbuf.GetString();
    if(DEBUG) cout << jsonString << endl;
*/
    return document;
}

LocDict convertSegmentTreeResultToBinnedData(vector<Interval> &outputList, int64_t time_begin, int64_t time_end, uint64_t bins){
    uint64_t bin_size(getBinSize(time_begin, time_end, bins));
    LocDict locDict;
    vector<Interval>::iterator j = outputList.begin();
    while(j!=outputList.end()){
        Point_pairs pp = getPointIntervalFromPureInterval((*j).first);
        uint64_t interval_time_start = (uint64_t)pp.first.x();
        uint64_t interval_loc = (uint64_t)pp.first.y();
        uint64_t interval_time_end = (uint64_t)pp.second.x();
        if(locDict.find(interval_loc) == locDict.end()) {
            vector<double> vd(bins);
            locDict[interval_loc] = vd;
        }
        int64_t startingBin = getBinNumber(time_begin, time_end, bins, interval_time_start);
        int64_t endingBin = getBinNumber(time_begin, time_end, bins, interval_time_end);

        if(startingBin < 0 || endingBin < 0){j++; continue;}
        for(int64_t bin_it = startingBin+1; 
            bin_it < endingBin && bin_it < (int64_t)bins && locDict[interval_loc][bin_it] < 0.5;
            bin_it++) {
            locDict[interval_loc][bin_it] = 1.0;
        }
        if(startingBin < (int64_t)bins && locDict[interval_loc][startingBin] < 0.5) locDict[interval_loc][startingBin] = (interval_time_start % bin_size)?0.5:1.0;
        if(endingBin < (int64_t)bins && locDict[interval_loc][endingBin] < 0.5) locDict[interval_loc][endingBin] = (interval_time_end % bin_size)?0.5:1.0;
        j++;
    }
    return locDict;
}
Document sgmntTreeSearchQuery(Segment_tree_3_type *Segment_tree_3,
                        int64_t time_begin,
                        int64_t time_end,
                        uint64_t location_begin,
                        uint64_t location_end,
                        uint64_t bins,
                        int64_t primitive,
                        int64_t max_primitive_number) {

    int64_t l_p_b = primitive < 0 ? 0 : primitive;
    int64_t u_p_b = primitive < 0 ? max_primitive_number : primitive;
    vector<Interval> OutputList1;
    Point_d p((double)time_begin, (double)location_begin, l_p_b);
    Point_d q((double)time_end, (double)location_end, u_p_b);

    if(DEBUG) cout << "doing window query with segment tree (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    Interval a = Interval(getPurIntervalFromPoint(p,q),"z");

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    Segment_tree_3->window_query(a,std::back_inserter(OutputList1));
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    LocDict locDict = convertSegmentTreeResultToBinnedData(OutputList1, time_begin, time_end, bins);

    cout << "SGT," << "ds_window";
    if(primitive > -1) cout << "_cond";
    cout << "," << time_begin << "," << time_end << "," << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
    endl;
    Document d = convertLocDictToDocument(locDict);
    locDict.clear();
    if(DEBUG) cout << "loc dict cleared" << endl;
    return d;
}

Document processReceivedRequest(BinnedKDT *binnedKDT,
                                Segment_tree_3_type *Segment_tree_3,
                                BinnedKDT *neighborKDT,
                                Segment_tree_3_type *neighborSGT,
                                AgglomerateClusters *agglomerateClusters,
                                EseManKDT *emk,
                                Document *d,
                                int64_t minId, int64_t maxId,
                                int64_t minTime, int64_t maxTime,
                                uint64_t minLocation, uint64_t maxLocation,
                                Primtive_mapping pm) {
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

        uint64_t location_begin = minLocation;
        uint64_t location_end = maxLocation;
        uint64_t bins = 1;
        vector<uint64_t> locationsList;
        string primitive("");
        if((*d).HasMember("locations")) {
            for (SizeType i = 0; i < (*d)["locations"].Size(); i++){
                locationsList.push_back(stol((*d)["locations"][i].GetString()));
            }
            if(DEBUG) cout << locationsList.size() << endl;
            sort(locationsList.begin(), locationsList.end());
            location_begin = locationsList[0];
            location_end = locationsList[locationsList.size()-1];
        }
        if((*d).HasMember("location_end")) {
            location_end = stoul(((*d)["location_end"].GetString()));
        }
        if((*d).HasMember("bins")) {
            bins = stoul(((*d)["bins"].GetString()));
        }
        if((*d).HasMember("primitive")) {
            primitive = ((*d)["primitive"].GetString());
        }

        if(ds_request == KDTREE) {
            if(DEBUG) cout << "got KD Tree request" << endl;
            return binnedKDTreeSearchQuery(binnedKDT, time_begin, time_end, location_begin, location_end, bins, pm[primitive]);
        } else if(ds_request == SGTREE) {
            if(DEBUG) cout << "got Segment Tree request" << endl;
            return sgmntTreeSearchQuery(Segment_tree_3, time_begin, time_end, location_begin, location_end, bins, pm[primitive], maxId);
        } else if(ds_request == AGCLUSTER) {
            if(DEBUG) cout << "got agglomerate cluster request" << endl;
            return binnedAGCSearchQuery(agglomerateClusters, time_begin, time_end, location_begin, location_end, bins, primitive);
        } else if(ds_request == ESEMAN) {
            if(DEBUG) cout << "got eseman cluster request" << endl;
            return binnedESEMANSearchQuery(emk, time_begin, time_end, location_begin, location_end, bins, primitive);
        } else {
            if(DEBUG) cout << "invalid ds request" << endl;
        }

    } else if(qCommand == GETEVENTATTRIBUTE) {

        uint64_t cTime = stol(((*d)["time"].GetString())) < 0 ? 0 : stoul(((*d)["time"].GetString()));
        uint64_t cLocation = stol((*d)["location"].GetString());

        if(ds_request == KDTREE) {
            if(DEBUG) cout << "got KD Tree request" << endl;
            return kdTreeGetAttributeQuery(binnedKDT, neighborKDT, cTime, cLocation);
        } else if(ds_request == SGTREE) {
            if(DEBUG) cout << "got Segment Tree request" << endl;
            return sgmntTreeGetAttributeQuery(Segment_tree_3, neighborSGT, 
                                            cTime, cLocation, 
                                            minId, maxId,
                                            minTime, maxTime,
                                            minLocation, maxLocation);
        } else if(ds_request == AGCLUSTER) {
            if(DEBUG) cout << "got AGC request get attribute request" << endl;
            return agcGetAttributeQuery(agglomerateClusters, cTime, cLocation);
        } else if(ds_request == ESEMAN) {
            if(DEBUG) cout << "got ESEMAN get attribute request" << endl;
            return esemanGetAttributeQuery(emk, cTime, cLocation);
        } else {
            if(DEBUG) cout << "invalid ds request" << endl;
        }
    }
    return queryResults;
}

void startServerListening(BinnedKDT *binnedKDT,
                        Segment_tree_3_type *Segment_tree_3,
                        BinnedKDT *neighborKDT,
                        Segment_tree_3_type *neighborSGT,
                        AgglomerateClusters *agglomerateClusters,
                        EseManKDT *emk,
                        uint64_t tree_build_time,
                        int64_t minId, int64_t maxId,
                        int64_t minTime, int64_t maxTime,
                        uint64_t minLocation, uint64_t maxLocation,
                        Primtive_mapping pm) {
    int PORT = 8080;
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
    cout << "tree build time (micros),ds type,ds query type,ds query begin,ds query end,ds query time (micros)" << endl;
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
        
        cout << tree_build_time << ",";
        Document d = rcvOverTheSocket(new_socket);
        Document queryResults = processReceivedRequest(
            binnedKDT, Segment_tree_3,
            neighborKDT, neighborSGT,
            agglomerateClusters,
            emk,
            &d, minId, maxId, 
            minTime, maxTime,
            minLocation, maxLocation, pm
        );
        if(DEBUG) cout << "received request processing done" << endl;
        StringBuffer qbuffer;
        Writer<StringBuffer> qwriter(qbuffer);
        queryResults.Accept(qwriter);
        sendOverTheSocket(new_socket, qbuffer.GetString());


        // closing the connected socket
        close(new_socket);
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
    bool is_build_dataset = false;
    if(argc>2) is_build_dataset = (string(argv[2]) == "true");

    char *pds = getenv("PROFILED_DS");
    char *pds2 = getenv("HORIZONTAL_RESOLUTION_DIVISOR");
    char *pds3 = getenv("TRAVELER_DATA_BACKUP_LOCATION");
    string profiled_ds = pds == NULL ? string("summed_area_table") : string(pds);
    int horizontal_resolution_divisor = pds2 == NULL ? 1 : atoi(pds2);
    string data_backup_location = pds3 == NULL ? string("/mnt/d/traveler_dataset_backups") : string(pds3);
/*
    urlparser.urlParameters.Parse(R""""({
                                      "begin":"813481624",
                                      "end":"7644595297"
                                      })"""");
*/

    if(profiled_ds == string("summed_area_table")){ cout << "Summed area table only." << endl; return 0;}

    vector<Interval> InputList, NeighborInputList;
    int totalIntervals = 0;
    Primtive_mapping primitiveMapping;
    string cPrimitive;
    int64_t totalPrimitives = 0;
    int64_t cPrimitiveNumber = -1;
    primitiveMapping[""] = -1;

    BinnedKDT *binnedKDT = nullptr;
    BinnedKDT *neighborKDT = nullptr;
    Segment_tree_3_type *Segment_tree_3 = nullptr;
    Segment_tree_3_type *Segment_tree_neighbor_3 = nullptr;
    AgglomerateClusters *agglomerateClusters = nullptr;
    EseManKDT *esemanKDT = nullptr;

    if(profiled_ds == KDTREE) {
        binnedKDT = new BinnedKDT();
        neighborKDT = new BinnedKDT();
    } else if(profiled_ds == SGTREE) {
        Segment_tree_3 = new Segment_tree_3_type();
        Segment_tree_neighbor_3 = new Segment_tree_3_type();
    } else if(profiled_ds == AGCLUSTER) {
        agglomerateClusters = new AgglomerateClusters();
        agglomerateClusters->horizontal_resolution_divisor = horizontal_resolution_divisor;
    } else if(profiled_ds == ESEMAN) {
        esemanKDT = new EseManKDT();
        esemanKDT->horizontal_resolution_divisor = horizontal_resolution_divisor;
        esemanKDT->dataset_id = urlparser.datasetId;
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

    if( profiled_ds == ESEMAN && !is_build_dataset) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if(esemanKDT->reloadNodesFromFile(true)) {
            cout << "ESEMAN dataset loaded from disk" << endl;
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
            std::cout << "EseMAN KDT load from disk time = " << tree_build_time << "[microseconds]" << std::endl;
            esemanKDT->openReadOnlyLMDB();
            startServerListening(binnedKDT, Segment_tree_3, neighborKDT, Segment_tree_neighbor_3,
                                agglomerateClusters, esemanKDT,
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
        Point_d interval_enter((double)v.GetObject()["enter"]["Timestamp"].GetInt64(), 
                                stod(v.GetObject()["Location"].GetString()),
                                (double)cPrimitiveNumber);
        Point_d interval_end((double)v.GetObject()["leave"]["Timestamp"].GetInt64(), 
                                stod(v.GetObject()["Location"].GetString()),
                                (double)cPrimitiveNumber);
        int64_t parent_id = 0;
        if(v.GetObject()["parent"].IsString() && strcmp(v.GetObject()["parent"].GetString(),""))
            parent_id = stol(v.GetObject()["parent"].GetString());

        uint64_t locationId = stoul(v.GetObject()["Location"].GetString());
        minLocation = min(minLocation, locationId);
        maxLocation = max(maxLocation, locationId);
        minId = min(minId, cPrimitiveNumber);
        maxId = max(maxId, cPrimitiveNumber);
        minTime = min(minTime, (int64_t)interval_enter.x());
        maxTime = max(maxTime, (int64_t)interval_end.x());

        // string locFileName = baseLocation + v.GetObject()["Location"].GetString() + ".loc";
        // ofstream outfile(locFileName, ios::app);
        // if(outfile) {
        //     outfile << std::fixed << std::setprecision(0) << interval_enter.x() << std::endl << interval_end.x() << std::endl;
        // }
        // outfile.close();

        if(profiled_ds == KDTREE) {
            binnedKDT->insertDataIntoTree(interval_enter.x(), interval_enter.y(), 
                                        interval_end.x(), interval_end.y(), 
                                        v.GetObject()["intervalId"].GetString(), cPrimitiveNumber);
            neighborKDT->insertDataIntoTree(interval_enter.x(), interval_enter.y(), 
                                        interval_end.x(), interval_end.y(), 
                                        v.GetObject()["intervalId"].GetString(), 
                                        parent_id);
            // if (locationKDT.find(locationId) == locationKDT.end()) {
            //     locationKDT[locationId] = make_unique<BinnedKDT>();
            // }
            // locationKDT[locationId]->insertDataIntoTree(interval_enter.x(), interval_enter.y(), 
            //                             interval_end.x(), interval_end.y(), 
            //                             v.GetObject()["intervalId"].GetString(), cPrimitiveNumber);
        } else if(profiled_ds == SGTREE) {
            InputList.emplace_back(Interval(
                    getPurIntervalFromPoint(interval_enter, interval_end), 
                    v.GetObject()["intervalId"].GetString()
            ));
            NeighborInputList.emplace_back(Interval(
                    getPurIntervalFromPoint(
                        Point_d(interval_enter.x(), interval_enter.y(), parent_id),
                        Point_d(interval_end.x(), interval_end.y(), parent_id)
                    ),
                    v.GetObject()["intervalId"].GetString()
            ));
        } else if(profiled_ds == AGCLUSTER) {
            agglomerateClusters->insertDataIntoTree(interval_enter.x(),
                                                    interval_end.x(),
                                                    v.GetObject()["Location"].GetString(),
                                                    cPrimitive,
                                                    v.GetObject()["intervalId"].GetString());
        } else if(profiled_ds == ESEMAN) {
            esemanKDT->insertDataIntoTree(interval_enter.x(),
                                          interval_end.x(),
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

    if(profiled_ds == KDTREE) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        binnedKDT->tree.build(); // explicitely call build, so that the first query wount spend time in building
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        std::cout << "KD Tree build time = " << tree_build_time << "[microseconds]" << std::endl;
        neighborKDT->tree.build();

        // const string baseDotFileLocation = "/mnt/c/Users/sayef/IdeaProjects/traveler-integrated/data_handler/cgal_libs/cgal_server/figures";
        // std::string outputFilePath = baseDotFileLocation + "/binnedKDT.dot";
        // binnedKDT.outputToDot(outputFilePath);
        // binnedKDT.tree.statistics(std::cout);

        // for (const auto& pair : locationKDT) {
        //     const std::uint64_t& location = pair.first;

        //     string outputFilePath = baseDotFileLocation + "/binnedKDT_" + to_string(location) + ".dot";
        //     locationKDT[location]->tree.build();
        //     cout << "Initialized KDT in location: " << to_string(location) << endl;
        //     locationKDT[location]->outputToDot(outputFilePath);
        //     locationKDT[location]->tree.statistics(std::cout);
        //     cout << endl;
        // }
    } else if(profiled_ds == AGCLUSTER) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        agglomerateClusters->buildAllAggClusters();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        std::cout << "Agglomerate Cluster build time = " << tree_build_time << "[microseconds]" << std::endl;
    } else if(profiled_ds == ESEMAN) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        esemanKDT->buildKDT();
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        std::cout << "EseMAN KDT build time = " << tree_build_time << "[microseconds]" << std::endl;

        esemanKDT->cleanNodesFromMemory(true);
        if(esemanKDT->reloadNodesFromFile(true)) {
            cout << "ESEMAN dataset loaded from disk after building" << endl;
        } else {
            cout << "ESEMAN dataset not found on disk after building" << endl;
        }
    }
    // begin = std::chrono::steady_clock::now();
    // kdtree.build();
    // end = std::chrono::steady_clock::now();
    // std::cout << "Old KD Tree build time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[microseconds]" << std::endl;

    else if(profiled_ds == SGTREE) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        Segment_tree_3->make_tree(InputList.begin(),InputList.end());
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        tree_build_time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        std::cout << "Segment Tree build time = " << tree_build_time << "[microseconds]" << std::endl;
        Segment_tree_neighbor_3->make_tree(NeighborInputList.begin(),NeighborInputList.end());
    }
    cout << "Tree build done" << endl << "Total interval count: " << totalIntervals <<  ", Primitive count: " << cPrimitiveNumber << endl;
    // testSearchQueries(&kdtree, &Segment_tree_3, minId, maxId, urlparser.datasetId);
    // point_with_info_testing();

    if(profiled_ds == ESEMAN) esemanKDT->openReadOnlyLMDB();
    startServerListening(
        binnedKDT, Segment_tree_3, 
        neighborKDT, Segment_tree_neighbor_3,
        agglomerateClusters,
        esemanKDT,
        tree_build_time, 
        minId, maxId, 
        minTime, maxTime,
        minLocation, maxLocation, 
        primitiveMapping
    );
    if(profiled_ds == ESEMAN) esemanKDT->closeReadOnlyLMDB();
    return EXIT_SUCCESS;
}
