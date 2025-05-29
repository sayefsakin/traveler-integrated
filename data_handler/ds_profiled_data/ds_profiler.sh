#!/bin/bash
# use shell script to write to file, to avoid csv file read write time overhead

traveler_base_directory="/mnt/c/Users/sayef/IdeaProjects/traveler-integrated"
profile_directory=$traveler_base_directory"/data_handler/ds_profiled_data"
python_env_directory=$traveler_base_directory"/traveler39"
export DATASET_LOCATION="/mnt/d/Projects/mosaic_testing/mosaic/data/traveler_data"

KDT="kd_tree"
SGT="segment_tree"
SAT="summed_area_table"
AGC="agglomerative_clustering"
ESEMAN="eseman_kdt"
DUCK_MIN_MAX="db_duck_min_max"
DUCK_SKETCH="db_duck_sketch"
POSTGRES_MIN_MAX="db_postgres_min_max"
POSTGRES_SKETCH="db_postgres_sketch"

Q_WINDOW='window'
Q_ATTRIBUTE='attribute'
Q_CHILDREN='children'
Q_CONDW='cond'

#for window and cond query
DGEM_ID="589ca754-ef75-426c-8d51-841cc61dc84a"
KMEANS_ID="8b3289c9-a740-4091-a56d-e4d55af526b5"
LULESH_ID="772c7330-d4eb-485b-866a-3b315063f9af"
KMEANS_LARGE_ID="c3d5e8fe-32df-4f4f-8cbb-4ba6fabd7d3d"

#for attribute and child query
DGEM_ID_N="a9bd20ca-c4f2-4b54-8c49-b968ae7e78be"
KMEANS_ID_N="faf17535-2f66-4621-995f-49c7dbd84e8b"
LULESH_ID_N="0deeca3b-8910-47ca-a3a1-f7bfefe64494"
KMEANS_LARGE_ID_N="908fc737-2cc7-41d8-8281-7dd9e83155ff"

#DATASET_ID="DATASET_ID="$DGEM_ID
LOCALHOST_URL="http://localhost:8000"
LONEPEAK_URL="http://lonepeak2:8000"

export DATASET_ID=$KMEANS_ID_N
export TOTAL_SAMPLE=10
export PROFILED_DS=$DUCK_SKETCH
export QUERY_TYPE=$Q_WINDOW
export BASE_URL=$LOCALHOST_URL
export HORIZONTAL_RESOLUTION_DIVISOR=1

export TRAVELER_DATA_BACKUP_LOCATION="/mnt/d/traveler_dataset_backups"
export LMDB_DATABASE_TOTAL_SIZE=20971520

serve_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_serve_check"
echo "Writing Traveler serve output to file: "$serve_watch
source $python_env_directory"/bin/activate"
# sudo apt-get install liblmdb-dev
traveler(){
  if [[ $PROFILED_DS == db_postgres* ]] ; then
    echo "starting postgres server";
    sudo service postgresql start;
  fi
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
  export PYTHON_PROCESS_ID=`ps -u $USER | grep python3 | awk '{print $1}'`
  export PRE_MEMORY_CHECK=`pmap $PYTHON_PROCESS_ID | grep total | awk '{print $2}' | awk '{SUM += $1} END {print SUM/1024}'`
}

cgal(){
  # run the cgal server
  if [[ $PROFILED_DS != $KDT ]]; then
    return 0;
  fi
  cgal_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_cgal_check"
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
  export CGAL_PROCESS_ID=`ps -u $USER | grep make | awk '{print $1}'`
  sleep 1
}

profile_window_query(){
  selenium_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_selenium_check"
  echo "Running the profiler. Please make sure the XMing is running"
  cd $profile_directory
  export QUERY_TYPE=$Q_WINDOW
  python3 headless_test.py > $selenium_watch &
}

profile_attribute_query(){
  selenium_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_selenium_check"
  echo "Running the profiler. Please make sure the XMing is running"
  cd $profile_directory
  export QUERY_TYPE=$Q_ATTRIBUTE
  python3 headless_test.py > $selenium_watch
}

profile_cond_query(){
  # selenium_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_selenium_check"
  echo "Running the profiler. Please make sure the XMing is running"
  cd $profile_directory
  # export QUERY_TYPE=$Q_ATTRIBUTE
  python3 headless_test.py &
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
  if [[ $PROFILED_DS != $KDT ]]; then
    return 0;
  fi
  if pgrep -x make >/dev/null; then
    export POST_CGAL_MEMORY_CHECK=`pmap $CGAL_PROCESS_ID | grep total | awk '{print $2}' | awk '{SUM += $1} END {print SUM/1024}'`
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
    export POST_MEMORY_CHECK=`pmap $PYTHON_PROCESS_ID | grep total | awk '{print $2}' | awk '{SUM += $1} END {print SUM/1024}'`
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
  export IS_KILLED=1
}

prepare_and_merge_files(){
  cd $profile_directory;
#  rm -rf $DATASET_ID;

  # dont merge selenium watch, problem with the get attribute query
  sed -i '/Serving on localhost:8000/d' $serve_watch;
  
  total_line=$(wc -l < $serve_watch);
  last_line=$(expr $total_line - $TOTAL_SAMPLE);
  sed -i "2,${last_line}d" "$serve_watch";

  mkdir -p $DATASET_ID;
  cp $serve_watch $DATASET_ID;

  if [[ $PROFILED_DS ==  $KDT ]]; then
    linenumber=$(grep -n "Server is now listening" "$cgal_watch" | head -n 1 | cut -d: -f1);
    sed -i "1,${linenumber}d" "$cgal_watch";
    if [[ $IS_KILLED == 1 ]]; then
      sed -i '$d' $cgal_watch;
    fi
    total_line=$(wc -l < $cgal_watch);
    last_line=$(expr $total_line - $TOTAL_SAMPLE);
    sed -i "2,${last_line}d" "$cgal_watch";
    cp $cgal_watch $DATASET_ID;
  fi

  if [[ $QUERY_TYPE ==  $Q_WINDOW ]]; then
    # sed -i '$d' $selenium_watch;
    cp $selenium_watch $DATASET_ID;
  fi

  cd $DATASET_ID;
#  rm -f *_selenium_check;
  paste -d , "$PROFILED_DS"_"$QUERY_TYPE"_* > "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;
  echo "Initial memory: $PRE_MEMORY_CHECK MB" >> "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;
  echo "Post run memory: $POST_MEMORY_CHECK MB" >> "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;

  if [[ $PROFILED_DS == db_postgres* ]] ; then
    echo "Post postgres memory: $POST_CGAL_MEMORY_CHECK MB" >> "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;
  fi
  echo "Formatting all output files";
}


traveler
cgal
prompt_help

export IS_KILLED=0
while IFS= read -r line; do
  if [[ $line ==  "exit" ]]; then
    killing_all_processes;
    break;
  elif [[ $line ==  "traveler" ]]; then
    killing_traveler;
    traveler;
    export IS_KILLED=0;
  elif [[ $line ==  "restart" ]]; then
    killing_all_processes;
    traveler;
    cgal;
    export IS_KILLED=0;
  elif [[ $line ==  "window" ]]; then
    profile_window_query;
  elif [[ $line ==  "attribute" ]]; then
    profile_attribute_query;
  elif [[ $line ==  "cond" ]]; then
    profile_cond_query;
  elif [[ $line ==  "kill" ]]; then
    killing_all_processes;
  elif [[ $line ==  "prepare" ]]; then
    killing_all_processes;
    prepare_and_merge_files;
  else
    $line
  fi
  prompt_help
done

deactivate
echo "DS Profiler exited successfully"

