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

from skimage.io import imread
from skimage.metrics import structural_similarity as ssim

#if __name__ == '__main__':
#
#    url = "https://scrapeme.live/shop/" 
#  
#    with webdriver.Chrome(service=ChromeService(ChromeDriverManager().install())) as driver:
#        driver.get(url)

class INPMeasurer:
    def __init__(self, driver):
        self.driver = driver
        self.inp_values = []
        
    def setup_inp_observer(self):
        """Setup multiple methods to capture INP measurements"""
        script = """
        window.inpValues = [];
        window.inpObserver = null;
        window.manualTimings = [];
        
        // Method 1: Try Performance Observer for event timing
        function setupEventObserver() {
            if ('PerformanceObserver' in window && PerformanceObserver.supportedEntryTypes.includes('event')) {
                const observer = new PerformanceObserver((list) => {
                    for (const entry of list.getEntries()) {
                        console.log('Event entry captured:', entry);
                        const inp = {
                            method: 'PerformanceObserver',
                            name: entry.name,
                            startTime: entry.startTime,
                            processingStart: entry.processingStart || entry.startTime,
                            processingEnd: entry.processingEnd || entry.startTime + entry.duration,
                            duration: entry.duration,
                            target: entry.target ? entry.target.tagName : 'unknown'
                        };
                        window.inpValues.push(inp);
                    }
                });
                
                try {
                    observer.observe({type: 'event', buffered: true});
                    window.inpObserver = observer;
                    console.log('Event observer setup successful');
                    return true;
                } catch (e) {
                    console.log('Event observer failed:', e);
                    return false;
                }
            }
            return false;
        }
        
        // Method 2: Manual timing with event listeners
        function setupManualTiming() {
            const interactionTypes = ['click', 'mousedown', 'keydown', 'pointerdown', 'touchstart'];
            
            interactionTypes.forEach(eventType => {
                document.addEventListener(eventType, function(event) {
                    const startTime = performance.now();
                    
                    // Use requestAnimationFrame to measure till next paint
                    requestAnimationFrame(() => {
                        requestAnimationFrame(() => {
                            const endTime = performance.now();
                            const duration = endTime - startTime;
                            
                            const inp = {
                                method: 'ManualTiming',
                                name: eventType,
                                startTime: startTime * 1000, // Convert to microseconds
                                processingStart: startTime * 1000,
                                processingEnd: endTime * 1000,
                                duration: duration * 1000, // Convert to microseconds
                                target: event.target ? event.target.tagName : 'unknown'
                            };
                            
                            window.inpValues.push(inp);
                            window.manualTimings.push(inp);
                            console.log('Manual timing captured:', inp);
                        });
                    });
                }, true);
            });
            
            console.log('Manual timing setup complete');
            return true;
        }
        
        // Method 3: Use Long Task API as fallback
        function setupLongTaskObserver() {
            if ('PerformanceObserver' in window && PerformanceObserver.supportedEntryTypes.includes('longtask')) {
                const observer = new PerformanceObserver((list) => {
                    for (const entry of list.getEntries()) {
                        console.log('Long task detected:', entry);
                        // Long tasks can indicate slow interactions
                        const inp = {
                            method: 'LongTask',
                            name: 'longtask',
                            startTime: entry.startTime * 1000, // Convert to microseconds
                            processingStart: entry.startTime * 1000,
                            processingEnd: (entry.startTime + entry.duration) * 1000,
                            duration: entry.duration * 1000, // Convert to microseconds
                            target: 'unknown'
                        };
                        window.inpValues.push(inp);
                    }
                });
                
                try {
                    observer.observe({type: 'longtask', buffered: true});
                    console.log('Long task observer setup successful');
                    return true;
                } catch (e) {
                    console.log('Long task observer failed:', e);
                    return false;
                }
            }
            return false;
        }
        
        // Setup all methods
        const eventObserver = setupEventObserver();
        const manualTiming = setupManualTiming();
        const longTaskObserver = setupLongTaskObserver();
        
        return {
            eventObserver: eventObserver,
            manualTiming: manualTiming,
            longTaskObserver: longTaskObserver,
            supportedTypes: PerformanceObserver.supportedEntryTypes || []
        };
        """
        
        result = self.driver.execute_script(script)
        # print(f"Setup result: {result}")
        if not any(result.values()):
            # print("Warning: No INP measurement methods available")
            return False
        return True
    
    def get_inp_values(self):
        """Retrieve collected INP values"""
        script = "return window.inpValues || [];"
        return self.driver.execute_script(script)
    
    def clear_inp_values(self):
        """Clear collected INP values"""
        self.driver.execute_script("window.inpValues = [];")
    
    def measure_interaction_inp(self, interaction_func, interaction_name=""):
        """Measure INP for a specific interaction with multiple methods"""
        # print(f"\n--- Measuring INP for: {interaction_name} ---")
        
        # Clear previous values
        self.clear_inp_values()
        
        # Record timestamp before interaction
        start_time = time.time()
        js_start_time = self.driver.execute_script("return performance.now();")
        
        # Pre-interaction state
        pre_values = self.get_inp_values()
        # print(f"Pre-interaction INP values: {len(pre_values)}")
        
        # Perform the interaction
        try:
            interaction_func()
            # print(f"Interaction '{interaction_name}' executed successfully")
        except Exception as e:
            # print(f"Error during interaction: {e}")
            return None
        
        # Wait for processing and painting
        time.sleep(0.5)  # Increased wait time
        
        # Get INP values after interaction
        inp_data = self.get_inp_values()
        # print(f"Post-interaction INP values: {len(inp_data)}")
        
        # Debug: Print all captured values
        # for i, inp in enumerate(inp_data):
        #     print(f"  [{i}] {inp['method']}: {inp['name']} - {inp['duration']:.0f}μs")
        
        # Filter values that occurred after our interaction started
        recent_interactions = [
            inp for inp in inp_data 
            if inp['startTime'] >= (js_start_time * 1000) - 100000  # Convert to microseconds with buffer
        ]
        
        # print(f"Recent interactions found: {len(recent_interactions)}")
        
        if recent_interactions:
            # Get the most recent interaction
            latest_inp = max(recent_interactions, key=lambda x: x['startTime'])
            # print(f"✅ INP for {interaction_name}: {latest_inp['duration']:.0f}μs (method: {latest_inp['method']})")
            return latest_inp
        else:
            # print(f"❌ No INP data captured for {interaction_name}")
            
            # Try to get any available data as fallback
            if inp_data:
                latest_any = inp_data[-1]  # Get the most recent one
                # print(f"📊 Using latest available data: {latest_any['duration']:.0f}μs")
                return latest_any
            
            return None
    
    def debug_performance_support(self):
        """Check what performance APIs are supported"""
        script = """
        const support = {
            performanceObserver: 'PerformanceObserver' in window,
            supportedEntryTypes: PerformanceObserver.supportedEntryTypes || [],
            performanceNow: 'performance' in window && 'now' in performance,
            userAgent: navigator.userAgent,
            webdriver: navigator.webdriver
        };
        return support;
        """
        return self.driver.execute_script(script)
    
    def force_interaction_timing(self, element, interaction_type='click'):
        """Manually measure interaction timing"""
        script = f"""
        const element = arguments[0];
        const interactionType = arguments[1];
        
        return new Promise((resolve) => {{
            const startTime = performance.now();
            
            // Create and dispatch event
            let event;
            if (interactionType === 'click') {{
                event = new MouseEvent('click', {{
                    view: window,
                    bubbles: true,
                    cancelable: true
                }});
            }} else if (interactionType === 'keydown') {{
                event = new KeyboardEvent('keydown', {{
                    key: 'Enter',
                    bubbles: true,
                    cancelable: true
                }});
            }}
            
            // Listen for next frame after interaction
            requestAnimationFrame(() => {{
                requestAnimationFrame(() => {{
                    const endTime = performance.now();
                    const duration = endTime - startTime;
                    
                    resolve({{
                        method: 'ForceManual',
                        name: interactionType,
                        startTime: startTime,
                        processingStart: startTime,
                        processingEnd: endTime,
                        duration: duration,
                        target: element.tagName
                    }});
                }});
            }});
            
            // Trigger the event
            element.dispatchEvent(event);
        }});
        """
        
        return self.driver.execute_async_script(script, element, interaction_type)
        """Get the current INP value (75th percentile of all interactions)"""
        script = """
        // Calculate INP as 75th percentile of interaction latencies (in microseconds)
        if (window.inpValues && window.inpValues.length > 0) {
            const durations = window.inpValues.map(inp => inp.duration).sort((a, b) => a - b);
            const index = Math.ceil(durations.length * 0.75) - 1;
            return {
                inp: durations[index], // Already in microseconds
                totalInteractions: durations.length,
                allDurations: durations
            };
        }
        return null;
        """
        return self.driver.execute_script(script)


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
    primitive_string = ''
    if 'SELECTED_PRIMITIVE' in params and params['SELECTED_PRIMITIVE'] != '':
        primitive_string = '?SELECTED_PRIMITIVE=' + requests.utils.quote(params['SELECTED_PRIMITIVE'])
    return params['baseUrl'] + '/static/interface.html' + primitive_string + '#' + params['dataset']


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

