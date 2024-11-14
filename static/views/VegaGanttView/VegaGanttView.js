/* globals uki */
import LinkedMixin from '../common/LinkedMixin.js';
// import * as duckdb from '@duckdb/duckdb-wasm';
// import * as vega from "../../node_modules/vega/build/vega.js";
// import * as vp from "../../node_modules/vega-parser/build/vega-parser.js";

class VegaGanttView extends LinkedMixin(uki.ui.GLView) {
  constructor (options) {
    options.resources = options.resources || [];
    options.resources.push(...[
      { type: 'text', url: 'views/VegaGanttView/template.html', name: 'template' },
      { type: 'less', url: 'views/VegaGanttView/style.less' }
    ]);
    super(options);
  }

  async setup () {
    await super.setup(...arguments);

    this.d3el.html(this.getNamedResource('template'))
      .classed('VegaGanttView', true);

  }

  async draw () {
    await super.draw(...arguments);

    if (this.isLoading) {
      // Don't draw anything if we're still waiting on something; super.draw
      // will show a spinner. Instead, ensure that another render() call is
      // fired when we're finally ready
      this.ready.then(() => { this.render(); });
      return;
    } else if (this.error) {
      // If there's an upstream error, super.draw will already display an error
      // message. Don't attempt to draw anything (or we'll probably just add to
      // the noise of whatever is really wrong)
      return;
    }

    
    this.drawVegaGraph();

    
    
  }

  async drawVegaGraph() {
    var selectionHeader;
    var specs =  {"$schema": "https://vega.github.io/schema/vega-lite/v5.json",
      "width": 500,
      "data": {
        "values": [
          {
            "name": "Project 1",
            "start": "2023-03-01",
            "end": "2023-03-15",
            "status": "On track",
            "description": "This is the description of project 1."
          },
          {
            "name": "Project 2",
            "start": "2023-03-10",
            "end": "2023-04-15",
            "status": "Delayed",
            "description": "This is the description of project 2."
          },
          {
            "name": "Project 3",
            "start": "2023-04-01",
            "end": "2023-05-15",
            "status": "Behind schedule",
            "description": "This is the description of project 3."
          }
        ]
      },
      "transform": [
        {"calculate": "toDate(utcFormat(now(), '%Y-%m-%d'))", "as": "currentDate"}
      ],
      "title": {
        "text": "Gantt Chart with Rule Line for Today's Date",
        "fontSize": 14,
        "anchor": "start",
        "dy": -15,
        "color": "#706D6C"
      },
      "layer": [
        {
          "mark": {"type": "bar", "tooltip": true},
          "encoding": {
            "y": {
              "field": "name",
              "type": "nominal",
              "axis": {
                "domain": true,
                "grid": false,
                "ticks": false,
                "labels": true,
                "labelFontSize": 11,
                "labelPadding": 6
              },
              "title": null
            },
            "x": {
              "field": "start",
              "type": "temporal",
              "timeUnit": "yearmonthdate",
              "axis": {
                "format": "%d-%b",
                "domain": true,
                "grid": false,
                "ticks": true,
                "labels": true,
                "labelFontSize": 11,
                "labelPadding": 6
              },
              "title": null
            },
            "x2": {"field": "end"},
            "color": {
              "title": null,
              "field": "status",
              "type": "nominal",
              "legend": {
                "padding": 0,
                "labelFontSize": 11,
                "labelColor": "#706D6C",
                "rowPadding": 8,
                "symbolOpacity": 0.9,
                "symbolType": "square"
              }
            }
          }
        },
        {
          "mark": {"type": "rule", "strokeDash": [2, 2], "strokeWidth": 2},
          "encoding": {
            "x": {
              "field": "currentDate",
              "type": "temporal",
              "axis": {"format": "%Y-%m-%d"}
            }
          }
        }
      ],
      "config": {"view": {"stroke": null}}
    };

    vegaEmbed(
      '#selectionHeader',
      specs
    );

    // fetch('https://vega.github.io/vega/examples/bar-chart.vg.json')
    //   .then(res => res.json())
    //   .then(spec => render(spec))
    //   .catch(err => console.error(err));

    // function render(spec) {
    //   selectionHeader = new vega.View(vp.parse(spec), {
    //     renderer:  'canvas',  // renderer (canvas or svg)
    //     container: '#view',   // parent DOM container
    //     hover:     true       // enable hover processing
    //   });
    //   return selectionHeader.runAsync();
    // }

    // this.d3el.select('.selectionHeader').node().appendChild(view);
  }
}
export default VegaGanttView;
