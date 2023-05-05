#!/bin/bash
# use shell script to write to file, to avoid csv file read write time overhead

traveler_base_directory="/mnt/c/Users/sayef/IdeaProjects/traveler-integrated"
profile_directory=$traveler_base_directory"/data_handler/ds_profiled_data"
KDT="kd_tree"
SGT="segment_tree"
SAT="summed_area_table"

DGEM_ID="589ca754-ef75-426c-8d51-841cc61dc84a"
KMEANS_ID="8b3289c9-a740-4091-a56d-e4d55af526b5"
LULESH_ID="772c7330-d4eb-485b-866a-3b315063f9af"
#DATASET_ID="DATASET_ID="$DGEM_ID

export DATASET_ID=$DGEM_ID
export TOTAL_SAMPLE=10
export PROFILED_DS=$KDT

serve_watch=$profile_directory"/"$PROFILED_DS"_serve_check"
echo "Writing Traveler serve output to file: "$serve_watch

traveler(){
  # run traveler first
  cd /mnt/c/Users/sayef/IdeaProjects/traveler-integrated
  python3 serve.py > $serve_watch &

  while true; do
    if curl -I "http://localhost:8000/static/interface.html" 2>&1 | grep -w "200\|301" ; then
        echo "Traveler is up and running";
        break;
    else
        sleep 1
    fi
  done
  sleep 1
}

cgal(){
  # run the cgal server
  cgal_watch=$profile_directory"/"$PROFILED_DS"_cgal_check"
  echo "Writing CGAL server output to file: "$cgal_watch
  cd $traveler_base_directory"/data_handler/cgal_libs/cgal_server"
  rm -f "cgal_server_check"
  make > $cgal_watch &
  while true; do
    if [ -f "cgal_server_check" ]; then
        break;
    else
        sleep 1
    fi
  done
  echo "CGAL server is running";
  sleep 1
}

profile_window_query(){
  selenium_watch=$profile_directory"/"$PROFILED_DS"_selenium_check"
  echo "Running the profiler. Please make sure the XMing is running"
  cd $profile_directory
  python3 headless_test.py > $selenium_watch
}

prompt_help(){
  echo ""
  echo "==========================================="
  echo "Input Commands: exit, kill, prepare, window, restart, traveler"
  echo "exit: exit and terminate the DS profiler"
  echo "kill: kill all processes run by the DS profiler"
  echo "prepare: prepare all output file to CSV format"
  echo "window: profile the window query using selenium"
  echo "restart: re run Traveler and then the cgal data server"
  echo "traveler: restart only Traveler"
  echo "==========================================="
  echo -n ">_"
}

killing_cgal(){
  if pgrep -x make >/dev/null; then
    echo "killing cgal data server";
    killall make 2>/dev/null;
  fi
  while true; do
    if pgrep -x make >/dev/null; then
#      echo "make is still running";
      sleep 1;
    else
      break
    fi
  done
  echo "killed cgal data server"
}

killing_traveler(){
  if pgrep -x python3 >/dev/null; then
    echo "killing Traveler";
    killall python3 2>/dev/null;
  fi
  while true; do
    if pgrep -x python3 >/dev/null; then
#      echo "Traveler is still running";
      sleep 2;
    else
      break
    fi
  done
  echo "killed Traveler"
}

killing_all_processes(){
  killing_cgal
  killing_traveler
  echo "killed all processes"
}

prepare_and_merge_files(){
  sed -i '/Serving on localhost:8000/d' $serve_watch;
  linenumber=$(grep -n "Server is now listening" "$cgal_watch" | head -n 1 | cut -d: -f1);
  sed -i "1,${linenumber}d" "$cgal_watch";
  if [[ $IS_KILLED == 1 ]]; then
    sed -i '$d' $cgal_watch;
  fi
  total_line=$(wc -l < $serve_watch);
  last_line=$(expr $total_line - $TOTAL_SAMPLE);
  sed -i "2,${last_line}d" "$serve_watch";

  total_line=$(wc -l < $cgal_watch);
  last_line=$(expr $total_line - $TOTAL_SAMPLE);
  sed -i "2,${last_line}d" "$cgal_watch";

  sed -i '$d' $selenium_watch;

  cd $profile_directory;
#  rm -rf $DATASET_ID;
  mkdir -p $DATASET_ID;
  mv $serve_watch $DATASET_ID;
  mv $cgal_watch $DATASET_ID;
  mv $selenium_watch $DATASET_ID;

  cd $DATASET_ID;
  paste -d , "$PROFILED_DS"_* > "$PROFILED_DS"_merged.csv;

  echo "Formatting all output files";
}


traveler
cgal
prompt_help

IS_KILLED=0
while IFS= read -r line; do
  if [[ $line ==  "exit" ]]; then
    killing_all_processes;
    break;
  elif [[ $line ==  "traveler" ]]; then
    killing_traveler;
    traveler;
    IS_KILLED=0;
  elif [[ $line ==  "restart" ]]; then
    killing_all_processes;
    traveler;
    cgal;
    IS_KILLED=0;
  elif [[ $line ==  "window" ]]; then
    profile_window_query;
  elif [[ $line ==  "kill" ]]; then
    killing_all_processes;
    IS_KILLED=1;
  elif [[ $line ==  "prepare" ]]; then
    killing_all_processes;
    prepare_and_merge_files;
  else
    $line
  fi
  prompt_help
done

echo "DS Profiler exited successfully"