class HoverTargetVisible():
    def __init__(self):
        pass

    def __call__(self, web_driver):
        hoverTarget = driver.find_elements(By.XPATH, "//*[@class='hoverTarget']")        
        for each_hover in hoverTarget:
            parent_element = each_hover.find_element(By.XPATH, "..")
            if parent_element.get_attribute("class") == 'rightHandle':
                return each_hover
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

def compare_images(imageA, imageB):
    # Load the two PNG images (convert to grayscale for SSIM)
    image1 = imread(imageA, as_gray=True)
    image2 = imread(imageB, as_gray=True)

    # Compute SSIM
    score, diff = ssim(image1, image2, full=True, data_range=1.0)
    return score

def getPNGFileName(params, qt, iteration, isRaw=False):
    primitive_string = ''
    if 'SELECTED_PRIMITIVE' in params and params['SELECTED_PRIMITIVE'] != '':
        primitive_string = '_' + params['SELECTED_PRIMITIVE']
        for char in '\/:*?"<>|':
            primitive_string = primitive_string.replace(char, '_')

    ds_name = params['prfiledDS']
    if isRaw:
        ds_name = "db_duck_raw"
    hrds = ''
    if params['hrd'] != '1':
        hrds = params['hrd'] + "_"
    exported_file_name = params['exportLocation'] + "/" + hrds + params['dataset'] + "/figures/" \
                        + str(iteration) \
                        + "_" + params['dataset'] \
                        + "_" + qt \
                        + "_" + ds_name \
                        + "_" + params['hrd'] \
                        + primitive_string + "_gantt.png"
    return exported_file_name

