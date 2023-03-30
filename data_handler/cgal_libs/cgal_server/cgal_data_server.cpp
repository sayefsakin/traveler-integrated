#include <iostream>
#include <vector>
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
typedef CGAL::Orthtrees::Preorder_traversal Preorder_traversal;

typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point_d;
typedef CGAL::Search_traits_3<K> Traits;
typedef CGAL::Kd_tree<Traits> Kdtree;
typedef CGAL::Fuzzy_iso_box<Traits> Fuzzy_iso_box;

typedef CGAL::Cartesian<double> Kert;
typedef CGAL::Segment_tree_map_traits_2<Kert, string> KertTraits;
typedef CGAL::Segment_tree_2<KertTraits > Segment_tree_2_type;


typedef KertTraits::Interval Interval;
typedef KertTraits::Pure_interval Pure_interval;
typedef KertTraits::Key Key;
typedef pair<Point_2, Point_2> Point_pairs;


typedef map<string, int> Primtive_mapping;
typedef vector<string> Primitive_reverse_mapping;

typedef CGAL::Orthogonal_k_neighbor_search<Traits> Kd_tree_search;
typedef Kd_tree_search::Tree NNKdtree;

Pure_interval getPurIntervalFromPoint(Point_2 p, Point_2 q) {
    return Pure_interval(Key(p.x(),(p.y()*2)-1), Key(q.x(),q.y()*2));
}
Point_pairs getPointIntervalFromPureInterval(Pure_interval pi) {
    Point_2 p(pi.first.x(), (pi.first.y()+1)/2);
    Point_2 q(pi.second.x(), pi.second.y()/2);
    return Point_pairs(p,q);
}

