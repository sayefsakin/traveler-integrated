# Imports
import copy

import numpy as np
import json
from profiling_tools._cCalcBin import ffi, lib


class DataCompStore():
    def __init__(self, hasKDT=True, hasQDT=True, hasBVH=True, hasSGT=True):
        self.kdtEnabled = hasKDT
        self.qdtEnabled = hasQDT
        self.bvhEnabled = hasBVH
        self.sgtEnabled = hasSGT

    def generateAllStructuresForDataStores(self):
        if self.kdtEnabled is True:
            pass
        if self.qdtEnabled is True:
            pass
        if self.bvhEnabled is True:
            pass
        if self.sgtEnabled is True:
            pass
