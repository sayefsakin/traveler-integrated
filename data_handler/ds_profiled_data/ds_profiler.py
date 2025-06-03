import csv
import os
from .duck_wrapper import DuckWrapper
# from .postgres_wrapper import PostgresWrapper


class DSProfiler:
    def __init__(self):
        self.RAW = "raw"
        self.KDT = "kd_tree"
        self.SGT = "segment_tree"
        self.SAT = "summed_area_table"
        self.AGC = "agglomerative_clustering"
        self.EKM = "eseman_kdt"
        self.DUCK = "db_duck"
        self.POSTGRES = "db_postgres"
        self.DBTYPE_MIN_MAX = "min_max"
        self.DBTYPE_SKETCH = "sketch"
        self.DBTYPE_RAW = "raw"
        self.profiled_ds = os.getenv('PROFILED_DS', self.SAT)
        self.db_wrapper = None
        if self.profiled_ds.startswith(self.DUCK):
            self.db_wrapper = DuckWrapper()
        # elif self.profiled_ds.startswith(self.POSTGRES):
        #     self.db_wrapper = PostgresWrapper()
