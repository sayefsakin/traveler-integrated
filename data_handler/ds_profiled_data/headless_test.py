import math
import os
import random
import sys
import time

from selenium import webdriver
from selenium.webdriver import Keys
from selenium.webdriver import ActionChains
from selenium.webdriver.chrome.service import Service as ChromeService
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.by import By
from webdriver_manager.chrome import ChromeDriverManager
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import NoSuchElementException
from selenium.common.exceptions import StaleElementReferenceException
from selenium.webdriver.common.desired_capabilities import DesiredCapabilities

import requests
import json

#if __name__ == '__main__':
#
#    url = "https://scrapeme.live/shop/" 
#  
#    with webdriver.Chrome(service=ChromeService(ChromeDriverManager().install())) as driver:
#        driver.get(url)


def getUrlStringFromDict(params):
    url_string = ''
    is_first = True
    for pkey in params:
        if is_first:
            is_first = False
        else:
            url_string += '&'
        url_string += pkey + '=' + str(params[pkey])
    return url_string

def generateDatasetDetailsUrl(params):
    return params['baseUrl'] + '/datasets/' + params['dataset']


def generateUtilHistogramUrl(params, e_params):
    return generateDatasetDetailsUrl(params) + '/utilizationHistogram?' + getUrlStringFromDict(e_params)


def getDatasetDetails(params):
    url = generateDatasetDetailsUrl(params)
    return requests.get(url).json()


def generateInterfaceUrl(params):
    return params['baseUrl'] + '/static/interface.html#' + params['dataset']


def wheel_element(element, deltaY = 120, offsetX = 0, offsetY = 0):
    error = element._parent.execute_script("""
        var element = arguments[0];
        var deltaY = arguments[1];
        var box = element.getBoundingClientRect();
        var clientX = box.left + (arguments[2] || box.width / 2);
        var clientY = box.top + (arguments[3] || box.height / 2);
        var target = element.ownerDocument.elementFromPoint(clientX, clientY);
        console.log(box.width);
        console.log(box.height);
        for (var e = target; e; e = e.parentElement) {
          if (e === element) {
            target.dispatchEvent(new MouseEvent('mouseover', {view: window, bubbles: true, cancelable: true, clientX: clientX, clientY: clientY}));
            target.dispatchEvent(new MouseEvent('mousemove', {view: window, bubbles: true, cancelable: true, clientX: clientX, clientY: clientY}));
            target.dispatchEvent(new WheelEvent('wheel',     {view: window, bubbles: true, cancelable: true, clientX: clientX, clientY: clientY, deltaY: deltaY}));
            return;
          }
        }    
        return "Element is not interactable";
        """, element, deltaY, offsetX, offsetY)
    if error:
        raise WebDriverException(error)


class GanttLoadingVisible():
    def __init__(self):
        pass

    def __call__(self, web_driver):
        loading_elem = web_driver.find_elements(By.XPATH, "//*[@class='overlayShadowEl shadowed']")
        for each_element in loading_elem:
            following_sibling = each_element.find_element(By.XPATH, "preceding-sibling::*")
            if following_sibling.get_attribute("class") == 'GLView ZoomableTimelineView':
                if each_element.is_displayed() is False:
                    return each_element
        return False


class MozSketchLoadingVisible():
    def __init__(self):
        pass

    def __call__(self, web_driver):
        loading_elem = web_driver.find_elements(By.XPATH, "//*[@class='selectionHeader']")
        for each_element in loading_elem:
            following_sibling = each_element.find_element(By.XPATH, "preceding-sibling::*")
            if following_sibling.get_attribute("class") == 'GLView MosaicGanttView':
                if each_element.is_displayed() is False:
                    return each_element
        return False


class UtilizationLoadingVisible():
    def __init__(self):
        pass

    def __call__(self, web_driver):
        loading_elem = web_driver.find_elements(By.XPATH, "//*[@class='overlayShadowEl shadowed']")
        for each_element in loading_elem:
            following_sibling = each_element.find_element(By.XPATH, "preceding-sibling::*")
            if following_sibling.get_attribute("class") == 'GLView UtilizationView':
                if each_element.is_displayed() is False:
                    return each_element
        return False

