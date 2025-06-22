#!/bin/bash
# use shell script to write to file, to avoid csv file read write time overhead

source experiment_vars.sh

if [ -z "$1" ]; then
  export QUERY_TYPE=$Q_WINDOW
elif [[ $1 == "window" || $1 == "attribute" || $1 == "cond" ]]; then
  export QUERY_TYPE=$1
else
  export QUERY_TYPE=$Q_WINDOW
fi

if [ -z "$2" ]; then
  export DATASET_ID=$DGEM_ID_N
else
  export DATASET_ID=$2
fi

if [ -z "$3" ]; then
  export PROFILED_DS=$ESEMAN
else
  export PROFILED_DS=$3
fi

#change these for each experiment
export TOTAL_SAMPLE=5
export HORIZONTAL_RESOLUTION_DIVISOR=1

echo "Running experiment on dataset: $DATASET_ID"
echo "Running experiment on algorithm: $PROFILED_DS"
echo "Running experiment on query: $QUERY_TYPE"

if [ -z "$4" ]; then
  export SELECTED_PRIMITIVE=''
else
  export SELECTED_PRIMITIVE=$4
  echo "Running experiment on selected primitive: $SELECTED_PRIMITIVE"
fi
echo "Running experiment on hrd: $HORIZONTAL_RESOLUTION_DIVISOR"
echo "Running experiment with iterations: $TOTAL_SAMPLE"
echo "======================================"


traveler_base_directory="/mnt/c/Users/sayef/IdeaProjects/traveler-integrated"
profile_directory="/mnt/d/ldav25_profiled_data"
python_env_directory=$traveler_base_directory"/traveler39"
export DATASET_LOCATION="/mnt/d/all_traveler_otf2_files/all_data/json_data"
traveler_discache_location="/mnt/d/all_traveler_otf2_files/all_data/"${dataset_names[$DATASET_ID]}

LOCALHOST_URL="http://localhost:8000"
# LONEPEAK_URL="http://lonepeak2:8000"
LONEPEAK_URL="http://"$(hostname)":8000"
export BASE_URL=$LONEPEAK_URL

# this is where eseman stores lmdb files
export LMDB_DATA_BACKUP_LOCATION="/mnt/d/traveler_dataset_backups"
export LMDB_DATABASE_TOTAL_SIZE=$((2*1000*1000*1000))
# 20971520, 50GB

serve_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_serve_check"
echo "Writing Traveler serve output to file: "$serve_watch
source $python_env_directory"/bin/activate"

# sudo apt-get install liblmdb-dev
traveler(){
  if [[ $PROFILED_DS == db_postgres* ]] ; then
    echo "starting postgres server";
    export PGDATA=$HOME/postgres_data;
    pg_ctl start -l ~/postgres_logs;
    # service postgresql start;
  fi
  # run traveler first
  cd $traveler_base_directory
  python3 serve.py --db_dir $traveler_discache_location > $serve_watch &

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
  if [[ $PROFILED_DS != $KDT && $PROFILED_DS != $AGC && $PROFILED_DS != $ESEMAN && $PROFILED_DS != $ESEMAN_TD ]]; then
    return 0;
  fi
  cgal_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_cgal_check"
  echo "Writing CGAL server output to file: "$cgal_watch
  cd $traveler_base_directory"/data_handler/cgal_libs/cgal_server"
  rm -f "cgal_server_check"
  ./cgal_data_server $DATASET_ID false > $cgal_watch &
  while true; do
    if [ -f "cgal_server_check" ]; then
        break;
    else
        sleep 1
    fi
  done
  echo "CGAL server is running";
  export CGAL_PROCESS_ID=`ps -u $USER | grep cgal_data_serve | awk '{print $1}'`
  sleep 1
}

build_lmdb(){
  # run the cgal server
  if [[ $PROFILED_DS != $ESEMAN && $PROFILED_DS != $ESEMAN_TD ]]; then
    return 0;
  fi
  cgal_watch=$profile_directory"/"$PROFILED_DS"_"$DATASET_ID"_cgal_check_build_"$ESEMAN_TASK_ID
  echo "Writing CGAL server output to file: "$cgal_watch
  cd $traveler_base_directory"/data_handler/cgal_libs/cgal_server"
  ./cgal_data_server $DATASET_ID true > $cgal_watch
  export CGAL_PROCESS_ID=`ps -u $USER | grep cgal_data_serve | awk '{print $1}'`
  sleep 1
}

profile_window_query(){
  cd $profile_directory;

  HRDS='';
  if [[ $HORIZONTAL_RESOLUTION_DIVISOR -ne 1 ]]; then
    HRDS=$HORIZONTAL_RESOLUTION_DIVISOR"_";
  fi
  NDDIR=$HRDS$DATASET_ID;
  mkdir -p $NDDIR;
  cd $NDDIR;
  mkdir -p "figures";

  selenium_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_selenium_check"
  echo "Running the window profiler."
  cd $traveler_base_directory"/data_handler/ds_profiled_data"
  export QUERY_TYPE=$Q_WINDOW
  python3 headless_test.py $profile_directory | tee $selenium_watch
}

profile_attribute_query(){
  #comment out headlesss to output png with clicking
  selenium_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_selenium_check"
  echo "Running the attribute profiler."
  cd $traveler_base_directory"/data_handler/ds_profiled_data"
  export QUERY_TYPE=$Q_ATTRIBUTE
  python3 headless_test.py $profile_directory | tee $selenium_watch
}

