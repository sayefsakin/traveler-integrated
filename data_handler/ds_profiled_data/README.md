# Running Traveler on selenium headless browser

Use the [headless_test.py](./headless_test.py) file to run Traveler on a headless selenium Chrome browser. For this to work, make sure that selenium webdriver and chrome driver is installed. Update **user_home_dir** to the location of the chrome driver path. Tune up (comment/uncomment) options for the webdriver in the main function.

The following parameters can be modified by setting the environment variables.

- DATASET_ID : Dataset UUID.
- BASE_URL : The base url (host and port) where Traveler is running (serving data over REST API).
- PROFILED_DS : Check [experiment_vars.sh](./experiment_vars.sh) for possible values.
- PIXEL_WINDOW : Number of consecutive pixels to apply the summarization.
- SELECTED_PRIMITIVE : Primitive name string to highlight specific primitive while drawing the Gantt chart.
- TOTAL_SAMPLE : This controls the number of iterations for the experiment.

The following parameters can be provided as the command line arguments.

- 1 : PNG export location. For each experiment, Setting this parameter will generate a screenshot and export that as PNG file in this location.
- 2 : Start time of the brush in the utilization view.
- 3 : End time of the brush in the utilization view.

# Running Traveler along with the ESeMan server

The [ds_profiler.sh](./ds_profiler.sh) contains scripts to automate running the Traveler for experimentation. This will also help to run the ESeMan server and Traveler server simultaneously to serve data from the LMDB database.

The following parameters can be provided as the command line arguments.

- 1 : Command type (see below)
- 2 : Dataset ID
- 3 : Profiled Dataset

Check [experiment_vars.sh](./experiment_vars.sh) for possible Dataset ID and Profiled Dataset values.

### Possible Command Types

- window : Run the range query
- attribute : Run the Get attribute detail query
- cond : Run the range query with conditional value (like primitive)
- kill : Kill all the running processess that this script started.
- interactive : Start the server in the background and keep the interactive terminal open.
- make_json : Convert OTF2 file to JSON. Start the server first in interactive mode and then execute this command.

For example, execute the following command from terminal to start Traveler and ESeMan server at the same time.

```
./ds_profiler.sh interactive f4e2fdfa-893e-4f13-bac8-e9fbbdf40c1f eseman_kdt
```