class SelectionEelmentNotStale():
    def __init__(self):
        pass

    def __call__(self, web_driver):
        try:
            h5_element = driver.find_element(By.XPATH, "//h5")
            if(h5_element):
                return h5_element.text
        except StaleElementReferenceException:
            return False
        return False

def checkClickValidity(driver, timeout, p_freq, tx, ty):
    selection_text = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(SelectionEelmentNotStale())
    if selection_text == "Interval Selection:":
        return True
    return False

def getNextCircularPoint(tx, ty):
    if tx == 0 and ty == 0:
        return 1, 0
    dx = tx
    dy = ty
    angle = math.degrees(math.atan2(float(dy), float(dx)))
    if angle < 45 and angle > -45:
        dy = dy - 1
    elif angle < - 134 or angle > 135:
        dy = dy + 1
    elif angle < 136 and angle > 44:
        dx = dx + 1
    else:
        dx = dx - 1
    return dx, dy

def scrollToWindow(driver, timeout, p_freq):
    hoverTarget = driver.find_elements(By.XPATH, "//*[@class='hoverTarget']")
    rightHandleLocation = 0
    leftHandleLocation = 0
    
    for each_hover in hoverTarget:
        parent_element = each_hover.find_element(By.XPATH, "..")
        if parent_element.get_attribute("class") == 'rightHandle':
            rightHandleLocation = each_hover.location['x']
        else:
            leftHandleLocation = each_hover.location['x']
    handleWindow = rightHandleLocation - leftHandleLocation

    iteration_count = 1
    previous_handle = rightHandleLocation

    for i in range(2): # for kmeans
        c_begin = int(handleWindow * (85 / 100)) + leftHandleLocation
        c_end = int(handleWindow * (86 / 100)) + leftHandleLocation
        for each_hover in hoverTarget:
            parent_element = each_hover.find_element(By.XPATH, "..")
            start = each_hover.location
            drag_offset = c_begin - start['x']
            current_handle = c_begin
            if parent_element.get_attribute("class") == 'rightHandle':
                drag_offset = c_end - start['x']
                previous_handle = c_begin
                current_handle = c_end
            ActionChains(driver).drag_and_drop_by_offset(each_hover, drag_offset, 0).perform()
            startTimer = round(time.time() * 1000000)
            WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
            endTimer = round(time.time() * 1000000)
            brush_percentage = int(abs(previous_handle - current_handle) / handleWindow * 100.0)
            # print(str(iteration_count), str(brush_percentage), str(endTimer - startTimer), sep=",")
            # print('Iteration', '{:2d}'.format(iteration_count), end=' ')
            # print('Brush percentage', '{:3d}%'.format(brush_percentage), end=' ')
            # print('Total drawing time:', '{:5d}'.format(endTimer - startTimer), 'ms')
            iteration_count = iteration_count + 1
        previous_handle = c_end

def conductRandomClicking(driver, timeout, p_freq, TOTAL_SAMPLE):
    scrollToWindow(driver, timeout, p_freq)
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, -150)
    # wheel_element(ganttYScroller, -150)

    ganttEventCapturerElement = driver.find_elements(By.XPATH, "//*[@class='eventCapturer']")[0]
    action = ActionChains(driver)
    random.seed(10)
    click_offset = 5
    x_bounds = [0, int(ganttEventCapturerElement.rect['y'])-click_offset]
    y_bounds = [-int(ganttEventCapturerElement.rect['x'])+click_offset, int(ganttEventCapturerElement.rect['x'])-click_offset]
    i = 0
    while(True):
        if i >= TOTAL_SAMPLE:
            break
        tx = random.randint(x_bounds[0], x_bounds[1])
        ty = random.randint(y_bounds[0], y_bounds[1])
        ntx = tx
        nty = ty
        while(True):
            action.move_to_element_with_offset(ganttEventCapturerElement, ntx, nty).click().perform()
            WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
            #i = i + 1
            #break
            if(checkClickValidity(driver, timeout, p_freq, ntx, nty)):
                i = i + 1
                break
            else:
                # print("not found", ntx, nty)
                for ctr in range(8):
                    rtx, rty = getNextCircularPoint(ntx - tx, nty - ty)
                    ntx = tx + rtx
                    nty = ty + rty
                    if x_bounds[0] <= ntx <= x_bounds[1] and y_bounds[0] <= nty <= y_bounds[1]:
                        break
                break
                    

    print("successfully run the profiling on", driver.title)

