import math
import os
import duckdb

class DuckWrapper():
    def __init__(self):
        self.dataset_id = os.getenv('DATASET_ID', "")
        self.dataset_location = os.getenv('DATASET_LOCATION', "/mnt/d/Projects/mosaic_testing/mosaic/data/traveler_data")
        self.connection = duckdb.connect(self.dataset_location + "/" + self.dataset_id + ".db")
        qry = "CREATE TABLE IF NOT EXISTS intervals AS " + \
            "SELECT " \
            " enter.Timestamp AS enter_timestamp, leave.Timestamp AS leave_timestamp, intervalId, parent, children, 'Parent GUID' AS pg, Location, GUID, Primitive" \
            " FROM read_json(\'" + self.dataset_location + "/" + self.dataset_id + \
            ".json\', auto_detect=true, format=\'array\', maximum_depth=-1)"
        self.connection.sql(qry)
        # self.connection.sql("CREATE INDEX et_idx ON intervals (enter_timestamp)")
        # self.connection.sql("CREATE INDEX lt_idx ON intervals (leave_timestamp)")
        # print("duckdb database created: " + qry)
        # self.db_agg_test("202591429", "278066359", "5", "50")

    def closeConnection(self):
        self.connection.close()

    def db_agg_test(self, bins, b, e, l):
        bin_size = str(int((int(e) - int(b)) / int(bins)))
        begin = str(b)
        end = str(e)
        location = str(l)
        only_bin_sql = " SELECT unnest(generate_series(" + begin + ", " + end + ", " + bin_size  + ")) AS bin " \
                " FROM intervals AS t " \
                " WHERE " \
                " t.leave_timestamp >= " + begin + " and t.enter_timestamp <= " + end + " and t.Location = " + location + ""
        tst_sql = "SELECT enter_timestamp as atime, 1 AS ct FROM intervals WHERE " \
                " leave_timestamp >= " + begin + " and enter_timestamp <= " + end + " and Location = " + location + "" \
                " UNION " \
                " SELECT leave_timestamp as atime, 0 AS ct FROM intervals WHERE " \
                " leave_timestamp >= " + begin + " and enter_timestamp <= " + end + " and Location = " + location + "" \
                " ORDER BY atime "
        cplx_sql = "SELECT bin_t.bin AS bn, min(util.atime) AS rn FROM " \
                " ( " + only_bin_sql + " ) AS bin_t INNER JOIN ( " + tst_sql + "" \
                " ) AS util " \
                " ON bin_t.bin <= util.atime " \
                " GROUP BY bin_t.bin"
        # cplx_with_ct_sql = "SELECT cp.bn, tst.atime, tst.ct, " \
        cplx_with_ct_sql = "SELECT " \
                " CASE WHEN tst.ct = 0 AND tst.atime - cp.bn >= " + bin_size  + " THEN 1 " \
                    " WHEN tst.ct = 0 AND tst.atime - cp.bn < " + bin_size  + " THEN 0.5 " \
                    " WHEN tst.atime - cp.bn < " + bin_size  + " THEN 0.5 " \
                    " ELSE 0 END as utl " \
                " FROM " \
                " ( " + cplx_sql + " ) AS cp INNER JOIN ( " + tst_sql + " ) AS tst ON cp.rn = tst.atime " \
                " ORDER BY cp.bn "
        results = self.connection.execute(cplx_with_ct_sql).fetchall()
        values_only = [float(t[0]) for t in results]
        if len(values_only) > 1:
            del values_only[-1]
        # print("duck db values: ", end='')
        # print(values_only)
        # print(len(values_only))
        return values_only
    
    def db_gantt_sketch(self, bins, begin, end, location):
        elements = "1000000"
        gs_query = "SELECT enter_timestamp, leave_timestamp, Location FROM intervals" \
            " WHERE Location = " + str(location) + \
            " USING SAMPLE reservoir(" + elements + " ROWS) REPEATABLE(100)"
        results = self.connection.execute(gs_query).fetchall()


        def getBinSize(time_begin: int, time_end: int, bins: int) -> int:
            return int(math.floor((time_end - time_begin) / bins))
        
        def getBinNumber(time_begin: int, time_end: int, bins: int, ctime: int) -> int:
            bin_size = getBinSize(time_begin, time_end, bins)
            if ctime < time_begin or ctime > time_end:
                return -1
            return int(math.floor((ctime - time_begin) / bin_size))

        locDict = [0.0] * bins
        bin_size = getBinSize(begin, end, bins)
        
        for item in results:
            interval_time_start = int(item[0])
            interval_time_end = int(item[1])
            interval_location = int(item[2])

            startingBin = getBinNumber(begin, end, bins, interval_time_start)
            endingBin = getBinNumber(begin, end, bins, interval_time_end)
            if startingBin < 0 or endingBin < 0:
                continue

            for bin_it in range(startingBin + 1, min(endingBin, bins)):
                if locDict[bin_it] < 0.5:
                    locDict[bin_it] = 1.0

            if startingBin < bins and locDict[startingBin] < 0.5:
                locDict[startingBin] = 0.5 if interval_time_start % bin_size else 1.0

            if endingBin < bins and locDict[endingBin] < 0.5:
                locDict[endingBin] = 0.5 if interval_time_end % bin_size else 1.0

        return locDict


    def db_m4_optimization(self, bins, begin, end, location):
        elements = "1000000"
        gs_query = "SELECT enter_timestamp, leave_timestamp, Location FROM intervals" \
            " WHERE Location = " + str(location) + \
            " USING SAMPLE reservoir(" + elements + " ROWS) REPEATABLE(100)"
        results = self.connection.execute(gs_query).fetchall()


        def getBinSize(time_begin: int, time_end: int, bins: int) -> int:
            return int(math.floor((time_end - time_begin) / bins))
        
        def getBinNumber(time_begin: int, time_end: int, bins: int, ctime: int) -> int:
            bin_size = getBinSize(time_begin, time_end, bins)
            if ctime < time_begin or ctime > time_end:
                return -1
            return int(math.floor((ctime - time_begin) / bin_size))

        locDict = [0.0] * bins
        bin_size = getBinSize(begin, end, bins)
        
        for item in results:
            interval_time_start = int(item[0])
            interval_time_end = int(item[1])
            interval_location = int(item[2])

            startingBin = getBinNumber(begin, end, bins, interval_time_start)
            endingBin = getBinNumber(begin, end, bins, interval_time_end)
            if startingBin < 0 or endingBin < 0:
                continue

            for bin_it in range(startingBin + 1, min(endingBin, bins)):
                if locDict[bin_it] < 0.5:
                    locDict[bin_it] = 1.0

            if startingBin < bins and locDict[startingBin] < 0.5:
                locDict[startingBin] = 0.5 if interval_time_start % bin_size else 1.0

            if endingBin < bins and locDict[endingBin] < 0.5:
                locDict[endingBin] = 0.5 if interval_time_end % bin_size else 1.0

        return locDict
