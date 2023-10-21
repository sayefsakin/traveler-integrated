import json
import socket

from data_store.sparseUtilizationList import SparseUtilizationList


class DataQueriesInterface:
    # Range parameter is a list of pair, where each pair is (begin, end) of a certain attribute.
    # Attribute order in the list defines their mapped plane. Like 0 for X-Plane,
    # 1 for Y-Plane, 2 for Z-Plane, etc.
    ## https://stackoverflow.com/questions/30329726/fastest-save-and-load-options-for-a-numpy-array
    # https://github.com/grantjenks/python-diskcache/pull/74

    host = "127.0.0.1"
    port = 8080

    def byte_length(self, i):
        return i.bit_length() + 7

    def sendOverTheSocket(self, client_socket, dict):
        # msg = "Hello from python client"
        msg = json.dumps(dict).encode()
        msg_size = f"{len(msg):016d}"
        # print('message length ' + msg_size)
        # print('message length ' + str(len(msg_size.encode())))

        client_socket.send(msg_size.encode())
        client_socket.send(msg)

    def recvOverTheSocket(self, client_socket):
        dataString = ""
        while True:
            data_len = int(client_socket.recv(16).decode())
            # print('Received from Server 1 : ' + str(data_len))
            if data_len == -1:
                break
            data = client_socket.recv(data_len).decode()
            while len(data) != data_len:
                # print('mismatched data length')
                r_data_len = data_len - len(data)
                data += client_socket.recv(r_data_len).decode()
            # print('Received from Server 2 : ')
            dataString += data

        # print('Received from Server 3 : ')
        if len(dataString) == 0:
            return {}
        return json.loads(dataString)

    def postProcessForUtilization(self, dataDict):
        # print("in post processing")
        sul = SparseUtilizationList()


        formattedResults = dict()
        formattedResults['locations'] = dict()
        allLocations = set()
        for dp in dataDict['data']:
            locKey = str(int(float(dp["location"])))
            if locKey not in formattedResults['locations']:
                formattedResults['locations'][locKey] = list()
                allLocations.add(locKey)
            formattedResults['locations'][locKey].append(int(float(dp["time"])))

        for loc in formattedResults['locations']:
            formattedResults['locations'][loc].sort()
            for idx, tm in enumerate(formattedResults['locations'][loc]):
                if idx%2 == 0:
                    sul.setIntervalAtLocation({'index': tm, 'counter': 1, 'util': 0}, str(loc))
                else:
                    sul.setIntervalAtLocation({'index': tm, 'counter': -1, 'util': 0}, str(loc))
        # print("going to finalize with locations")
        # print(allLocations)
        sul.finalize(allLocations)
        # print("sorting of loc is done")
        return sul

    def GetDataInRange(self, bins, begin, end, locations, primitive, dataStoreType: str = None):
        # print("inside the get data in range")

        dictionary = {
            "command": "GetDataInRange",
            "begin": str(begin),
            "end": str(end),
            "bins": str(bins),
            "db_store": dataStoreType
        }
        if locations:
            dictionary["locations"] = locations
        if primitive:
            dictionary["primitive"] = primitive
        # print(dictionary)

        client_socket = socket.socket()
        client_socket.connect((self.host, self.port))

        self.sendOverTheSocket(client_socket, dictionary)
        dataDict = self.recvOverTheSocket(client_socket)
        for loc in dataDict:
            dataDict[loc] = list(map(float, dataDict[loc]))

        ret = dataDict
        # ret = self.postProcessForUtilization(dataDict)
        # print("received data over socket in dict format")

        client_socket.close()
        return ret


    def GetAttributeOfEvent(self, timestamp, location, dataStoreType: str = None):
        # print("inside the get attribute of event")

        dictionary = {
            "command": "GetEventAttribute",
            "time": str(timestamp),
            "location": str(location),
            "db_store": dataStoreType
        }
        # print(dictionary)

        client_socket = socket.socket()
        client_socket.connect((self.host, self.port))

        self.sendOverTheSocket(client_socket, dictionary)
        dataDict = self.recvOverTheSocket(client_socket)
        if 'event_id' not in dataDict:
            ret = None
        else:
            ret = dataDict['event_id']
        # print("received data over socket in dict format")

        client_socket.close()
        return ret

    def GetDataForMatchedPattern(self, range, pattern_list):
        pass

    def GetEventsInRangeWithCondition(self, range, func):
        pass

    def GetTrackDetails(self, range, event_id):
        pass

    def OrderTracks(self, tracks, func):
        pass

    # attribute: the attribute based on which the structure is constructed
    def GetNeighborsDetailsOfANodeInStructure(self, attribute, attribute_value):
        pass

    def FindAttributeInStructure(self, structure, attribute, attribute_value):
        pass

    def GetStructureSummary(self, structure):
        pass

    def GetStructuresByAttributesForRange(self, range, attributes):
        pass

    def AddAnnotaton(self, range, annotation):
        pass

    def UpdateEvent(self, event_id):
        pass

    def RemoveTrack(self, tracks):
        pass