def conductBrushing(driver, timeout, p_freq, TOTAL_SAMPLE):
    time.sleep(3)
    domain_window = 90
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, -150)

    # element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(UtilizationLoadingVisible())
    hoverTarget = driver.find_elements(By.XPATH, "//*[@class='hoverTarget']")
    rightHandleLocation = 0
    leftHandleLocation = 0
    
    for each_hover in hoverTarget:
        parent_element = each_hover.find_element(By.XPATH, "..")
        if parent_element.get_attribute("class") == 'rightHandle':
            rightHandleLocation = each_hover.location['x']
        else:
            leftHandleLocation = each_hover.location['x']
    handleWindow = rightHandleLocation - leftHandleLocation

    iteration_count = 1
    previous_handle = rightHandleLocation

    print("iteration,brush percentage,total drawing time (micros)")
    random.seed(10)
    for i in range(int(TOTAL_SAMPLE/2)):
        # c_begin = random.randint(leftHandleLocation, int(handleWindow * (domain_window / 100)) + leftHandleLocation)
        # c_end = random.randint(int(handleWindow * ((100 - domain_window) / 100)) + leftHandleLocation, rightHandleLocation)
        left_handle_list = random.randint(65, 90) # for kmeans
        c_begin = int(handleWindow * (left_handle_list / 100)) + leftHandleLocation
        c_end = int(handleWindow * ((left_handle_list + random.randint(4,10)) / 100)) + leftHandleLocation
        # print(leftHandleLocation, c_begin, c_end, rightHandleLocation)
        for each_hover in hoverTarget:
            parent_element = each_hover.find_element(By.XPATH, "..")
            start = each_hover.location
            drag_offset = c_begin - start['x']
            current_handle = c_begin
            if parent_element.get_attribute("class") == 'rightHandle':
                drag_offset = c_end - start['x']
                previous_handle = c_begin
                current_handle = c_end
            ActionChains(driver).drag_and_drop_by_offset(each_hover, drag_offset, 0).perform()
            startTimer = round(time.time() * 1000000)
            WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
            endTimer = round(time.time() * 1000000)
            brush_percentage = int(abs(previous_handle - current_handle) / handleWindow * 100.0)
            print(str(iteration_count), str(brush_percentage), str(endTimer - startTimer), sep=",")
            time.sleep(3000)
            # print('Iteration', '{:2d}'.format(iteration_count), end=' ')
            # print('Brush percentage', '{:3d}%'.format(brush_percentage), end=' ')
            # print('Total drawing time:', '{:5d}'.format(endTimer - startTimer), 'ms')
            iteration_count = iteration_count + 1
        previous_handle = c_end

    # print("successfully run the profiling on", driver.title)

