#include "optimized_binned_kdt.h"
#include <cmath>
#include <algorithm>

int point_with_info_testing()
{
  // const unsigned int K = 5;
  // // generator for random data points in the cube ( (-1,-1,-1), (1,1,1) )
  // Random_points_iterator rpit( 1.0);
  // std::vector<Point_3> points;
  // std::vector<std::string>     indices;
  // std::vector<std::string>     primates;
  
  // // points.push_back(Point_3(*rpit++));
  // // points.push_back(Point_3(*rpit++));
  // // points.push_back(Point_3(*rpit++));
  // // points.push_back(Point_3(*rpit++));
  // // points.push_back(Point_3(*rpit++));
  // // points.push_back(Point_3(*rpit++));
  // // points.push_back(Point_3(*rpit++));

  // points.push_back(Point_3(0.358341, 0.735557, -0.0831224));
  // points.push_back(Point_3(0.791051, -0.326561, -0.0805253));
  // points.push_back(Point_3(-0.695843, 0.592749, -0.630712));
  // points.push_back(Point_3(-0.144725, 0.0143613, -0.509173));
  // points.push_back(Point_3(-0.725944, 0.144757, 0.84278));
  // points.push_back(Point_3(0.855758, 0.624467, 0.0655862));
  // points.push_back(Point_3(-0.335131, 0.0407417, -0.52731));
  // // for(int i=0;i<points.size();i++){
  // //   std::cout << points[i] << std::endl;
  // // }
  // indices.push_back("hello");
  // indices.push_back("honey");
  // indices.push_back("honey");
  // indices.push_back("honey");
  // indices.push_back("faint");
  // indices.push_back("lpthing");
  // indices.push_back("vuaa");

  // primates.push_back("0");
  // primates.push_back("1");
  // primates.push_back("1");
  // primates.push_back("1");
  // primates.push_back("1");
  // primates.push_back("5");
  // primates.push_back("6");

  
  // // Insert number_of_data_points in the tree
  // KNSKDTree tree(boost::make_zip_iterator(boost::make_tuple( points.begin(),indices.begin(), primates.begin())),
  //           boost::make_zip_iterator(boost::make_tuple( points.end(),indices.end(), primates.end())));

  // // search K nearest neighbors
  // Point_3 query(0.0, 0.0, 0.0);
  // Distance tr_dist;
  // K_neighbor_search search(tree, query, K);

  // NN_positive_x_iterator it(search.end(), X_not_positive(), search.begin()), end(search.end(), X_not_positive());

  // for (int j=0; (j < 5)&&(it!=end); ++j,++it){
  //   std::cout << " d(q, nearest neighbor)=  "
  //             << tr_dist.inverse_of_transformed_distance((*it).second) << " , "
  //             << boost::get<0>((*it).first)<< " - " << boost::get<1>((*it).first)
  //             << " = " << boost::get<2>((*it).first)
  //             << std::endl;
  // }

  return 0;
}

// int main() {
//     std::cout << "hello inside cgal binned kdt" << std::endl;
//     // point_with_info_testing();
//     return 0;
// }


void BinnedKDT::insertDataIntoTree(double enter_time, double enter_loc,
                          double end_time, double end_loc,
                          std::string interval_id, int64_t primitive_number){
  int64_t interval_length = end_time - enter_time;
  tree.insert(
    boost::make_tuple(
      Point_3(enter_time, enter_loc, primitive_number), 
      interval_id, 
      interval_length,
      false
    )
  );
  tree.insert(
    boost::make_tuple(
      Point_3(end_time, end_loc, primitive_number),
      interval_id,
      interval_length, 
      true
    )
  );
  if(interval_length > max_interval_length) max_interval_length = interval_length + 1;
  if(primitive_number > max_primitive_number) max_primitive_number = primitive_number;
  max_time = max(max_time, max((int64_t)enter_time, (int64_t)end_time));
  max_location = max((int64_t)enter_loc, max((int64_t)end_loc, max_location));
}