def calcSSIM(params, qt, iteration):
    exported_file_name_1 = getPNGFileName(params, qt, iteration, False)
    exported_file_name_2 = getPNGFileName(params, qt, iteration, True)
    if not os.path.exists(exported_file_name_1) or not os.path.exists(exported_file_name_2):
        print("PNG files do not exist for SSIM calculation:", exported_file_name_1, exported_file_name_2)
        return -1.0
    return compare_images(exported_file_name_1, exported_file_name_2)

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
    WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(HoverTargetVisible())
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
        c_begin = int(handleWindow * (84 / 100)) + leftHandleLocation
        c_end = int(handleWindow * (85 / 100)) + leftHandleLocation
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
            # brush_percentage = int(abs(previous_handle - current_handle) / handleWindow * 100.0)
            # print(str(iteration_count), str(brush_percentage), str(endTimer - startTimer), sep=",")
            # print('Iteration', '{:2d}'.format(iteration_count), end=' ')
            # print('Brush percentage', '{:3d}%'.format(brush_percentage), end=' ')
            # print('Total drawing time:', '{:5d}'.format(endTimer - startTimer), 'ms')
            iteration_count = iteration_count + 1
        previous_handle = c_end

def conductRandomClicking(driver, timeout, p_freq, TOTAL_SAMPLE, base_params, qt):
    scrollToWindow(driver, timeout, p_freq)
    # zoom out in the gantt y axis to reveal all locations
    # ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    # wheel_element(ganttYScroller, -150)

    ganttEventCapturerElement = driver.find_elements(By.XPATH, "//*[@class='eventCapturer']")[0]
    action = ActionChains(driver)
    random.seed(10)
    click_offset = 5
    x_bounds = [0, int(ganttEventCapturerElement.rect['y'])-click_offset]
    y_bounds = [-int(ganttEventCapturerElement.rect['x'])+click_offset, int(ganttEventCapturerElement.rect['x'])-click_offset]
    i = 1

    print("iteration,x,y,total drawing time (micros)")
    while(True):
        if i > TOTAL_SAMPLE:
            break
        tx = random.randint(x_bounds[0], x_bounds[1])
        ty = random.randint(y_bounds[0], y_bounds[1])
        ntx = tx
        nty = ty
        for ctr in range(8):
            action.move_to_element_with_offset(ganttEventCapturerElement, ntx, nty).click().perform()
            startTimer = round(time.time() * 1000000)
            WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
            endTimer = round(time.time() * 1000000)
            time.sleep(2)
            if(checkClickValidity(driver, timeout, p_freq, ntx, nty)):
                print(str(i), str(ntx), str(nty), str(endTimer - startTimer), sep=",")
                if i == 1:
                    conductPNGOutput(driver, base_params, qt)
                i = i + 1
                break
            else:
                # print("not found", ntx, nty, i)
                rtx, rty = getNextCircularPoint(ntx - tx, nty - ty)
                ntx = tx + rtx
                nty = ty + rty
                if x_bounds[0] <= ntx <= x_bounds[1] and y_bounds[0] <= nty <= y_bounds[1]:
                    break
                    

    # print("successfully run the profiling on", driver.title)