def conductPNGOutput(driver, params):
    loading_elem = driver.find_elements(By.XPATH, "//*[@class='scrollArea GLView SelectionInfoView']")
    json_data = {}
    for each_element in loading_elem:
        following_sibling = each_element.find_element(By.TAG_NAME, "pre")
        try:
            json_data = json.loads(following_sibling.text)
            break
        except json.JSONDecodeError as e:
            print("Failed to parse JSON:", e)
            return
    
    time.sleep(3)
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, -150)

    element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
    time.sleep(10)
    if 'exportStartTime' in params:
        # element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(UtilizationLoadingVisible())
        hoverTarget = driver.find_elements(By.XPATH, "//*[@class='hoverTarget']")
        rightHandleLocation = 0
        leftHandleLocation = 0
        
        for each_hover in hoverTarget:
            parent_element = each_hover.find_element(By.XPATH, "..")
            if parent_element.get_attribute("class") == 'rightHandle':
                rightHandleLocation = each_hover.location['x']
            else:
                leftHandleLocation = each_hover.location['x']
        handleWindow = rightHandleLocation - leftHandleLocation

        start_boundary = json_data["intervalDomain"][0]
        end_boundary = json_data["intervalDomain"][1]
    
        start_time = params['exportStartTime']
        end_time = params['exportEndTime']

        start_p = (start_time - start_boundary) / (end_boundary - start_boundary)
        end_p = (end_time - start_boundary) / (end_boundary - start_boundary)

        c_begin = int(handleWindow * start_p) + leftHandleLocation
        c_end = int(handleWindow * end_p) + leftHandleLocation
        # print(leftHandleLocation, c_begin, c_end, rightHandleLocation)
        for each_hover in hoverTarget:
            parent_element = each_hover.find_element(By.XPATH, "..")
            start = each_hover.location
            drag_offset = c_begin - start['x']
            if parent_element.get_attribute("class") == 'rightHandle':
                drag_offset = c_end - start['x']
            ActionChains(driver).drag_and_drop_by_offset(each_hover, drag_offset, 0).perform()
            WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())

    ganttEventCapturerElement = driver.find_elements(By.XPATH, "//*[@class='GLView ZoomableTimelineView']")[0]
    exported_file_name = params['exportLocation'] + "/" + params['dataset'] + "_gantt.png"
    ganttEventCapturerElement.screenshot(exported_file_name)
    print("Saved screenshot from of the Gantt chart at: ", exported_file_name)

def conductFixedScrolling(driver, timeout, p_freq, TOTAL_SAMPLE):
    time.sleep(3)
    domain_window = 90
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, 150)
    # wheel_element(ganttYScroller, -150)
    time.sleep(3)

    # canvas_element = driver.find_elements(By.XPATH, "//canvas")
    # foreign_element = canvas_element.find_element(By.XPATH, "..")

    ganttXScroller = driver.find_element(By.CLASS_NAME, "eventCapturer")
    if ganttXScroller is None:
        print("not found")
    else:
        print("X scroller found")
    for i in range(5):
        wheel_element(ganttXScroller)
        print("scrolling")
        time.sleep(3)

    # # element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(UtilizationLoadingVisible())
    # hoverTarget = driver.find_elements(By.XPATH, "//*[@class='hoverTarget']")
    # rightHandleLocation = 0
    # leftHandleLocation = 0
    
    # for each_hover in hoverTarget:
    #     parent_element = each_hover.find_element(By.XPATH, "..")
    #     if parent_element.get_attribute("class") == 'rightHandle':
    #         rightHandleLocation = each_hover.location['x']
    #     else:
    #         leftHandleLocation = each_hover.location['x']
    # handleWindow = rightHandleLocation - leftHandleLocation

    # iteration_count = 1
    # previous_handle = rightHandleLocation

    # print("iteration,brush percentage,total drawing time (micros)")
    # random.seed(10)
    # for i in range(int(TOTAL_SAMPLE/2)):
    #     c_begin = random.randint(leftHandleLocation, int(handleWindow * (domain_window / 100)) + leftHandleLocation)
    #     c_end = random.randint(int(handleWindow * ((100 - domain_window) / 100)) + leftHandleLocation, rightHandleLocation)
    #     # print(leftHandleLocation, c_begin, c_end, rightHandleLocation)
    #     for each_hover in hoverTarget:
    #         parent_element = each_hover.find_element(By.XPATH, "..")
    #         start = each_hover.location
    #         drag_offset = c_begin - start['x']
    #         current_handle = c_begin
    #         if parent_element.get_attribute("class") == 'rightHandle':
    #             drag_offset = c_end - start['x']
    #             previous_handle = c_begin
    #             current_handle = c_end
    #         ActionChains(driver).drag_and_drop_by_offset(each_hover, drag_offset, 0).perform()
    #         startTimer = round(time.time() * 1000000)
    #         WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
    #         endTimer = round(time.time() * 1000000)
    #         brush_percentage = int(abs(previous_handle - current_handle) / handleWindow * 100.0)
    #         print(str(iteration_count), str(brush_percentage), str(endTimer - startTimer), sep=",")
    #         # print('Iteration', '{:2d}'.format(iteration_count), end=' ')
    #         # print('Brush percentage', '{:3d}%'.format(brush_percentage), end=' ')
    #         # print('Total drawing time:', '{:5d}'.format(endTimer - startTimer), 'ms')
    #         iteration_count = iteration_count + 1
    #     previous_handle = c_end

    # print("successfully run fixed scrolling profiling on", driver.title)

