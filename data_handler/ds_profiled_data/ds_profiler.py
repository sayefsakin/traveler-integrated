import csv
import os
from .duck_wrapper import DuckWrapper
from .postgres_wrapper import PostgresWrapper


class DSProfiler:
    def __init__(self):
        self.KDT = "kd_tree"
        self.SGT = "segment_tree"
        self.SAT = "summed_area_table"
        self.DUCK = "duck_db"
        self.POSTGRES = "postgres"
        self.profiled_ds = os.getenv('PROFILED_DS', self.SAT)
        self.db_wrapper = None
        if self.profiled_ds == self.DUCK:
            self.db_wrapper = DuckWrapper()
        elif self.profiled_ds == self.POSTGRES:
            self.db_wrapper = PostgresWrapper()