LocDict BinnedKDT::binnedRangeQuery(int64_t time_begin, 
                            int64_t time_end, 
                            uint64_t location_begin, 
                            uint64_t location_end, 
                            uint64_t bins,
                            int64_t primitive) {
  // cout << "hello from binned KDT search" << endl;
  LocDict locDict;
  uint64_t bin_size(getBinSize(time_begin, time_end, bins));

  vector<Point_and_string> result;
  int64_t l_p_b = primitive < 0 ? 0 : primitive;
  int64_t u_p_b = primitive < 0 ? max_primitive_number : primitive;
  Point_3 p(time_begin, location_begin, l_p_b);
  Point_3 q(time_end, location_end, u_p_b);

  // cout << "doing window query (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
  // Searching an exact range
  // using default value 0.0 for epsilon fuzziness parameter
  // Fuzzy_box exact_range(r); replaced by
  PS_Fuzzy_iso_box exact_range(p,q);

  std::chrono::steady_clock::time_point clock_begin = std::chrono::steady_clock::now();
  tree.search( back_inserter( result ), exact_range);
  std::chrono::steady_clock::time_point clock_end = std::chrono::steady_clock::now();
  vector<Point_and_string>::iterator it;
  for(it = result.begin(); it != result.end(); it++) {
    // cout << boost::get<0>(*it) << ", ";
    // string cPrimitive = boost::get<2>(*it);
    // if(primitive != "" && primitive != cPrimitive) continue;
    string intervalId = boost::get<1>(*it);
    int64_t interval_time_start = (boost::get<0>(*it)).x();
    uint64_t interval_loc = (boost::get<0>(*it)).y();
    int64_t interval_length = boost::get<2>(*it);
    if(boost::get<3>(*it)) interval_length *= -1;
    int64_t interval_time_end = interval_time_start + interval_length;
    if(interval_length < 0) swap(interval_time_start, interval_time_end);

    if(locDict.find(interval_loc) == locDict.end()) {
      vector<double> vd(bins);
      locDict[interval_loc] = vd;
    }

    if(interval_time_start < time_begin) interval_time_start = time_begin;
    if(interval_time_start > time_end) continue;
    if(interval_time_end < time_begin) continue;
    if(interval_time_end > time_end) interval_time_end = time_end;
    int64_t startingBin = getBinNumber(time_begin, time_end, bins, interval_time_start);
    int64_t endingBin = getBinNumber(time_begin, time_end, bins, interval_time_end);
    if(startingBin < 0 || endingBin < 0) continue;

    for(int64_t bin_it = startingBin+1;
     bin_it < endingBin && bin_it < (int64_t)bins && locDict[interval_loc][bin_it] < 0.5; 
     bin_it++)
      locDict[interval_loc][bin_it] = 1.0;
    
    if(startingBin < (int64_t)bins && locDict[interval_loc][startingBin] < 0.5) 
      locDict[interval_loc][startingBin] = (interval_time_start % bin_size)?0.5:1.0;
    if(endingBin < (int64_t)bins && locDict[interval_loc][endingBin] < 0.5)
      locDict[interval_loc][endingBin] = (interval_time_end % bin_size)?0.5:1.0;
  }
  
  // for ( uint64_t c_loc = location_begin; c_loc <= location_end; c_loc++ ) {
  //   if(locDict.find(c_loc) == locDict.end()) {
  //     vector<double> vd(bins);
  //     locDict[c_loc] = vd;
  //   }
  //   for(uint64_t c_bin = 0; c_bin < bins; c_bin++) {
  //     if(locDict[c_loc][c_bin] > 0) continue;
  //     int64_t start_time = (c_bin * bin_size) + time_begin;
  //     int64_t end_time = ((c_bin+1) * bin_size) + time_begin;
  //     p = Point_3(start_time, c_loc, 1);
  //     q = Point_3(end_time, c_loc, bin_size - 1);
  //     exact_range = PS_Fuzzy_iso_box(p,q);
  //     boost::optional<Point_and_string> any_point = tree.search_any_point(exact_range);

  //     // result.clear();
  //     // tree.search( back_inserter( result ), exact_range);
  //     // if(result.size() > 0)
  //     if(any_point) {
  //       locDict[c_loc][c_bin] = 0.5;
  //       if(boost::get<3>(*any_point)) continue;
  //       int64_t interval_time_start = (boost::get<0>(*any_point)).x();
  //       // uint64_t interval_loc = (boost::get<0>(*any_point)).y();
  //       int64_t interval_length = (boost::get<0>(*any_point)).z();
  //       int64_t interval_time_end = interval_time_start + interval_length;
  //       if(end_time < interval_time_end && c_bin + 1 < bins)
  //         locDict[c_loc][++c_bin] = 0.5;
  //     }
  //   }
  // }
  

  cout << "KDT," << "ds_window";
  if(primitive > -1) cout << "_cond";
  cout << "," << time_begin << "," << time_end << "," << std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_begin).count() <<
    endl;
  // std::cout << "KD Tree window query time = " << std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_begin).count() << "[ms]" << std::endl;
  // for ( const auto &myPair : locDict ) {
  //     std::cout << myPair.first << " = ";
  //     copy (myPair.second.begin(), myPair.second.end(), ostream_iterator<double>(cout,"\n") );
  //     cout << endl;
  // }
  result.clear();
  return locDict;
}

