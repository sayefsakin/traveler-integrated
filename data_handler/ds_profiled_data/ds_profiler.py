import csv

class DSProfiler:
    def __init__(self):
        self.KDT = "kd_tree"
        self.SGT = "segment_tree"
        self.SAT = "summed_area_table"
        self.profiled_ds = self.SGT  # change this parameter to profile different data structure

        # dont use csv writer as it could add overhead
        # self.profile_directory = '/mnt/c/Users/sayef/IdeaProjects/traveler-integrated/data_handler/ds_profiled_data'
        # self.wFile = open(self.profile_directory + '/' + self.profiled_ds + '.csv', 'w', encoding='UTF8')
        # self.writer = csv.writer(self.wFile)

