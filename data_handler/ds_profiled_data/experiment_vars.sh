#!/bin/bash

KDT="kd_tree"
SGT="segment_tree"
SAT="summed_area_table"
AGC="agglomerative_clustering"
ESEMAN="eseman_kdt"
ESEMAN_TD="eseman_kdt_twod"
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
LULESH_ID_N="a01b2607-32a6-4435-a2ae-20a4d227e5fd"
KMEANS_LARGE_ID_N="908fc737-2cc7-41d8-8281-7dd9e83155ff"
FIB23_ID_N="ae63b22a-66a3-4e92-ae49-b8206d8a0e7b"
LRA_ID_N="f4e2fdfa-893e-4f13-bac8-e9fbbdf40c1f"
SMTIMER_ID_N="671149b3-d888-4685-af1d-b4fed4337a8f"

# export ESEMAN_SPLITTING_RULE="MAX-DISTANCE"
# export ESEMAN_SPLITTING_RULE="MIDPOINT"
export ESEMAN_SPLITTING_RULE="FAIR"

declare -A primitives
primitives[$DGEM_ID_N]="halide_hpx_for"
primitives[$KMEANS_ID_N]="/phylanx\$0/__add\$0/1\$45\$8"
primitives[$LULESH_ID_N]="run_on_completed_on_new_thread"
primitives[$KMEANS_LARGE_ID_N]="/phylanx\$0/__add\$0/1\$45\$8"
primitives[$FIB23_ID_N]="run_on_completed_on_new_thread"
primitives[$LRA_ID_N]="run_on_completed_on_new_thread"
primitives[$SMTIMER_ID_N]="run_on_completed_on_new_thread"

declare -A dataset_names
dataset_names[$DGEM_ID_N]="dgemm"
dataset_names[$KMEANS_ID_N]="kmeans"
dataset_names[$LULESH_ID_N]="lulesh"
dataset_names[$KMEANS_LARGE_ID_N]="kmeans_large"
dataset_names[$FIB23_ID_N]="fib23"
dataset_names[$LRA_ID_N]="lra"
dataset_names[$SMTIMER_ID_N]="6m_timers"