string BinnedKDT::findNearestInterval(int64_t c_time, uint64_t c_location) {
  string ret_result("");

  Point_3 p(c_time, c_location, 0);
  Point_3 q(c_time+1, c_location, max_primitive_number);

  if(KDT_DEBUG) cout << "doing new event attribute query (" << p.x() << "," << p.y() << ") (" << q.x() << "," << q.y() << ")" << endl;
  uint64_t two = 1;
  vector<Point_and_string> leftResult;
  vector<Point_and_string>::iterator it, left_it, right_it;

  while(1) {
    Point_3 npd(p.x()-two, p.y(), 0);
    Point_3 nqd(q.x(), q.y(), max_primitive_number);
    PS_Fuzzy_iso_box exact_range(npd,nqd);
    tree.search( back_inserter( leftResult ), exact_range);
    if(KDT_DEBUG) cout << "kd tree points are with size: " << leftResult.size() <<  " " << two << endl;
    if(KDT_DEBUG) {
      for(it = leftResult.begin(); it != leftResult.end(); it++) {
        cout << boost::get<0>(*it).x() << " " << boost::get<0>(*it).y() << endl;
      }
    }
    if(leftResult.size()>0 || two > (1<<30)) break;
    two <<= 1;
  }
  if(leftResult.size() > 0) left_it = leftResult.begin();
  if(leftResult.size() > 1) {
    it = leftResult.begin();
    int64_t diff = abs(boost::get<0>(*it).x() - p.x());
    for(; it != leftResult.end(); it++) {
        if(abs(boost::get<0>(*it).x()  - p.x()) < diff) {
            diff = abs(boost::get<0>(*it).x()  - p.x());
            left_it = it;
        }
    }
  }

  if(leftResult.size() > 0) {
    if(KDT_DEBUG) cout << "nearest neighbor in left " << boost::get<1>(*left_it) << endl;

    vector<Point_and_string> rightResult;
    while(1) {
      Point_3 npd(p.x(), p.y(), 0);
      Point_3 nqd(q.x()+two, q.y(), max_primitive_number);
      PS_Fuzzy_iso_box exact_range(npd,nqd);
      tree.search( back_inserter( rightResult ), exact_range);
      if(KDT_DEBUG) cout << "kd tree points are with size: " << rightResult.size() <<  " " << two << endl;
      if(KDT_DEBUG) {
        for(it = rightResult.begin(); it != rightResult.end(); it++) {
          cout << boost::get<0>(*it).x() << " " << boost::get<0>(*it).y() << endl;
        }
      }
      if(rightResult.size()>0 || two > (1<<30)) break;
      two <<= 1;
    }
    if(rightResult.size() > 0) {
      right_it = rightResult.begin();
      if(rightResult.size() > 1) {
        it = rightResult.begin();
        int64_t diff = abs(boost::get<0>(*it).x() - p.x());
        for(; it != rightResult.end(); it++) {
            if(abs(boost::get<0>(*it).x() - p.x()) < diff) {
                diff = abs(boost::get<0>(*it).x() - p.x());
                right_it = it;
            }
        }
      }
      if(KDT_DEBUG) cout << "nearest neighbor in right " << boost::get<1>(*right_it) << endl;
      if(boost::get<1>(*left_it) == boost::get<1>(*right_it)) {
        ret_result = boost::get<1>(*left_it);
        if(KDT_DEBUG) cout << "actual nearest neighbor " << ret_result << endl;
      }
      rightResult.clear();
    }
    leftResult.clear();
  }
  
  return ret_result;
}

