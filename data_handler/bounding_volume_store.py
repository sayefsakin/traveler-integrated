import data_queries
import math


class BoundingVolumeStore(data_queries.DataQueriesInterface):
    pass


class BVHNode:
    # Node assignment precedence -> up_left, up_right, down_left, down_right
    def __init__(self, start_loc, end_loc, st_time, en_time, ul=None, ur=None, dl=None, dr=None):
        self.location = (start_loc, end_loc)  # this is location index
        self.timestamp = (st_time, en_time)
        self.up_left = ul
        self.up_right = ur
        self.down_left = dl
        self.down_right = dr

    def getTimeWindow(self):
        return self.timestamp[1] - self.timestamp[0]

    def isLeaf(self):
        return self.up_left is None and self.up_right is None and self.down_left is None and self.down_right is None

    def isOverlap(self, sl, el, st, et):
        return sl <= self.location[1] and el >= self.location[0] and st <= self.timestamp[1] and et >= self.timestamp[0]


class BVHStore:
    def __init__(self):
        self.parsed_data = DataParser()
        self.parsed_data.parseTraceData('data/converted')
        self.qd_tree = self.buildQDTree()
        # self.parsed_data.combineIntervals()
        # s = 67063027

    # ignore the cases where en_index < st_index
    def findIntervalsInLocation(self, location, st_time, en_time):
        st_index = self.parsed_data.sortedEventsByLocation[location].bisect((st_time,))
        en_index = self.parsed_data.sortedEventsByLocation[location].bisect((en_time,))
        if st_index == len(self.parsed_data.sortedEventsByLocation[location]):
            st_index -= 1  # st_index = None
        elif self.parsed_data.sortedEventsByLocation[location][st_index][1]['Event'] == 'LEAVE':
            st_index -= 1
        if en_index == len(self.parsed_data.sortedEventsByLocation[location]):
            en_index -= 1  # en_index = None
        elif self.parsed_data.sortedEventsByLocation[location][en_index][1]['Event'] == 'ENTER':  # remove the trailing enter event
            en_index -= 1
        return (st_index, en_index)

    def buildQDTree(self):
        return self.insertIntoQDTree(0,
                                     len(self.parsed_data.info['locationNames'])-1,
                                     self.parsed_data.info['domain'][0],
                                     self.parsed_data.info['domain'][1], 0)

    def insertIntoQDTree(self, start_loc_index, end_loc_index, st_time, en_time):
        if start_loc_index > end_loc_index:
            print('==== start loc is greater than en loc')
            return None
        start_loc = self.parsed_data.info['locationNames'][start_loc_index]
        end_loc = self.parsed_data.info['locationNames'][end_loc_index]
        if start_loc == end_loc:
            st, en = self.findIntervalsInLocation(start_loc, st_time, en_time)
            if st is None or en is None:
                if st is not None:
                    st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                    return BVHNode(start_loc_index, end_loc_index, st_time, en_time)
                if en is not None:
                    en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)
                    return BVHNode(start_loc_index, end_loc_index, st_time, en_time)
                print('==== both st en is None')
                return None
            if st + 1 == en:  # this belongs to a single interval
                if self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Event'] == 'ENTER' \
                        and self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Event'] == 'ENTER':
                    last_time = en_time
                    st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                    en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)
                    left = BVHNode(start_loc_index, end_loc_index, st_time, en_time - 1)
                    right = BVHNode(start_loc_index, end_loc_index, en_time, last_time)
                    return BVHNode(start_loc_index, end_loc_index, st_time, last_time, left, right)
                elif self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Event'] == 'LEAVE' \
                        and self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Event'] == 'LEAVE':
                    f_time = st_time
                    st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                    en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)
                    left = BVHNode(start_loc_index, end_loc_index, f_time, st_time)
                    right = BVHNode(start_loc_index, end_loc_index, st_time + 1, en_time)
                    return BVHNode(start_loc_index, end_loc_index, f_time, en_time, left, right)
                elif self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Event'] == 'LEAVE' \
                        and self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Event'] == 'ENTER':
                    last_time = en_time
                    f_time = st_time
                    st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                    en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)
                    left = BVHNode(start_loc_index, end_loc_index, f_time, st_time)
                    right = BVHNode(start_loc_index, end_loc_index, en_time, last_time)
                    return BVHNode(start_loc_index, end_loc_index, f_time, last_time, left, right)
                else:
                    st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                    en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)
                    return BVHNode(start_loc_index, end_loc_index, st_time, en_time)
            elif en == st:
                if self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Event'] == 'LEAVE':
                    en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)
                    return BVHNode(start_loc_index, end_loc_index, st_time, en_time)
                else:
                    st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                    return BVHNode(start_loc_index, end_loc_index, st_time, en_time)
            elif en < st:
                st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                return BVHNode(start_loc_index, end_loc_index, st_time, en_time)
            else:
                st_time = max(self.parsed_data.sortedEventsByLocation[start_loc][st][1]['Timestamp'], st_time)
                en_time = min(self.parsed_data.sortedEventsByLocation[start_loc][en][1]['Timestamp'], en_time)

        mid_h = math.floor((st_time + en_time) / 2)
        up_left = self.insertIntoQDTree(start_loc_index, end_loc_index, st_time, mid_h)
        up_right = self.insertIntoQDTree(start_loc_index, end_loc_index, mid_h+1, en_time)

        mid_v = math.floor((start_loc_index + end_loc_index) / 2)
        down_left = self.insertIntoQDTree(start_loc_index, mid_v, st_time, en_time)
        if mid_v+1 <= end_loc_index:
            down_right = self.insertIntoQDTree(mid_v+1, end_loc_index, st_time, en_time)
        else:
            down_right = None
        return BVHNode(start_loc_index, end_loc_index, st_time, en_time, up_left, up_right, down_left, down_right)

    def queryInRange(self, start_loc_index, end_loc_index, st_time, en_time, figure_width):
        data = {}
        for lc in range(start_loc_index, end_loc_index + 1):
            data[lc] = list()
        pixel_window = (en_time - st_time) / figure_width

        def searchInQDTree(qdt_node, sl_index, el_index, st, et):
            if qdt_node.getTimeWindow() <= pixel_window:
                if qdt_node.isLeaf() is False:
                    for loc in range(qdt_node.location[0], qdt_node.location[1] + 1):
                        data[loc].append((qdt_node.timestamp[0], qdt_node.timestamp[1], (120, 107, 255, 255)))
                return
            if qdt_node.isLeaf():
                for loc in range(qdt_node.location[0], qdt_node.location[1] + 1):
                    data[loc].append((qdt_node.timestamp[0], qdt_node.timestamp[1], (33, 12, 250, 255)))
                return
            if qdt_node.up_left and qdt_node.up_left.isOverlap(sl_index, el_index, st, et):
                searchInQDTree(qdt_node.up_left, sl_index, el_index, st, et)
            if qdt_node.up_right and qdt_node.up_right.isOverlap(sl_index, el_index, st, et):
                searchInQDTree(qdt_node.up_right, sl_index, el_index, st, et)
            if qdt_node.down_left and qdt_node.down_left.isOverlap(sl_index, el_index, st, et):
                searchInQDTree(qdt_node.down_left, sl_index, el_index, st, et)
            if qdt_node.down_right and qdt_node.down_right.isOverlap(sl_index, el_index, st, et):
                searchInQDTree(qdt_node.down_right, sl_index, el_index, st, et)

        searchInQDTree(self.qd_tree, start_loc_index, end_loc_index, st_time, en_time)
        return data