import json
import socket


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
        print('message length ' + msg_size)
        print('message length ' + str(len(msg_size.encode())))

        client_socket.send(msg_size.encode())
        client_socket.send(msg)

    def recvOverTheSocket(self, client_socket):
        data = client_socket.recv(16).decode()
        print('Received from Server 1 : ' + data)

        data = client_socket.recv(int(data)).decode()
        print('Received from Server 2 : ' + data)
        if len(data) == 0:
            return {}
        return json.loads(data)

    def GetDataInRange(self, bins, begin, end, locations, primitive):
        print("inside the get data in range")

        dictionary = {
            "begin": str(begin),
            "end": str(end)
        }

        client_socket = socket.socket()
        client_socket.connect((self.host, self.port))

        self.sendOverTheSocket(client_socket, dictionary)
        dataDict = self.recvOverTheSocket(client_socket)
        print("received data over socket in dict format")
        print(dataDict["project"])

        client_socket.close()


        return [0,0,0,0]


    def GetAttributeOfEvent(self, event_id):
        pass

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