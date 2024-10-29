import time
import duckdb
import urllib.request, json 

class DBInterface:
    con = None
    db_location = None

    def makeUrlString(self, datasetId):
        baseUrl = "http://localhost:8000"
        urlString = baseUrl + "/datasets/" + datasetId + "/intervals";
        return urlString

    def db_insert(self, datasetId):
        urlString = self.makeUrlString(datasetId)
        self.db_location = "/tmp/traveler-integrated/" + datasetId + "/intervals.db"
        self.con = duckdb.connect(self.db_location)

        with urllib.request.urlopen(urlString) as url:
            with open("intervals.json", "w") as outfile:
                outfile.write(json.dumps(json.load(url)))
            # print(data)
        self.con.sql("SELECT * FROM read_json_auto('intervals.json', format = 'array')")
        self.con.close()

    def db_read(self, datasetId):
        # self.db_location = "/tmp/traveler-integrated/" + datasetId + "/intervals.db"
        self.db_location = "./intervals.db"
        con = duckdb.connect(self.db_location)
        results = con.execute("SHOW TABLES").fetchall()
        # results = con.execute("SELECT enter.Timestamp, leave.Timestamp FROM intervals.json LIMIT 10").fetchall()
        print(results)
        con.close()


    def db_agg_test(self, datasetId, bins, begin, end, locations):
        if locations is None:
            locations = ""
        else:
            locations = " WHERE Location IN (" + ','.join(locations) + ") "

        bin_size = str(int((end - begin) / bins))
        self.db_location = "/tmp/traveler-integrated/" + datasetId + "/intervals.db"
        con = duckdb.connect(self.db_location)
        pbin_sql = " SELECT " \
                        " unnest(generate_series(min(enter.Timestamp), max(leave.Timestamp), " + bin_size + ")) AS bin, " \
                        " row_number() OVER ( " \
                            " ORDER BY t.enter.Timestamp RANGE BETWEEN 0 PRECEDING AND " + bin_size + " FOLLOWING " \
                            " ) AS et, " \
                        " CASE WHEN et > 0 THEN 1 ELSE 0 END AS flg " \
                        " FROM intervals.json AS t " \
                        " GROUP BY t.enter.Timestamp"
        abin_sql = " SELECT " \
                        " unnest(generate_series(min(enter.Timestamp), max(leave.Timestamp), " + bin_size + ")) AS bin, " \
                        " count(t.enter.Timestamp) OVER ( " \
                            " ORDER BY t.enter.Timestamp RANGE BETWEEN 0 PRECEDING AND " + bin_size + " FOLLOWING " \
                            " ) AS et, " \
                        " CASE WHEN et > 0 THEN 1 ELSE 0 END AS flg " \
                        " FROM intervals.json AS t "
    #                    " GROUP BY t.enter.Timestamp"

        only_bin_sql = " SELECT unnest(generate_series(" + str(begin) + ", " + str(end) + ", " + bin_size + ")) AS bin "
                #" GROUP BY enter.Timestamp"

        bin_sql = "SELECT bin_t.bin, count(t.enter.Timestamp) FROM " \
                " ( " + only_bin_sql + " ) AS bin_t INNER JOIN intervals.json AS t " \
                " ON t.enter.Timestamp BETWEEN bin_t.bin AND bin_t.bin + " + bin_size + " " \
                " GROUP BY bin_t.bin "
        sql_str = "SELECT time_bucket(INTERVAL 7547493 MICROSECOND, strptime(enter.Timestamp, '%f') AS bucket FROM intervals.json GROUP BY bucket ORDER BY bucket"
        tst_sql = "SELECT enter.Timestamp as atime, 1 AS ct FROM intervals.json " + locations + "" \
                " UNION " \
                " SELECT leave.Timestamp as atime, 0 AS ct FROM intervals.json " + locations + "" \
                " ORDER BY atime "
        fetch_sql = "SELECT atime, ct FROM (" + tst_sql + ") as tst " \
                " WHERE atime >= 812445187 "
        cplx_sql = "SELECT bin_t.bin AS bn, min(util.atime) AS rn FROM " \
                " ( " + only_bin_sql + " ) AS bin_t LEFT JOIN ( " + tst_sql + "" \
                " ) AS util " \
                " ON bin_t.bin <= util.atime " \
                " GROUP BY bin_t.bin"
        cplx_with_ct_sql = "SELECT cp.bn, tst.atime, tst.ct, " \
                " CASE WHEN tst.ct = 0 THEN 1 " \
                    " WHEN tst.atime - cp.bn < " + bin_size + " THEN 0.5 " \
                    " ELSE 0 END AS utl " \
                " FROM " \
                " ( " + cplx_sql + " ) AS cp LEFT JOIN ( " + tst_sql + " ) AS tst ON cp.rn = tst.atime " \
                " ORDER BY cp.bn "
        preProcessTimer = round(time.time() * 1000000)
        results = con.execute(cplx_sql).fetchall()
        postProcessTimer = round(time.time() * 1000000)
        # location_test_sql = "SELECT enter.Timestamp, Location " \
        #                 " FROM intervals.json " + locations + "" \
        #                 " LIMIT 10 "
        # results = con.execute(location_test_sql).fetchall()
        print(results)
        print("total time: ", str(postProcessTimer - preProcessTimer))
        con.close()

    def db_cli_test(self, datasetId):
        self.db_location = "/tmp/traveler-integrated/" + datasetId + "/intervals.db"
        con = duckdb.connect(self.db_location, read_only=False)
        # sql_query = "DESCRIBE SELECT * FROM intervals.json LIMIT 2"
        sql_query = "CREATE INDEX s_idx ON intervals.json (GUID)"
        results = con.execute(sql_query).fetchall()
        print(results)
        con.close()

if __name__ == '__main__':
    print("duck db trying to insert from json")
    dbi = DBInterface()
    # dbi.db_insert("8b3289c9-a740-4091-a56d-e4d55af526b5")
    #db_insert()
    dbi.db_read("8b3289c9-a740-4091-a56d-e4d55af526b5")
    # dbi.db_agg_test("8b3289c9-a740-4091-a56d-e4d55af526b5", 10, 547966146, 1827283017, ["1"])
    # dbi.db_cli_test("8b3289c9-a740-4091-a56d-e4d55af526b5")
    print("input from json done.")