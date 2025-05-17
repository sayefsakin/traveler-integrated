import datetime
import json
import time

from fastapi import APIRouter, HTTPException
from starlette.responses import StreamingResponse

from data_handler.data_queries import DataQueriesInterface
from data_handler.ds_profiled_data.ds_profiler import DSProfiler
from . import db, validateDataset, dsp

from data_handler import data_queries

router = APIRouter()

@router.get('/datasets/{datasetId}/metrics')
def get_procMetrics(datasetId: str):
    datasetId = validateDataset(datasetId, requiredFiles=['otf2'])
    return db[datasetId]['info']['procMetricList']

@router.get('/datasets/{datasetId}/metrics/raw')
def get_procMetric_values(datasetId: str,
                          metric: str,
                          begin: float = None,
                          end: float = None):
    datasetId = validateDataset(datasetId, requiredFiles=['otf2'], filesMustBeReady=['otf2'])

    if begin is None:
        begin = db[datasetId]['info']['intervalDomain'][0]
    if end is None:
        end = db[datasetId]['info']['intervalDomain'][1]

    def procMetricGenerator():
        yield '['
        firstItem = True
        for timestamp in db[datasetId]['procMetrics'][metric]:
            if float(timestamp) < begin or float(timestamp) > end:
                continue
            if not firstItem:
                yield ','
            yield json.dumps(db[datasetId]['procMetrics'][metric][timestamp])
            firstItem = False
        yield ']'

    return StreamingResponse(procMetricGenerator(), media_type='application/json')

@router.get('/datasets/{datasetId}/metrics/{metric}/summary')
def getMetricData(datasetId: str,
                  metric: str,
                  bins: int = 100,
                  begin: int = None,
                  end: int = None,
                  location: str = None):
    datasetId = validateDataset(datasetId, requiredFiles=['otf2'], filesMustBeReady=['otf2'])

    if begin is None:
        begin = db[datasetId]['info']['intervalDomain'][0]
    if end is None:
        end = db[datasetId]['info']['intervalDomain'][1]

    ret = {}
    if location is None:
        ret['data'] = db[datasetId]['sparseUtilizationList']['metrics'][metric].calcMetricHistogram(bins, begin, end)
    else:
        ret['data'] = db[datasetId]['sparseUtilizationList']['metrics'][metric].calcMetricHistogram(bins, begin, end, location)
    ret['metadata'] = {'begin': begin, 'end': end, 'bins': bins}
    return ret

@router.get('/datasets/{datasetId}/utilizationHistogram')
def get_utilization_histogram(datasetId: str,
                              bins: int = 100,
                              begin: int = None,
                              end: int = None,
                              locations: str = None,
                              primitive: str = None):
    datasetId = validateDataset(datasetId, requiredFiles=['otf2'], filesMustBeReady=['otf2'])


    if begin is None:
        begin = db[datasetId]['info']['intervalDomain'][0]
    if end is None:
        end = db[datasetId]['info']['intervalDomain'][1]

    ret = {}

    if locations:
        locations = locations.split(',')

    dqi = DataQueriesInterface()

    timerStart = round(time.time() * 1000000)
    ds_command = "window"
    
    utilObject = None
    if dsp.profiled_ds == dsp.SAT:
        utilObject = db[datasetId]['sparseUtilizationList']['intervals']
        
    if primitive is not None:
        if primitive not in db[datasetId]['sparseUtilizationList']['primitives']:
            raise HTTPException(status_code=404, detail='No utilization data for primitive: %s' % primitive)
        if dsp.profiled_ds == dsp.SAT:
            utilObject = db[datasetId]['sparseUtilizationList']['primitives'][primitive]
        ds_command = ds_command + "_cond"

    fetchTimer = round(time.time() * 1000000)

    calcHistorgramTimer = 0
    if locations:
        if dsp.profiled_ds == dsp.KDT or dsp.profiled_ds == dsp.SGT or dsp.profiled_ds == dsp.AGC or dsp.profiled_ds == dsp.EKM:
            ret['locations'] = dqi.GetDataInRange(bins, begin, end, locations, primitive, dsp.profiled_ds)
        else:
            ret['locations'] = {}
            for location in locations:
                if dsp.profiled_ds.startswith("db"):
                    if dsp.profiled_ds.endswith(dsp.DBTYPE_MIN_MAX):
                        ret['locations'][location] = dsp.db_wrapper.db_min_max_test(bins, begin, end, location, primitive)
                    else:
                        ret['locations'][location] = dsp.db_wrapper.db_gantt_sketch(bins, begin, end, location, primitive)
                else:
                    ret['locations'][location] = utilObject.calcUtilizationForLocation(bins, begin, end, location)
                    calcHistorgramTimer = calcHistorgramTimer + utilObject.utilLocationEstiamteTimer
    else:
        utilObject = db[datasetId]['sparseUtilizationList']['intervals']
        ret['data'] = utilObject.calcUtilizationHistogram(bins, begin, end)
    
    postProcessTimer = round(time.time() * 1000000)
    # datasetId, begin, end, bins, fetch time, post process time
    api_fetch_text = str(fetchTimer - timerStart + calcHistorgramTimer)
    if primitive is not None:
        api_fetch_text = primitive
    if locations:
        print(datasetId, str(begin), str(end), str(bins), api_fetch_text, str(postProcessTimer - fetchTimer), ds_command, sep=",")

    ret['metadata'] = {'begin': begin, 'end': end, 'bins': bins}
    return ret
