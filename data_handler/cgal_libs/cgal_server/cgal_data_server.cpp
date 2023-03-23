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

#include "curl_get.cpp"

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

Pure_interval getPurIntervalFromPoint(Point_2 p, Point_2 q) {
    return Pure_interval(Key(p.x(),(p.y()*2)-1), Key(q.x(),q.y()*2));
}
Point_pairs getPointIntervalFromPureInterval(Pure_interval pi) {
    Point_2 p(pi.first.x(), (pi.first.y()+1)/2);
    Point_2 q(pi.second.x(), pi.second.y()/2);
    return Point_pairs(p,q);
}


int main()
{
    vector<string> primitives;
    UrlParser urlparser;
    //urlparser.urlString = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/intervalHistograms?bins=100";
    //urlparser.baseUrl = "http://localhost:8000/datasets/9b9d5286-736c-481a-ba29-0f871979967c/primitives";
    urlparser.baseUrl = "http://localhost:8000";
    urlparser.datasetId = "589ca754-ef75-426c-8d51-841cc61dc84a";//"9b9d5286-736c-481a-ba29-0f871979967c";
    urlparser.travelerApi = "intervals";//"primitives";

    urlparser.urlParameters.Parse(R""""({
                                      "begin":"192648732",
                                      "end":"255840252"
                                      })"""");
    //cout << "json data " << urlparser.urlParameters["primitive"].GetString() << endl;

    Document fetchedData = urlparser.fetchContentFromURL();
    if(fetchedData.IsNull() || kArrayType != fetchedData.GetType()) { cout << "nothing is in the content" << endl; return 0;}

    Point_vector points_2d;
    vector<Interval> InputList;
    int numberOfEvents = 10;
    Kdtree kdtree;

    int minId = 100000;
    int maxId = 0;
    for (auto& v : fetchedData.GetArray()) {
        Point_2 interval_enter((double)v.GetObject()["enter"]["Timestamp"].GetInt(), stod(v.GetObject()["Location"].GetString()));
        Point_2 interval_end((double)v.GetObject()["leave"]["Timestamp"].GetInt(), stod(v.GetObject()["Location"].GetString()));
        //points_2d.emplace_back(interval_enter)

        int intervalId = stoi(v.GetObject()["intervalId"].GetString());
        minId = min(minId, intervalId);
        maxId = max(maxId, intervalId);

        kdtree.insert(Point_d(interval_enter.x(), interval_enter.y(), intervalId));
        kdtree.insert(Point_d(interval_end.x(), interval_end.y(), intervalId));

        InputList.emplace_back(Interval(getPurIntervalFromPoint(interval_enter, interval_end), v.GetObject()["intervalId"].GetString()));
        numberOfEvents--;
        if(numberOfEvents<=0) break;
    }
    Segment_tree_2_type Segment_tree_2(InputList.begin(),InputList.end());

    list<Point_d> result;
    Point_2 p(192648732, 11);
    Point_2 q(255840252, 13);
    Point_2 r(224244492, 12);


    Point_d pd(192648732, 11, minId);
    Point_d qd(255840252, 13, maxId);
    //Point_d rd(224244492, 12);

    // Searching an exact range
    // using default value 0.0 for epsilon fuzziness parameter
    // Fuzzy_box exact_range(r); replaced by
    Fuzzy_iso_box exact_range(pd,qd);
    kdtree.search( back_inserter( result ), exact_range);
    cout << "kd tree points are" << endl;
    copy (result.begin(), result.end(), ostream_iterator<Point_d>(cout,"\n") );
    cout << endl;
/*
    cout << "KD Tree" << endl;
    cout << kdtree << endl;
*/
    //segmentTreeTest(points_2d);
    //buildSegmentTreeFromPointVector(points_2d);
    vector<Interval> OutputList1;
    Interval a=Interval(Pure_interval(Key(p.x(),(p.y()*2)-1), Key(q.x(),q.y()*2)),"z");
    Segment_tree_2.window_query(a,std::back_inserter(OutputList1));
    vector<Interval>::iterator j = OutputList1.begin();
    std::cout << "\n window_query (1,1),(4,20)\n";
    while(j!=OutputList1.end()){
        Point_pairs pp = getPointIntervalFromPureInterval((*j).first);
        cout << pp.first << " " << pp.second <<  " id " << (*j).second << endl;
        j++;
    }
    return EXIT_SUCCESS;
}
