import csv
import os


class DSProfiler:
    def __init__(self):
        self.KDT = "kd_tree"
        self.SGT = "segment_tree"
        self.SAT = "summed_area_table"
        self.profiled_ds = os.getenv('PROFILED_DS', self.SAT)
