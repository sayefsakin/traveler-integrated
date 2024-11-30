import math
import os
import psycopg2
import json
from psycopg2.extras import RealDictCursor
from pathlib import Path

class PostgresWrapper():
    def __init__(self):
        self.dataset_id = os.getenv('DATASET_ID', "")
        self.dataset_location = os.getenv('DATASET_LOCATION', "/mnt/d/Projects/mosaic_testing/mosaic/data/traveler_data")
        self.connection = self.connect_to_db()
        self.total_data = 0
        try:
            with self.connection.cursor() as cur:
                cur.execute("SELECT 1 FROM pg_catalog.pg_tables WHERE tablename = 'traveler'")
                exists = cur.fetchone()
                if not exists:
                    cur.execute(
                        """ CREATE TABLE if not exists mytr.traveler(
                            id SERIAL PRIMARY KEY,
                            data JSONB
                        ) """
                    )
                    print("copying table data in postgres")
                    cur.execute(
                        " COPY mytr.traveler(data) " +
                           " FROM PROGRAM 'jq -c \".[]\" " + 
                           self.dataset_location + "/" + self.dataset_id + ".json'"
                    )
                    print("copying table data in postgres completed")
                    print("creating indexes in postgres")
                    cur.execute("CREATE INDEX enter_index ON mytr.traveler USING BTREE ((data->'enter'->>'Timestamp'));")
                    cur.execute("CREATE INDEX leave_index ON mytr.traveler USING BTREE ((data->'leave'->>'Timestamp'));")
                    print("creating indexes in postgres completed")
                else:
                    print("traveler table exist in postgres")

                cur.execute("SELECT count(*) FROM mytr.traveler")
                exists = cur.fetchone()
                self.total_data = exists[0]
                # print(self.db_get_parent_child_trace("184", "253203368", "362625818"))
                # print("testing min_max")
                # print(self.db_min_max_test("50", "202591429", "278066359", "5"))
                # print(self.db_gantt_sketch(50, 202591429, 278066359, "5"))
        except Exception as e:
            print(f"Error connecting to database: {e}")
        finally:
            self.connection.commit()

    def connect_to_db(self):
        try:
            conn = psycopg2.connect(
                dbname="traveler",
                user="sayefsakin",
                password="sayefsakin",
                host="localhost",
                port="5432"
            )
            return conn
        except Exception as e:
            print(f"Error connecting to database: {e}")
            return None


    def closeConnection(self):
        self.connection.close()

    # def db_agg_test(self, bins, b, e, l):
    #     bin_size = str(int((int(e) - int(b)) / int(bins)))
    #     begin = str(b)
    #     end = str(e)
    #     location = str(l)
    #     only_bin_sql = " SELECT unnest(generate_series(" + begin + ", " + end + ", " + bin_size  + ")) AS bin " \
    #             " FROM intervals AS t " \
    #             " WHERE " \
    #             " t.leave_timestamp >= " + begin + " and t.enter_timestamp <= " + end + " and t.Location = " + location + ""
    #     tst_sql = "SELECT enter_timestamp as atime, 1 AS ct FROM intervals WHERE " \
    #             " leave_timestamp >= " + begin + " and enter_timestamp <= " + end + " and Location = " + location + "" \
    #             " UNION " \
    #             " SELECT leave_timestamp as atime, 0 AS ct FROM intervals WHERE " \
    #             " leave_timestamp >= " + begin + " and enter_timestamp <= " + end + " and Location = " + location + "" \
    #             " ORDER BY atime "
    #     cplx_sql = "SELECT bin_t.bin AS bn, min(util.atime) AS rn FROM " \
    #             " ( " + only_bin_sql + " ) AS bin_t INNER JOIN ( " + tst_sql + "" \
    #             " ) AS util " \
    #             " ON bin_t.bin <= util.atime " \
    #             " GROUP BY bin_t.bin"
    #     cplx_with_ct_sql = "SELECT " \
    #             " CASE WHEN tst.ct = 0 AND tst.atime - cp.bn >= " + bin_size  + " THEN 1 " \
    #                 " WHEN tst.ct = 0 AND tst.atime - cp.bn < " + bin_size  + " THEN 0.5 " \
    #                 " WHEN tst.atime - cp.bn < " + bin_size  + " THEN 0.5 " \
    #                 " ELSE 0 END as utl " \
    #             " FROM " \
    #             " ( " + cplx_sql + " ) AS cp INNER JOIN ( " + tst_sql + " ) AS tst ON cp.rn = tst.atime " \
    #             " ORDER BY cp.bn "
    #     results = self.connection.execute(cplx_with_ct_sql).fetchall()
    #     values_only = [float(t[0]) for t in results]
    #     if len(values_only) > 1:
    #         del values_only[-1]
    #     # print("duck db values: ", end='')
    #     # print(values_only)
    #     # print(len(values_only))
    #     return values_only
    
    def db_min_max_test(self, bin, b, e, l):
        bins = int(bin)
        bin_size = int((int(e) - int(b)) / int(bins))
        begin = str(b)
        end = str(e)
        location = str(l)
        tst_sql = "SELECT (data->'enter'->>'Timestamp')::int8 as atime, 1 AS ct FROM mytr.traveler WHERE " \
                " (data->'leave'->>'Timestamp')::int8 >= " + begin + " and (data->'enter'->>'Timestamp')::int8 <= " + end + " and data->>'Location' = '" + location + "'" \
                " UNION " \
                " SELECT (data->'leave'->>'Timestamp')::int8 as atime, 0 AS ct FROM mytr.traveler WHERE " \
                " (data->'leave'->>'Timestamp')::int8 >= " + begin + " and (data->'enter'->>'Timestamp')::int8 <= " + end + " and data->>'Location' = '" + location + "'" \
                " ORDER BY atime "
        with_tst_sql = "WITH Q AS (" + tst_sql + ")"
        bin_find_sql = "round(" + str(bins) + "*(atime - " + begin + ")/(" + end + " - " + begin + "))"

        inside_sql =" SELECT " + bin_find_sql + " AS k, min(atime) as min_atime, max(atime) as max_atime " + \
                    " FROM Q GROUP BY k"
        # " (SELECT ct from Q WHERE atime = min_atime) as min_ct, (SELECT ct from Q WHERE atime = max_atime) as max_ct " + \
        minmax_sql = with_tst_sql + " SELECT k, atime, Q.ct FROM Q JOIN (" + inside_sql +" ) as QA " + \
                    " ON k = " + bin_find_sql + " AND (atime = min_atime or atime = max_atime) ORDER BY k, atime"
        
        # print(minmax_sql)
        locDict = [0.0] * (bins+1)
        with self.connection.cursor() as cur:
            cur.execute(minmax_sql)
            results = cur.fetchall()
            # print(results)

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
        total_rows = (int(bins) * 3) + 1
        elements = str(int(min(total_rows / self.total_data * 100, 100))) # in percentage
        
        gs_query = "SELECT data->'enter'->>'Timestamp' AS enter_timestamp, " \
            + " data->'leave'->>'Timestamp' AS leave_timestamp, " \
            + " data->>'Location' AS Location FROM mytr.traveler" \
            + " TABLESAMPLE BERNOULLI(" + elements + ") REPEATABLE(100)" \
            + " WHERE data->>'Location' = '" + str(location) + "'" \
            + " AND (data->'leave'->>'Timestamp')::int8 >= " + str(begin) + " AND (data->'enter'->>'Timestamp')::int8 <= " + str(end)
        
        def getBinSize(time_begin: int, time_end: int, bins: int) -> int:
            return int(math.floor((time_end - time_begin) / bins))
        
        def getBinNumber(time_begin: int, time_end: int, bins: int, ctime: int) -> int:
            bin_size = getBinSize(time_begin, time_end, bins)
            if ctime < time_begin or ctime > time_end:
                return -1
            return int(math.floor((ctime - time_begin) / bin_size))

        locDict = [0.0] * bins
        bin_size = getBinSize(begin, end, bins)

        # print(gs_query)
        with self.connection.cursor() as cur:
            cur.execute(gs_query)
            results = cur.fetchall()
        
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

    def db_get_attribute_of_event(self, c_time, location):
        attr_query = "SELECT data->>'intervalId' FROM mytr.traveler WHERE " \
             + " data->>'Location' = '" + str(location) + "' AND " \
             + " (data->'enter'->>'Timestamp')::int8 <= " + str(c_time) + " AND " \
             + " (data->'leave'->>'Timestamp')::int8 >= " + str(c_time)
        
        eid = None
        with self.connection.cursor() as cur:
            cur.execute(attr_query)
            result = cur.fetchone()
            if result is not None:
                eid = result[0]
        return eid

    def db_get_parent_child_trace(self, eid, b, e):
        # print("doing postgres parent child trace")
        begin = str(b)
        end = str(e)
        ancestor_query = f"""
            WITH RECURSIVE interval_ancestors AS (
                SELECT 
                    data->>'intervalId' AS eid,
                    data->>'parent' AS parent_id,
                    data->>'Location' AS location,
                    (data->'enter'->>'Timestamp')::int8 AS enter_timestamp,
                    (data->'leave'->>'Timestamp')::int8 AS leave_timestamp,
                    1 AS depth
                FROM mytr.traveler
                WHERE data->>'intervalId' = '{eid}'

                UNION ALL

                SELECT 
                    child.data->>'intervalId' AS eid, 
                    child.data->>'parent' AS parent_id, 
                    child.data->>'Location' AS location,
                    (child.data->'enter'->>'Timestamp')::int8 AS enter_timestamp,
                    (child.data->'leave'->>'Timestamp')::int8 AS leave_timestamp,
                    parent.depth + 1 AS depth
                FROM mytr.traveler AS child
                JOIN interval_ancestors parent ON child.data->>'intervalId' = parent.parent_id
            )
            SELECT * FROM interval_ancestors
            WHERE leave_timestamp >= {begin} AND enter_timestamp <= {end}
        """
        
        descendant_query = f"""
            WITH RECURSIVE interval_descendants AS (
                SELECT
                            data->>'intervalId' AS eid,
                            data->>'parent' AS parent_id,
                            data->>'Location' AS location,
                            (data->'enter'->>'Timestamp')::int8 AS enter_timestamp,
                            (data->'leave'->>'Timestamp')::int8 AS leave_timestamp,
                            1 AS depth
                FROM mytr.traveler
                WHERE data->>'parent' = '{eid}'

                UNION ALL

                SELECT
                            child.data->>'intervalId' AS eid,
                            child.data->>'parent' AS parent_id,
                            child.data->>'Location' AS location,
                            (child.data->'enter'->>'Timestamp')::int8 AS enter_timestamp,
                            (child.data->'leave'->>'Timestamp')::int8 AS leave_timestamp,
                            parent.depth + 1 AS depth
                FROM mytr.traveler AS child
                JOIN interval_descendants parent ON child.data->>'parent' = parent.eid
            )
            SELECT * FROM interval_descendants
            WHERE leave_timestamp >= {begin} AND enter_timestamp <= {end}
        """

        ancestor_dict = {}
        descendent_dict = {}
        # print(ancestor_query)
        with self.connection.cursor() as cur:
            cur.execute(ancestor_query)
            results = cur.fetchall()

            for item in results:
                if item[1] is None or item[1] == "":
                    continue
                if item[0] == eid:
                    ancestor_dict[eid] = {
                        'enter': item[3],
                        'leave': item[4],
                        'location': item[2],
                        'parent': item[1]
                    }
                    descendent_dict[eid] = {
                        'enter': item[3],
                        'leave': item[4],
                        'location': item[2],
                        'parent': item[1]
                    }
                ancestor_dict[item[1]] = {
                    'enter': item[3],
                    'leave': item[4],
                    'location': item[2],
                    'child': item[0]
                }
            
            cur.execute(descendant_query)
            results = cur.fetchall()
            
            for item in results:
                descendent_dict[item[0]] = {
                    'enter': item[3],
                    'leave': item[4],
                    'location': item[2],
                    'parent': item[1]
                }
        return json.dumps({"ancestors":ancestor_dict, "descendants": descendent_dict})