if __name__ == '__main__':
    url = "https://stackoverflow.com"
    options = webdriver.ChromeOptions()
    options.add_argument('--headless')
    options.add_argument("--start-maximized") # open Browser in maximized mode
    #options.add_argument("disable-infobars") # disabling infobars
    # options.add_argument("--disable-extensions") # disabling extensions
    # options.add_argument("--disable-gpu") # applicable to windows os only
    # options.add_argument("--disable-dev-shm-usage") # overcome limited resource problems
    # options.add_argument("--no-sandbox") # Bypass OS security model
    # options.add_argument("--remote-debugging-port=9222")
    #options.add_argument("--incognito")

    DGEM_ID = 'a9bd20ca-c4f2-4b54-8c49-b968ae7e78be'#'589ca754-ef75-426c-8d51-841cc61dc84a'
    KMEANS_ID = '8b3289c9-a740-4091-a56d-e4d55af526b5'#'faf17535-2f66-4621-995f-49c7dbd84e8b'
    LULESH_ID = '772c7330-d4eb-485b-866a-3b315063f9af'

    base_params = {
        'dataset': os.getenv('DATASET_ID', DGEM_ID),
        'baseUrl': os.getenv('BASE_URL', "http://localhost:8000"),
        'exportLocation': "."
    }
    TOTAL_SAMPLE = int(os.getenv('TOTAL_SAMPLE', 20))
    QUERY_TYPE = os.getenv('QUERY_TYPE', 'window')
    if len(sys.argv) > 1:
        base_params['exportLocation'] = sys.argv[1]
    if len(sys.argv) > 2:
        base_params['exportStartTime'] = sys.argv[2]
    if len(sys.argv) > 3:
        base_params['exportEndTime'] = int(sys.argv[3])

    timeout = 500  # in seconds
    p_freq = 0.001  # in seconds

    # Define paths
    user_home_dir = os.path.expanduser("/home/sayefsakin/selenium_testing")
    chrome_binary_path = os.path.join(user_home_dir, "chrome-linux64", "chrome")
    chromedriver_path = os.path.join(user_home_dir, "chromedriver-linux64", "chromedriver")

    # Set binary location and service
    options.binary_location = chrome_binary_path
    options.set_capability('goog:loggingPrefs', {'performance': 'ALL'})
    service = ChromeService(chromedriver_path)

    with webdriver.Chrome(service=service, options=options) as driver:
    #with webdriver.Chrome(service=ChromeService(ChromeDriverManager().install()), options=options) as driver:
        url = generateInterfaceUrl(base_params)
        driver.get(url)
        startTimer = round(time.time() * 1000000)
        element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
        #element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(MozSketchLoadingVisible())
        endTimer = round(time.time() * 1000000)
        # timing = driver.execute_script("return window.performance.timing;")
        # print("Iniital Gantt Render time: ", timing["domContentLoadedEventEnd"] - timing["navigationStart"])
        conductPNGOutput(driver, base_params)
        # canvas_elem = driver.find_elements(By.XPATH, "//canvas")
        # for parent_element in canvas_elem:
        #     print(parent_element.rect)

        # conductFixedScrolling(driver, timeout, p_freq, TOTAL_SAMPLE)
        # time.sleep(3000)
        # if QUERY_TYPE == 'window':
        #     conductBrushing(driver, timeout, p_freq, TOTAL_SAMPLE)
        # elif QUERY_TYPE == 'attribute':
        #     conductRandomClicking(driver, timeout, p_freq, TOTAL_SAMPLE)
        # elif QUERY_TYPE == 'cond':
        #     time.sleep(60)
        # print('Initial Gantt Loading Time: ', '{:9d}'.format(endTimer - startTimer), 'micros ')
        # for entry in driver.get_log('performance'):
        #     print(entry)
