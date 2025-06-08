#!/bin/bash

KDT="kd_tree"
SGT="segment_tree"
SAT="summed_area_table"
AGC="agglomerative_clustering"
ESEMAN="eseman_kdt"
DUCK_MIN_MAX="db_duck_min_max"
DUCK_SKETCH="db_duck_sketch"
DUCK_RAW="db_duck_raw"
POSTGRES_MIN_MAX="db_postgres_min_max"
POSTGRES_SKETCH="db_postgres_sketch"
POSTGRES_RAW="db_postgres_raw"

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
DGEM_ID_N="ecc21d0a-112a-4b52-8cdd-6aca80adde93"
KMEANS_ID_N="faf17535-2f66-4621-995f-49c7dbd84e8b"
LULESH_ID_N="0deeca3b-8910-47ca-a3a1-f7bfefe64494"
KMEANS_LARGE_ID_N="908fc737-2cc7-41d8-8281-7dd9e83155ff"

# export ESEMAN_SPLITTING_RULE="MAX-DISTANCE"
# export ESEMAN_SPLITTING_RULE="MIDPOINT"
export ESEMAN_SPLITTING_RULE="FAIR"

declare -A primitives
primitives[$DGEM_ID_N]="halide_hpx_for"
primitives[$KMEANS_ID_N]="/phylanx\$0/__add\$0/1\$45\$8"
primitives[$KMEANS_LARGE_ID_N]="/phylanx\$0/__add\$0/1\$45\$8"

declare -A dataset_names
dataset_names[$KMEANS_ID_N]="kmeans"
dataset_names[$DGEM_ID_N]="dgemm"
dataset_names[$LULESH_ID]="lulesh"
dataset_names[$KMEANS_LARGE_ID_N]="kmeans_large"