def increaseGanttWindow(driver, timeout, p_freq):
    utilView = driver.find_elements(By.XPATH, "//*[@class='GLView UtilizationView']")
    height = 100
    if utilView:
        height = utilView[0].size['height']
    else:
        return
    height = 2 / 3 * height * (-1)
    dragTarget = driver.find_elements(By.XPATH, "//*[@class='lm_splitter lm_vertical']")
    for drag_target in dragTarget:
        action = ActionChains(driver)
        action.click_and_hold(drag_target)\
              .move_by_offset(0, height)\
              .release()\
              .perform()
        time.sleep(5)
        WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())

def conductBrushing(driver, timeout, p_freq, TOTAL_SAMPLE, qt):
    time.sleep(3)
    increaseGanttWindow(driver, timeout, p_freq)
    
    # zoom out in the gantt y axis to reveal all locations
    ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
    wheel_element(ganttYScroller, -150)
    time.sleep(5)
    WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())

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

    inp_measurer = INPMeasurer(driver)
    inp_measurer.setup_inp_observer()

    print("iteration,brush percentage,total drawing time (micros),inp (micros),ssim")
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

            # Perform brushing
            def click_navigation():
                try:
                    ActionChains(driver).drag_and_drop_by_offset(each_hover, drag_offset, 0).perform()
                except:
                    pass
            inp_measurer.measure_interaction_inp(click_navigation, "Navigation Click")

            startTimer = round(time.time() * 1000000)
            WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
            endTimer = round(time.time() * 1000000)
            brush_percentage = int(abs(previous_handle - current_handle) / handleWindow * 100.0)

            all_inp_values = inp_measurer.get_inp_values()
            inp_results = ''
            for inp in all_inp_values:
                if inp['name'] == 'pointerdown':
                    inp_results = inp['duration']

            conductPNGOutput(driver, base_params, qt, iteration_count)
            ssim = calcSSIM(base_params, qt, iteration_count)
            print(str(iteration_count), str(brush_percentage), str(endTimer - startTimer), str(int(inp_results)), f"{ssim:.3f}", sep=",")
            time.sleep(1)
            # print('Iteration', '{:2d}'.format(iteration_count), end=' ')
            # print('Brush percentage', '{:3d}%'.format(brush_percentage), end=' ')
            # print('Total drawing time:', '{:5d}'.format(endTimer - startTimer), 'ms')
            iteration_count = iteration_count + 1
        previous_handle = c_end

    # print("successfully run the profiling on", driver.title)

