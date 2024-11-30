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
            " CAST(enter.Timestamp AS BIGINT) AS enter_timestamp, CAST(leave.Timestamp AS BIGINT) AS leave_timestamp, intervalId, parent, children, 'Parent GUID' AS pg, Location, GUID, Primitive" \
            " FROM read_json(\'" + self.dataset_location + "/" + self.dataset_id + \
            ".json\', auto_detect=true, format=\'array\', maximum_depth=-1)"
        self.connection.sql(qry)
        print("Fetching data from DuckDB")
        self.connection.sql("CREATE INDEX IF NOT EXISTS et_idx ON intervals (enter_timestamp)")
        self.connection.sql("CREATE INDEX IF NOT EXISTS lt_idx ON intervals (leave_timestamp)")
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
    
    def db_min_max_test(self, bins, b, e, l):
        bin_size = int((int(e) - int(b)) / int(bins))
        begin = str(b)
        end = str(e)
        location = str(l)
        tst_sql = "SELECT enter_timestamp as atime, 1 AS ct FROM intervals WHERE " \
                " leave_timestamp >= " + begin + " and enter_timestamp <= " + end + " and Location = " + location + "" \
                " UNION " \
                " SELECT leave_timestamp as atime, 0 AS ct FROM intervals WHERE " \
                " leave_timestamp >= " + begin + " and enter_timestamp <= " + end + " and Location = " + location + "" \
                " ORDER BY atime "
        with_tst_sql = "WITH Q AS (" + tst_sql + ")"
        bin_find_sql = "round(" + str(bins) + "*(atime - " + begin + ")/(" + end + " - " + begin + "))"

        inside_sql =" SELECT " + bin_find_sql + " AS k, min(atime) as min_atime, max(atime) as max_atime, " + \
                    " FROM Q GROUP BY k"
                    # " (SELECT ct from Q WHERE atime = min_atime) as min_ct, (SELECT ct from Q WHERE atime = max_atime) as max_ct " + \
        minmax_sql = with_tst_sql + " SELECT k, atime, Q.ct FROM Q JOIN (" + inside_sql +" ) as QA " + \
                    " ON k = " + bin_find_sql + " AND (atime = min_atime or atime = max_atime) ORDER BY k, atime"
        results = self.connection.execute(minmax_sql).fetchall()
        
        locDict = [0.0] * (bins+1)

        pre_bin = -1
        for item in results:
            bin_it = int(item[0])
            interval_time_start = int(item[1])
            st_en = int(item[2])

            if bin_it < 0 or bins < bin_it:
                continue
            locDict[bin_it] = 0.5
            if st_en == 0 and bin_it != pre_bin:
                cp_bn = bin_it * bin_size
                if interval_time_start > cp_bn:
                    locDict[bin_it] = 1.0
                c_bin = bin_it
                for bit in range(c_bin - 1, 0, -1):
                    if locDict[bit] > 0:
                        break
                    locDict[bit] = 1.0
            pre_bin = bin_it

        return locDict
    
    def db_gantt_sketch(self, bins, begin, end, location):
        # print("DuckDB gantt sketch")
        elements = str((int(bins) * 3) + 1)# "1000000"
        gs_query = "SELECT enter_timestamp, leave_timestamp, Location FROM intervals" \
            " WHERE Location = " + str(location) + \
            " AND leave_timestamp >= " + str(begin) + " AND enter_timestamp <= " + str(end) + \
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