profile_cond_query(){
  cd $profile_directory;
  
  HRDS='';
  if [[ $HORIZONTAL_RESOLUTION_DIVISOR -ne 1 ]]; then
    HRDS=$HORIZONTAL_RESOLUTION_DIVISOR"_";
  fi
  NDDIR=$HRDS$DATASET_ID;
  mkdir -p $NDDIR;
  cd $NDDIR;
  mkdir -p "figures";

  selenium_watch=$profile_directory"/"$PROFILED_DS"_"$QUERY_TYPE"_selenium_check"
  echo "Running the cond profiler."
  cd $traveler_base_directory"/data_handler/ds_profiled_data"
  export QUERY_TYPE=$Q_CONDW
  python3 headless_test.py $profile_directory | tee $selenium_watch
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
  if [[ $PROFILED_DS != $KDT && $PROFILED_DS != $AGC && $PROFILED_DS != $ESEMAN && $PROFILED_DS != $ESEMAN_TD ]]; then
    return 0;
  fi
  if pgrep -x cgal_data_serve >/dev/null; then
    export POST_CGAL_MEMORY_CHECK=`pmap $CGAL_PROCESS_ID | grep total | awk '{print $2}' | awk '{SUM += $1} END {print SUM/1024}'`
    echo "killing cgal data server";
    cd $traveler_base_directory;
    python3 stopCgalServer.py;
  fi
  while true; do
    if pgrep -x cgal_data_serve >/dev/null; then
      sleep 1;
    else
      break
    fi
  done
  echo "killed cgal data server"
}

killing_traveler(){
  if [ -n "$PYTHON_PROCESS_ID" ] && ps -p $PYTHON_PROCESS_ID > /dev/null; then
    export POST_MEMORY_CHECK=`pmap $PYTHON_PROCESS_ID | grep total | awk '{print $2}' | awk '{SUM += $1} END {print SUM/1024}'`
    echo "killing Traveler";
    kill $PYTHON_PROCESS_ID;
  fi
  while true; do
    if [ -n "$PYTHON_PROCESS_ID" ] && ps -p $PYTHON_PROCESS_ID > /dev/null; then
      # echo "Traveler is still running";
      sleep 2;
    else
      break
    fi
  done
  echo "killed Traveler"
  if [[ $PROFILED_DS == db_postgres* ]] ; then
    pg_ctl stop -l ~/postgres_logs;
    echo "stopping postgres server";
  fi
}

killing_all_processes(){
  killing_cgal
  killing_traveler
  echo "killed all processes"
  export IS_KILLED=1
}

prepare_and_merge_files(){
  cd $profile_directory;
  
  HRDS='';
  if [[ $HORIZONTAL_RESOLUTION_DIVISOR -ne 1 ]]; then
    HRDS=$HORIZONTAL_RESOLUTION_DIVISOR"_";
  fi
  NDDIR=$HRDS$DATASET_ID;

  sed -i '/Serving on localhost:8000/d' $serve_watch;

  mkdir -p $NDDIR;
  cd $NDDIR;
  rm -f "$PROFILED_DS"_"$QUERY_TYPE"_*;
  cd ..;

  cp $serve_watch $NDDIR;

  if [[ $PROFILED_DS == $KDT || $PROFILED_DS == $AGC || $PROFILED_DS == $ESEMAN || $PROFILED_DS == $ESEMAN_TD ]]; then
    linenumber=$(grep -n "Server is now listening" "$cgal_watch" | head -n 1 | cut -d: -f1);
    sed -i "1,${linenumber}d" "$cgal_watch";
    sed -i '$d' $cgal_watch;
    cp $cgal_watch $NDDIR;
  fi

  cd $NDDIR;
#  rm -f *_selenium_check;
  paste -d , "$PROFILED_DS"_"$QUERY_TYPE"_* > "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;
  cp $selenium_watch ./;

  echo "Initial serve memory: $PRE_MEMORY_CHECK MB" >> "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;
  echo "Post serve memory: $POST_MEMORY_CHECK MB" >> "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;
  echo "Post data serve memory: $POST_CGAL_MEMORY_CHECK MB" >> "$PROFILED_DS"_"$QUERY_TYPE"_merged.csv;

  echo "Formatting all output files";
}

traveler
if [[ $1 == "build_lmdb" ]]; then
  build_lmdb;
  killing_traveler;
else
  cgal
fi

do_interactive_prompts(){
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
}

do_make_json(){
  curl -H "Accept: application/json" -X GET http://localhost:8000/datasets/$DATASET_ID/intervals -o ~/$DATASET_ID.json

  jq . ~/$DATASET_ID.json 1> /dev/null
  if [ $? -eq 0 ]; then
    echo "JSON file format is correct"
    mv ~/$DATASET_ID.json $DATASET_LOCATION/
  else
    echo "Wrong JSON format"
  fi
  killing_all_processes
}

if [[ $1 == "window" ]]; then
  profile_window_query
elif [[ $1 ==  "attribute" ]]; then
  profile_attribute_query
elif [[ $1 ==  "cond" ]]; then
  profile_cond_query
elif [[ $1 ==  "kill" ]]; then
  killing_all_processes
elif [[ $1 ==  "interactive" ]]; then
  do_interactive_prompts
elif [[ $1 ==  "make_json" ]]; then
  do_make_json
fi

if [[ $1 == "window" || $1 == "attribute" || $1 == "cond" ]]; then
  killing_all_processes;
  prepare_and_merge_files;
fi

deactivate
echo "DS Profiler exited successfully"

