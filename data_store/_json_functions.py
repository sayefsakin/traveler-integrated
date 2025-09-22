import copy
import os
import re
import gc
import time

import diskcache
from . import logToConsole
import ijson

# Helper function from https://stackoverflow.com/a/4836734/1058935 for
# human-friendly location name sorting
def natural_sort(l):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
    return sorted(l, key = alphanum_key)

async def processJSON(self, datasetId, file, log=logToConsole):
    # Run each substep, with manual calls to python's garbage collector in
    # between
    priorBuildTime = round(time.time() * 1000)
    print('FROM JSON parser')
    await self.processRawJSON(datasetId, file, log)
    gc.collect()
    await self.buildIntervalTree(datasetId, log)
    gc.collect()
    # # await self.connectIntervals(datasetId, log)
    # gc.collect()
    priorSparseTime = round(time.time() * 1000)
    await self.buildSparseUtilizationLists(datasetId, log)
    gc.collect()
    # # await self.buildDependencyTree(datasetId, log)
    # gc.collect()
    self.finishLoadingSourceFile(datasetId, file)

    postBuildTime = round(time.time() * 1000)
    totalBuildTime = priorSparseTime - priorBuildTime
    totalSATBuildTime = postBuildTime - priorBuildTime
    print('Construction time: ', totalBuildTime, 'ms')
    print('SAT Construction time: ', totalSATBuildTime, 'ms')

async def processRawJSON(self, datasetId, file, log):
    idDir = os.path.join(self.dbDir, datasetId)
    intervals = self[datasetId]['intervals'] = diskcache.Index(os.path.join(idDir, 'intervals.diskCacheIndex'))

    self.original_max_time = 0
    self.original_max_location = 0
    intervalDomain = [float('inf'), float('-inf')]
    newListedLocations = []

    # Temporary counters / lists for sorting
    numEvents = 0
    await log('Parsing JSON events (.=2500 events)')

    with open(file, 'r', encoding='utf-8') as f:
        objects = ijson.items(f, 'item')
        for current_obj in objects:
            if current_obj:
                # print(current_obj)
                if 'ts' in current_obj:
                    event_obj = dict()
                    event_obj["intervalId"] = str(numEvents)
                    event_obj["GUID"] = str(current_obj.get('id', '0'))
                    event_obj["Primitive"] = current_obj.get('name', '(primitive name missing)')
                    event_obj["cat"] = current_obj.get('cat', '')
                    event_obj["pid"] = current_obj.get('pid', '')
                    event_obj["tid"] = current_obj.get('tid', 0)
                    event_obj["Location"] = str(current_obj['args']['p_idx']) # str(current_obj.get('tid', '0'))
                    event_obj["enter"] = {
                        "Timestamp": int(current_obj['ts']),
                        "Event": "ENTER"
                    }
                    event_obj["leave"] = {
                        "Timestamp": int(current_obj['ts']) + int(current_obj.get('dur', 0)),
                        "Event": "LEAVE"
                    }
                    event_obj["args"] = current_obj.get('args', {})

                    if event_obj['Location'] not in newListedLocations:
                        newListedLocations.append(event_obj['Location'])
                    intervalDomain[0] = min(intervalDomain[0], event_obj['enter']['Timestamp'])
                    intervalDomain[1] = max(intervalDomain[1], event_obj['leave']['Timestamp'])
                    intervals[event_obj['intervalId']] = copy.deepcopy(event_obj)
                    if event_obj["Primitive"] not in self[datasetId]['primitives']:
                        self[datasetId]['primitives'][event_obj["Primitive"]] = {'parents': [], 'children': [], 'name': event_obj["Primitive"]}

                    numEvents = self[datasetId].get('numEvents', 0) + 1
                    self[datasetId]['numEvents'] = numEvents
                    if numEvents % 2500 == 0:
                        await log('.', end='')
                    if numEvents % 100000 == 0:
                        await log('processed %i events' % numEvents)

    self[datasetId]['info']['intervalDomain'] = intervalDomain
    self[datasetId]['info']['locationNames'] = newListedLocations
    await log('')
    await log('Finished processing %i events' % numEvents)