void testSearchQueries(Kdtree *kdtree, Segment_tree_2_type *Segment_tree_2, int minId, int maxId, NNKdtree *nnkdtree) {

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
    uint64_t two = 1;
    while(1) {
        Point_d nqd(q.x()+two, q.y(), maxId);
        Fuzzy_iso_box exact_range(pd,nqd);
        kdtree->search( back_inserter( result ), exact_range);
        if(DEBUG) cout << "kd tree points are with size: " << result.size() <<  " " << two << endl;
        copy (result.begin(), result.end(), ostream_iterator<Point_d>(cout,"\n") );
        if(DEBUG) cout << endl;
        if(result.size()>0 || two > (1<<31)) break;
        two <<= 1;
    }
    Kd_tree_search search((*nnkdtree), pd, 1);

    cout << "nearest neighbor " << (search.end()-1)->first << endl;

/*
    if(DEBUG) cout << "KD Tree" << endl;
    if(DEBUG) cout << kdtree << endl;
*/
    //segmentTreeTest(points_2d);
    //buildSegmentTreeFromPointVector(points_2d);
    vector<Interval> OutputList1;
    Interval a=Interval(Pure_interval(Key(p.x(),(p.y()*2)-1), Key(q.x(),q.y()*2)),"z");
    Segment_tree_2->window_query(a,std::back_inserter(OutputList1));
    vector<Interval>::iterator j = OutputList1.begin();
    if(DEBUG) cout << "\nwindow_query with segment tree result size: " << OutputList1.size() << endl;;
    while(j!=OutputList1.end()){
        Point_pairs pp = getPointIntervalFromPureInterval((*j).first);
        if(DEBUG) cout << pp.first << " " << pp.second <<  " id " << (*j).second << endl;
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
    int valread = read(new_socket, buffer, MSG_SIZE_IN_BYTE);
    int msg_size = atoi(buffer);
    if(DEBUG) printf("%d\n", msg_size);
    Document d;
    if(!msg_size) return d;
    buffer = (char *)malloc(msg_size+1);
    memset(buffer, 0, msg_size+1);
    valread = read(new_socket, buffer, msg_size);
    if(DEBUG) printf("%s\n", buffer);
    d.Parse(buffer);
    memset(buffer, 0, msg_size+1);
    buffer = NULL;
    return d;
}

Document kdTreeGetAttributeQuery(Kdtree *nnkdtree, uint64_t cTime, uint64_t cLocation, uint64_t minId, uint64_t maxId) {
    vector<Point_d> result;
    Point_2 p(cTime, cLocation);
    Point_2 q(cTime+1, cLocation);

    Point_d pd(p.x(), p.y(), minId);
    Point_d qd(q.x(), q.y(), maxId);

    if(DEBUG) cout << "doing event attribute query (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    uint64_t two = 1;
    vector<Point_d> leftResult;
    while(1) {
        Point_d npd(p.x()-two, p.y(), minId);
        Point_d nqd(q.x(), q.y(), maxId);
        Fuzzy_iso_box exact_range(npd,nqd);
        nnkdtree->search( back_inserter( leftResult ), exact_range);
        if(DEBUG) cout << "kd tree points are with size: " << leftResult.size() <<  " " << two << endl;
        copy (leftResult.begin(), leftResult.end(), ostream_iterator<Point_d>(cout,"\n") );
        if(DEBUG) cout << endl;
        if(leftResult.size()>0 || two > (1<<30)) break;
        two <<= 1;
    }
    if(leftResult.size() > 1) {
        int64_t diff = abs(leftResult[0].x() - p.x());
        int ind = 0;
        for(int i = 1; i<leftResult.size(); i++) {
            if(abs(leftResult[i].x() - p.x()) < diff) {
                diff = abs(leftResult[i].x() - p.x());
                ind = i;
            }
        }
        leftResult[0] = leftResult[ind];
    }

    if(leftResult.size() > 0) {
        if(DEBUG) cout << "nearest neighbor in left " << leftResult[0].z() << endl;

        vector<Point_d> rightResult;
        while(1) {
            Point_d npd(p.x(), p.y(), minId);
            Point_d nqd(q.x()+two, q.y(), maxId);
            Fuzzy_iso_box exact_range(npd,nqd);
            nnkdtree->search( back_inserter( rightResult ), exact_range);
            if(DEBUG) cout << "kd tree points are with size: " << rightResult.size() <<  " " << two << endl;
            copy (rightResult.begin(), rightResult.end(), ostream_iterator<Point_d>(cout,"\n") );
            if(DEBUG) cout << endl;
            if(rightResult.size()>0 || two > (1<<30)) break;
            two <<= 1;
        }
        if(rightResult.size() > 1) {
            int64_t diff = abs(rightResult[0].x() - p.x());
            int ind = 0;
            for(int i = 1; i<rightResult.size(); i++) {
                if(abs(rightResult[i].x() - p.x()) < diff) {
                    diff = abs(rightResult[i].x() - p.x());
                    ind = i;
                }
            }
            rightResult[0] = rightResult[ind];
        }
        if(DEBUG) cout << "nearest neighbor in right " << rightResult[0].z() << endl;
        if(leftResult[0].z() == rightResult[0].z())
            result = leftResult;
    }

    if(result.size()>0) if(DEBUG) cout << "actual nearest neighbor " << result[0].z() << endl;

    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();
    if(result.size()>0) {
        Value val(kObjectType);
        //string begin_string = to_string(int64_t(search.begin()->first.z()));
        string begin_string = to_string(int64_t(result[0].z()));
        val.SetString(begin_string.c_str(), static_cast<SizeType>(begin_string.length()), allocator);
        document.AddMember("event_id", val, allocator);
    }
    return document;
}

Document kdTreeSearchQuery( Kdtree *kdtree,
                        uint64_t time_begin,
                        uint64_t time_end,
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
    kdtree->search( back_inserter( result ), exact_range);
    if(DEBUG) cout << "kd tree points are with size: " << result.size() << endl;
    copy (result.begin(), result.end(), ostream_iterator<Point_d>(cout,"\n") );
    if(DEBUG) cout << endl;

    Document document;
    document.SetObject();

    Value a(kArrayType);

    Document::AllocatorType& allocator = document.GetAllocator();

    for(int i = 0; i < result.size(); i++) {
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

Document sgmntTreeGetAttributeQuery(Segment_tree_2_type *Segment_tree_2,
                        uint64_t cTime,
                        uint64_t cLocation,
                        uint64_t minId,
                        uint64_t maxId) {

    vector<Interval> OutputList1;
    Point_d p((double)cTime, (double)cLocation, minId);
    Point_d q((double)cTime+1, (double)cLocation, maxId);

    if(DEBUG) cout << "doing window query with segment tree (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    Interval a=Interval(Pure_interval(Key(p.x(),(p.y()*2)-1), Key(q.x(),q.y()*2)),"z");
    Segment_tree_2->window_query(a,std::back_inserter(OutputList1));
    vector<Interval>::iterator j = OutputList1.begin();
    if(DEBUG) cout << "\n get attribute query with segment tree result size: " << OutputList1.size() << endl;;

    Document document;
    document.SetObject();
    Document::AllocatorType& allocator = document.GetAllocator();

    while(j!=OutputList1.end()){
        Point_pairs pp = getPointIntervalFromPureInterval((*j).first);
        Value val(kObjectType);
        string begin_string((*j).second);
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

Document sgmntTreeSearchQuery(Segment_tree_2_type *Segment_tree_2,
                        uint64_t time_begin,
                        uint64_t time_end,
                        uint64_t location_begin,
                        uint64_t location_end,
                        uint64_t minId,
                        uint64_t maxId) {

    vector<Interval> OutputList1;
    Point_d p((double)time_begin, (double)location_begin, minId);
    Point_d q((double)time_end, (double)location_end, maxId);

    if(DEBUG) cout << "doing window query with segment tree (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
    Interval a=Interval(Pure_interval(Key(p.x(),(p.y()*2)-1), Key(q.x(),q.y()*2)),"z");
    Segment_tree_2->window_query(a,std::back_inserter(OutputList1));
    vector<Interval>::iterator j = OutputList1.begin();
    if(DEBUG) cout << "\nwindow_query with segment tree result size: " << OutputList1.size() << endl;;

    Document document;
    document.SetObject();
    Value a1(kArrayType);
    Document::AllocatorType& allocator = document.GetAllocator();

    while(j!=OutputList1.end()){
        Point_pairs pp = getPointIntervalFromPureInterval((*j).first);

        Value obj(kObjectType);
        Value val(kObjectType);

        string time_string = to_string(pp.first.x());
        val.SetString(time_string.c_str(), static_cast<SizeType>(time_string.length()), allocator);
        obj.AddMember("time", val, allocator);

        string location_string = to_string(pp.first.y());
        val.SetString(location_string.c_str(), static_cast<SizeType>(location_string.length()), allocator);
        obj.AddMember("location", val, allocator);

        string iid_string((*j).second);
        val.SetString(iid_string.c_str(), static_cast<SizeType>(iid_string.length()), allocator);
        obj.AddMember("interval_id", val, allocator);

        a1.PushBack(obj, allocator);


        Value obj1(kObjectType);
        Value val1(kObjectType);

        string time_string1 = to_string(pp.second.x());
        val1.SetString(time_string1.c_str(), static_cast<SizeType>(time_string1.length()), allocator);
        obj1.AddMember("time", val1, allocator);

        string location_string1 = to_string(pp.second.y());
        val1.SetString(location_string1.c_str(), static_cast<SizeType>(location_string1.length()), allocator);
        obj1.AddMember("location", val1, allocator);

        string iid_string1((*j).second);
        val1.SetString(iid_string1.c_str(), static_cast<SizeType>(iid_string1.length()), allocator);
        obj1.AddMember("interval_id", val1, allocator);

        a1.PushBack(obj1, allocator);

        j++;
    }

    document.AddMember("data", a1, allocator);
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

Document processReceivedRequest(Kdtree *kdtree,
                                NNKdtree *nkdtree,
                                Segment_tree_2_type *Segment_tree_2,
                                Document *d,
                                uint64_t minId, uint64_t maxId,
                                uint64_t minLocation, uint64_t maxLocation) {
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

    string GETDATAINRANGE("GetDataInRange");
    string GETEVENTATTRIBUTE("GetEventAttribute");
    string KDTREE("kd_tree");
    string SGTREE("segment_tree");

    if(qCommand == GETDATAINRANGE) {

        uint64_t time_begin = stol(((*d)["begin"].GetString())) < 0 ? 0 : stoul(((*d)["begin"].GetString()));
        uint64_t time_end = stol(((*d)["end"].GetString())) < 0 ? 0 : stoul(((*d)["end"].GetString()));

        uint64_t location_begin = minLocation;
        uint64_t location_end = maxLocation;
        vector<uint64_t> locationsList;
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

        if(ds_request == KDTREE) {
            if(DEBUG) cout << "got KD Tree request" << endl;
            return kdTreeSearchQuery(kdtree, time_begin, time_end, location_begin, location_end, minId, maxId);
        } else if(ds_request == SGTREE) {
            if(DEBUG) cout << "got Segment Tree request" << endl;
            return sgmntTreeSearchQuery(Segment_tree_2, time_begin, time_end, location_begin, location_end, minId, maxId);
        } else {
            if(DEBUG) cout << "invalid ds request" << endl;
        }

    } else if(qCommand == GETEVENTATTRIBUTE) {

        uint64_t cTime = stol(((*d)["time"].GetString())) < 0 ? 0 : stoul(((*d)["time"].GetString()));
        uint64_t cLocation = stol((*d)["location"].GetString());

        if(ds_request == KDTREE) {
            if(DEBUG) cout << "got KD Tree request" << endl;
            return kdTreeGetAttributeQuery(kdtree, cTime, cLocation, minId, maxId);
        } else if(ds_request == SGTREE) {
            if(DEBUG) cout << "got Segment Tree request" << endl;
            return sgmntTreeGetAttributeQuery(Segment_tree_2, cTime, cLocation, minId, maxId);
        } else {
            if(DEBUG) cout << "invalid ds request" << endl;
        }
    }
    return queryResults;
}

void startServerListening(Kdtree *kdtree,
                        NNKdtree *nkdtree,
                        Segment_tree_2_type *Segment_tree_2,
                        uint64_t minId, uint64_t maxId,
                        uint64_t minLocation, uint64_t maxLocation) {
    int PORT = 8080;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    //const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
    string jstring = "{\"project\":\"sayef\",\"stars\":10}";
    const char* json = jstring.data();

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

    cout << "Server now listening" << endl;
    while(1) {
        if ((new_socket
                = accept(server_fd, (struct sockaddr*)&address,
                        (socklen_t*)&addrlen))
                < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

        Document d = rcvOverTheSocket(new_socket);
        Document queryResults = processReceivedRequest(kdtree, nkdtree, Segment_tree_2, &d, minId, maxId, minLocation, maxLocation);

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

int main()
{
    vector<string> primitives;
    UrlParser urlparser;
    //urlparser.urlString = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/intervalHistograms?bins=100";
    //urlparser.baseUrl = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/primitives";
    urlparser.baseUrl = "http://localhost:8000";
    //urlparser.datasetId = "772c7330-d4eb-485b-866a-3b315063f9af";// "589ca754-ef75-426c-8d51-841cc61dc84a";//"9b9d5286-736c-481a-ba29-0f871979967c";
    urlparser.datasetId = "8b3289c9-a740-4091-a56d-e4d55af526b5";//kmeans
    urlparser.travelerApi = "intervals";//"primitives";
/*
    urlparser.urlParameters.Parse(R""""({
                                      "begin":"192648732",
                                      "end":"255840252"
                                      })"""");
*/
    Document fetchedData = urlparser.fetchContentFromURL();
    if(fetchedData.IsNull() || kArrayType != fetchedData.GetType()) { if(DEBUG) cout << "nothing is in the content" << endl; return 0;}

    Point_vector points_2d;
    vector<Interval> InputList;
    //int numberOfEvents = 1000;
    int totalIntervals = 0;
    Kdtree kdtree;
    Primtive_mapping primitiveMapping;
    string cPrimitive;
    int totalPrimitives = 0;
    int cPrimitiveNumber = -1;

    uint64_t minId = 1000000000;
    uint64_t maxId = 0;
    uint64_t minLocation = 1000000000;
    uint64_t maxLocation = 0;
    vector<Point_d> nnpoints;
    for (auto& v : fetchedData.GetArray()) {
        Point_2 interval_enter((double)v.GetObject()["enter"]["Timestamp"].GetInt64(), stod(v.GetObject()["Location"].GetString()));
        Point_2 interval_end((double)v.GetObject()["leave"]["Timestamp"].GetInt64(), stod(v.GetObject()["Location"].GetString()));

        cPrimitive = v.GetObject()["Primitive"].GetString();
        cPrimitiveNumber = -1;
        Primtive_mapping::iterator lb = primitiveMapping.lower_bound(cPrimitive);
        if(lb != primitiveMapping.end() && !(primitiveMapping.key_comp()(cPrimitive, lb->first))) {
            cPrimitiveNumber = lb->second;
        } else {
            primitiveMapping.insert(lb, Primtive_mapping::value_type(cPrimitive, ++totalPrimitives));
            cPrimitiveNumber = totalPrimitives;
        }

        //points_2d.emplace_back(interval_enter)

        uint64_t locationId = stoul(v.GetObject()["Location"].GetString());
        minLocation = min(minLocation, locationId);
        maxLocation = max(maxLocation, locationId);
        uint64_t intervalId = stoul(v.GetObject()["intervalId"].GetString());
        minId = min(minId, intervalId);
        maxId = max(maxId, intervalId);

        kdtree.insert(Point_d(interval_enter.x(), interval_enter.y(), intervalId));
        kdtree.insert(Point_d(interval_end.x(), interval_end.y(), intervalId));

        nnpoints.push_back(Point_d(interval_enter.x(), interval_enter.y(), intervalId));
        nnpoints.push_back(Point_d(interval_end.x(), interval_end.y(), intervalId));

        InputList.emplace_back(Interval(getPurIntervalFromPoint(interval_enter, interval_end), v.GetObject()["intervalId"].GetString()));
        totalIntervals++;
        if(totalIntervals % 2500 == 0)
            cout << ".";
        if(totalIntervals % 100000 == 0)
            cout << " processed " << totalIntervals << " intervals" << endl;
        //numberOfEvents--;
        //if(numberOfEvents<=0) break;
    }

    NNKdtree nnkdtree(nnpoints.begin(), nnpoints.end());
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    nnkdtree.build();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "KD Tree build time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;

    begin = std::chrono::steady_clock::now();
    Segment_tree_2_type Segment_tree_2(InputList.begin(),InputList.end());
    end = std::chrono::steady_clock::now();
    std::cout << "Segment Tree build time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
    cout << "kd tree and segment tree build done with total interval count: " << totalIntervals <<  " " << cPrimitiveNumber << endl;
    //testSearchQueries(&kdtree, &Segment_tree_2, minId, maxId, &nnkdtree);

    startServerListening(&kdtree, &nnkdtree, &Segment_tree_2, minId, maxId, minLocation, maxLocation);
    return EXIT_SUCCESS;
}