void BinnedKDT::getNeighborQuery(int64_t parent_id) {
  // cout << "hello from get neighbor KDT of parent: " << parent_id << endl;

  vector<Point_and_string> result;
  Point_3 p(min_time, min_location, parent_id);
  Point_3 q(max_time, max_location, parent_id);

  PS_Fuzzy_iso_box exact_range(p,q);

  std::chrono::steady_clock::time_point clock_begin = std::chrono::steady_clock::now();
  tree.search( back_inserter( result ), exact_range);
  std::chrono::steady_clock::time_point clock_end = std::chrono::steady_clock::now();
  vector<Point_and_string>::iterator it;
  // cout << "Children: ";
  // for(it = result.begin(); it != result.end(); it++) {
  //   if(boost::get<3>(*it)) continue;
  //   string intervalId = boost::get<1>(*it);
  //   cout << ", " << intervalId;
  // }
  // cout << endl;
  cout << "0,KDT," << "ds_neighbor," << parent_id << "," << result.size()/2 << "," << std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_begin).count() <<
    endl;
}

void BinnedKDT::outputToDot(string fileName) {
  ofstream dotFile(fileName.c_str());
  if (!dotFile.is_open()) {
    cout << "Output stream is not open" << endl;
    return;
  }
  tree.write_graphviz(dotFile);
  // cout << "Exporting to dot file: " << fileName << endl;

  // dotFile << "digraph BinnedKDT {\n";
  // dotFile << "node [shape=record];\n";

  // int nodeId = 0;
  
  // tree.traverse([&](const auto& node) {
  //     int currentId = nodeId++;
  //     if (node.is_leaf()) {
  //         dotFile << "node" << currentId << " [label=\"Leaf\\n";
  //         for (const auto& point : node.points()) {
  //             dotFile << "(" << point.x() << ", " << point.y() << ", " << point.z() << ")\\n";
  //         }
  //         dotFile << "\"];\n";
  //     } else {
  //         dotFile << "node" << currentId << " [label=\"Internal\\nBounding Box\\n[("
  //                 << node.bounding_box().min(0) << ", " << node.bounding_box().min(1) << ", " << node.bounding_box().min(2)
  //                 << ") -> ("
  //                 << node.bounding_box().max(0) << ", " << node.bounding_box().max(1) << ", " << node.bounding_box().max(2)
  //                 << ")\"];\n";

  //         for (const auto& child : node.children()) {
  //             int childId = nodeId++;
  //             dotFile << "node" << currentId << " -> node" << childId << ";\n";
  //         }
  //     }
  // });

  // dotFile << "}\n";
  dotFile.close();
  cout << "DOT file exported as " << fileName << endl;









}