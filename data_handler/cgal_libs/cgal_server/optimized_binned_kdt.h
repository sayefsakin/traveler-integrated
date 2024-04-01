#ifndef OPTIMIZED_BINNED_KDT_H_
#define OPTIMIZED_BINNED_KDT_H_

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Search_traits_adapter.h>
#include <CGAL/point_generators_3.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/property_map.h>
#include <CGAL/Fuzzy_iso_box.h>
#include <boost/iterator/zip_iterator.hpp>

#include <utility>
#include <string>

using namespace std;

typedef CGAL::Simple_cartesian<double>                      EPICKernel;
// typedef CGAL::Exact_predicates_inexact_constructions_kernel EPICKernel;
typedef EPICKernel::Point_3                                 Point_3; // time, location, primitive
typedef boost::tuple<Point_3,int>                           Point_and_int;
//3d point, interval id, lenght, enter(0) or leave(10)
typedef boost::tuple<Point_3, std::string, int64_t, bool>     Point_and_string;
typedef CGAL::Random_points_in_cube_3<Point_3>              Random_points_iterator;
typedef CGAL::Search_traits_3<EPICKernel>                   Traits_base;
typedef CGAL::Search_traits_adapter<Point_and_string,
  CGAL::Nth_of_tuple_property_map<0, Point_and_string>,
  Traits_base>                                              PSTraits;
typedef CGAL::Orthogonal_k_neighbor_search<PSTraits>        K_neighbor_search;
typedef CGAL::Fuzzy_iso_box<PSTraits>                       PS_Fuzzy_iso_box;

typedef K_neighbor_search::Tree                             KNSKDTree;
typedef K_neighbor_search::Distance                         Distance;
typedef K_neighbor_search::iterator                         NN_iterator;

typedef std::map<uint64_t, std::vector<double>>             LocDict;

#define KDT_DEBUG 0

// A functor that returns true, iff the x-coordinate of a dD point is not positive
struct X_not_positive {
  bool operator()(const NN_iterator& it) { return boost::get<1>((*it).first) != "1";  }
};
// An iterator that only enumerates dD points with positive x-coordinate
typedef CGAL::Filter_iterator<NN_iterator, X_not_positive> NN_positive_x_iterator;

int point_with_info_testing();

inline uint64_t getBinSize(int64_t time_begin, int64_t time_end, uint64_t bins){
  return (uint64_t)floor((double)(time_end - time_begin) / (double)bins);
}

inline int getBinNumber(int64_t time_begin, int64_t time_end, uint64_t bins, int64_t ctime) {
  uint64_t bin_size = getBinSize(time_begin, time_end, bins);
  if(ctime < time_begin || ctime > time_end) return -1;
  return (int)floor((double)(ctime - time_begin) / (double)bin_size);
}

class BinnedKDT {
  int64_t max_interval_length;
  int64_t max_primitive_number;
public:
  int64_t min_time, max_time;
  int64_t min_location, max_location;
  KNSKDTree tree;
  BinnedKDT(){
    max_interval_length=1;
    max_primitive_number=0;
    min_time = 0;
    max_time = 0;
    min_location = 0;
    max_location = 0;
  }
  void insertDataIntoTree(double enter_time, double enter_loc,
                          double end_time, double end_loc,
                          std::string interval_id, int64_t primitive_number);
  LocDict binnedRangeQuery(int64_t time_begin, 
                            int64_t time_end, 
                            uint64_t location_begin, 
                            uint64_t location_end, 
                            uint64_t bins,
                            int64_t primitive);
  string findNearestInterval(int64_t c_time, uint64_t c_location);
  void getNeighborQuery(int64_t parent_id);
};

#endif