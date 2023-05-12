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

import requests

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

def conductRandomClicking(driver, timeout, p_freq, TOTAL_SAMPLE):
    domain_window = 30
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, -150)
    # wheel_element(ganttYScroller, -150)

    ganttEventCapturerElement = driver.find_elements(By.XPATH, "//*[@class='eventCapturer']")[0]
    action = ActionChains(driver)
    random.seed(10)
    click_offset = 5
    for i in range(TOTAL_SAMPLE):
        tx = random.randint(0, int(ganttEventCapturerElement.rect['y'])-click_offset)
        ty = random.randint(-int(ganttEventCapturerElement.rect['x'])+click_offset, int(ganttEventCapturerElement.rect['x'])-click_offset)
        action.move_to_element_with_offset(ganttEventCapturerElement, tx, ty).click().perform()
        startTimer = round(time.time() * 1000)
        WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
        endTimer = round(time.time() * 1000)

    print("successfully run the profiling on", driver.title)

def conductBrushing(driver, timeout, p_freq, TOTAL_SAMPLE):
    domain_window = 30
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, -150)
    # wheel_element(ganttYScroller, -150)

    element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(UtilizationLoadingVisible())

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
        c_begin = random.randint(leftHandleLocation, int(handleWindow * (domain_window / 100)) + leftHandleLocation)
        c_end = random.randint(int(handleWindow * ((100 - domain_window) / 100)) + leftHandleLocation, rightHandleLocation)
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
            # print('Iteration', '{:2d}'.format(iteration_count), end=' ')
            # print('Brush percentage', '{:3d}%'.format(brush_percentage), end=' ')
            # print('Total drawing time:', '{:5d}'.format(endTimer - startTimer), 'ms')
            iteration_count = iteration_count + 1
        previous_handle = c_end

    print("successfully run the profiling on", driver.title)

if __name__ == '__main__':
    url = "https://stackoverflow.com"
    options = webdriver.ChromeOptions()
    # options.add_argument('--headless')
    options.add_argument("--start-maximized") # open Browser in maximized mode
    #options.add_argument("disable-infobars") # disabling infobars
    # options.add_argument("--disable-extensions") # disabling extensions
    # options.add_argument("--disable-gpu") # applicable to windows os only
    # options.add_argument("--disable-dev-shm-usage") # overcome limited resource problems
    # options.add_argument("--no-sandbox") # Bypass OS security model
    # options.add_argument("--remote-debugging-port=9222")
    #options.add_argument("--incognito")

    DGEM_ID = '589ca754-ef75-426c-8d51-841cc61dc84a'
    KMEANS_ID = '8b3289c9-a740-4091-a56d-e4d55af526b5'
    LULESH_ID = '772c7330-d4eb-485b-866a-3b315063f9af'

    base_params = {
        'dataset': os.getenv('DATASET_ID', DGEM_ID),
        'baseUrl': "http://localhost:8000",
    }
    TOTAL_SAMPLE = int(os.getenv('TOTAL_SAMPLE', 20))
    QUERY_TYPE = os.getenv('QUERY_TYPE', 'window')

    timeout = 500  # in seconds
    p_freq = 0.001  # in seconds

    with webdriver.Chrome(service=ChromeService(ChromeDriverManager().install()), options=options) as driver:
        url = generateInterfaceUrl(base_params)
        driver.get(url)
        startTimer = round(time.time() * 1000000)
        element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
        endTimer = round(time.time() * 1000000)
        # print('Initial Gantt Loading Time: ', '{:9d}'.format(endTimer - startTimer), 'micros ')

        # canvas_elem = driver.find_elements(By.XPATH, "//canvas")
        # for parent_element in canvas_elem:
        #     print(parent_element.rect)

        if QUERY_TYPE == 'window':
            conductBrushing(driver, timeout, p_freq, TOTAL_SAMPLE)
        elif QUERY_TYPE == 'attribute':
            conductRandomClicking(driver, timeout, p_freq, TOTAL_SAMPLE)

