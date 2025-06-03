#!/bin/bash

source experiment_vars.sh

alg_list=($SAT $AGC $ESEMAN)
# $DUCK_MIN_MAX $DUCK_SKETCH $DUCK_RAW $POSTGRES_MIN_MAX $POSTGRES_SKETCH $POSTGRES_RAW)
query_list=($Q_WINDOW $Q_ATTRIBUTE $Q_CONDW)
dataset_list=($DGEM_ID_N $KMEANS_ID_N)


declare -A primitives

primitives[$DGEM_ID_N]="halide_hpx_for"
primitives[$KMEANS_ID_N]="/phylanx\$0/__add\$0/1\$45\$8"

for key in "${!primitives[@]}"; do
  echo "Key: $key, Value: ${primitives[$key]}"
done

for dtst in "${dataset_list[@]}"
do
    for alg in "${alg_list[@]}"
    do
        for qry in "${query_list[@]}"
        do
            if [[ $qry == $Q_CONDW ]]; then
                ./ds_profiler.sh $qry $dtst $alg ${primitives[$dtst]}
            else
                ./ds_profiler.sh $qry $dtst $alg
            fi
            # echo "Processing (dataset, algorithm, query): ($dtst, $alg, $qry)"
        done
    done
done