def conductPNGOutput(driver, params, qt, iteration=1):
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
    
    # zoom out in the gantt y axis to reveal all locations
    if qt != 'attribute':
        time.sleep(3)
        ganttYScroller = driver.find_element(By.CLASS_NAME, "yAxisScrollCapturer")
        wheel_element(ganttYScroller, -150)

    element = WebDriverWait(driver, timeout=timeout, poll_frequency=p_freq).until(GanttLoadingVisible())
    time.sleep(3)
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
    exported_file_name = getPNGFileName(params, qt, iteration, False)
    ganttEventCapturerElement.screenshot(exported_file_name)
    # print("Saved screenshot from of the Gantt chart at: ", exported_file_name)

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
    options.add_argument("--window-size=3840,2160")
    options.add_argument("--start-maximized") # open Browser in maximized mode
    #options.add_argument("disable-infobars") # disabling infobars
    # options.add_argument("--disable-extensions") # disabling extensions
    # options.add_argument("--disable-gpu") # applicable to windows os only
    # options.add_argument("--disable-dev-shm-usage") # overcome limited resource problems
    options.add_argument("--no-sandbox") # Bypass OS security model
    # options.add_argument("--remote-debugging-port=9222")
    #options.add_argument("--incognito")
    options.add_argument('--disable-blink-features=AutomationControlled')
    options.add_argument('--enable-logging')
    options.add_argument('--log-level=0')

    DGEM_ID = 'ecc21d0a-112a-4b52-8cdd-6aca80adde93'#'589ca754-ef75-426c-8d51-841cc61dc84a'
    KMEANS_ID = 'faf17535-2f66-4621-995f-49c7dbd84e8b'
    LULESH_ID = '772c7330-d4eb-485b-866a-3b315063f9af'

    base_params = {
        'dataset': os.getenv('DATASET_ID', DGEM_ID),
        'baseUrl': os.getenv('BASE_URL', "http://localhost:8000"),
        'prfiledDS': os.getenv('PROFILED_DS', "summed_area_table"),
        'hrd': os.getenv('HORIZONTAL_RESOLUTION_DIVISOR', "1"),
        'exportLocation': "."
    }
    TOTAL_SAMPLE = int(os.getenv('TOTAL_SAMPLE', 10))
    QUERY_TYPE = os.getenv('QUERY_TYPE', 'window')
    if len(sys.argv) > 1:
        base_params['exportLocation'] = sys.argv[1]
    if len(sys.argv) > 2:
        base_params['exportStartTime'] = sys.argv[2]
    if len(sys.argv) > 3:
        base_params['exportEndTime'] = int(sys.argv[3])
    base_params['SELECTED_PRIMITIVE'] = os.getenv('SELECTED_PRIMITIVE', '')

    timeout = 500  # in seconds
    p_freq = 0.001  # in seconds

    # Define paths
    user_home_dir = os.path.expanduser("/home/sci/sayefsakin/installed_programs/selenium_driver")
    # user_home_dir = os.path.expanduser("/home/sayefsakin/selenium_testing")
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

        # canvas_elem = driver.find_elements(By.XPATH, "//canvas")
        # for parent_element in canvas_elem:
        #     print(parent_element.rect)

        # conductFixedScrolling(driver, timeout, p_freq, TOTAL_SAMPLE)
        # time.sleep(3000)
        if QUERY_TYPE == 'window' or QUERY_TYPE == 'cond':
            conductBrushing(driver, timeout, p_freq, TOTAL_SAMPLE, QUERY_TYPE)
        elif QUERY_TYPE == 'attribute':
            conductRandomClicking(driver, timeout, p_freq, TOTAL_SAMPLE, base_params, QUERY_TYPE)
        # elif QUERY_TYPE == 'cond':
        #     time.sleep(60)
        # print('Initial Gantt Loading Time: ', '{:9d}'.format(endTimer - startTimer), 'micros ')
        # for entry in driver.get_log('performance'):
        #     print(entry)
