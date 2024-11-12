/* globals uki */
import LinkedMixin from '../common/LinkedMixin.js';
import * as vg from "../../node_modules/@uwdata/vgplot/dist/vgplot.js";
import * as mc from '../../node_modules/@uwdata/mosaic-core/dist/mosaic-core.js';
// import * as duckdb from '@duckdb/duckdb-wasm';

import { Query, and, count, isBetween } from "../../node_modules/@uwdata/mosaic-sql/dist/mosaic-sql.js";

class MosaicGanttView extends LinkedMixin(uki.ui.GLView) {
  constructor (options) {
    options.resources = options.resources || [];
    options.resources.push(...[
      { type: 'text', url: 'views/MosaicGanttView/template.html', name: 'template' },
      { type: 'less', url: 'views/MosaicGanttView/style.less' }
    ]);
    super(options);
  }

  async setup () {
    await super.setup(...arguments);

    this.d3el.html(this.getNamedResource('template'))
      .classed('MosaicGanttView', true);

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

    

    
    // // Trying to add sampling
    // const gantt_plot = vg.plot(
    //   vg.barX(
    //     vg.from("intervals", {sample: 50}),
    //     {x1: "enter_t", x2: "leave_t", y: "loc", fill: "yellow", clip: true, fillOpacity: 0.2}
    //   ),
    //   vg.barX(
    //     vg.from("intervals"),
    //     {x1: "enter_t", x2: "leave_t", y: "loc", fill: "gray", clip: true}
    //   ),
    //   vg.yScale("band"),
    //   vg.toggleY({as: $click}),
    //   vg.toggleX({as: $click}),
    //   // vg.highlight({by: $click}),
    //   vg.xDomain(vg.Fixed),
    //   vg.yDomain(vg.Fixed),
    //   // vg.axisY({tickPadding: 5, labelAnchor: "bottom"}),
    //   vg.panZoom({y:$xs}),
    //   vg.highlight({by:$click}),
    //   vg.gridY(),
    //   vg.width(chartShape.chartWidth),
    //   vg.height(chartShape.chartHeight)
    // );

    
    // this.drawGanttSketch();
    this.drawMosaicGraph();
    // this.drawPlotlyGraph();

    
    
  }

  async drawGanttSketch() {
    // cnt = mc.restConnector();
    vg.coordinator().databaseConnector(mc.restConnector());
    

//     const loc_text = "(CAST((CAST(Location AS BIGINT) >> 32) AS VARCHAR) || ' - T' || " +
// "CAST(CAST(CAST(CAST(Location AS BIGINT) AS BIT) & CAST(CAST(4294967295 AS BIGINT) AS BIT) AS BIGINT) AS VARCHAR)) " +
// "AS loc ";
    const loc_text = "CAST(Location AS BIGINT) AS loc";
    const json_file = `data/traveler_data/${this.datasetId}.json`;
    await vg.coordinator().exec([
      vg.loadJSON("intervals", json_file, 
        {
          format: 'array', maximum_depth:-1,
          select: ["enter.Timestamp AS enter_t", "leave.Timestamp AS leave_t", loc_text],
          // sample: [1000, 10]// "reservoir"]//(1000 ROWS)// REPEATABLE (100), this doesnt work
          // where: isBetween("enter_t", [290000000, 360000000])
        })

    ]);

    const $nrows = 840000;
    await vg.coordinator().exec([
      `CREATE TEMP TABLE IF NOT EXISTS sampled_intervals AS SELECT * from intervals`
      + ` WHERE loc BETWEEN 1 AND 40 `
      + ` USING SAMPLE reservoir(` + $nrows + ` ROWS) REPEATABLE(100)`
    ]);
    
    const $xs = vg.Selection.intersect();
    const $ys = vg.Selection.intersect();

    const bounds = this.getBounds();
    this.margin = {
      top: 0,
      right: 0,
      bottom: 0,
      left: 0
    };
    const $click = vg.Selection.single();
    const chartShape = {
      chartWidth: bounds.width - this.margin.left - this.margin.right,
      chartHeight: bounds.height - this.margin.top - this.margin.bottom
    };

    // Naive implementation using barX, this is very slow
    const gantt_plot = vg.plot(
      // vg.barX(
      //   vg.from("intervals", {optimize: true}),
      //   {x1: "enter_t", x2: "leave_t", y: "loc", fill: "yellow", clip: true, fillOpacity: 0.2}
      // ),
      vg.barX(
        vg.from("sampled_intervals"),
        {x1: "enter_t", x2: "leave_t", y: "loc", fill: "gray", clip: true}
      ),
      vg.yScale("band"),
      vg.toggleY({as: $click}),
      vg.toggleX({as: $click}),
      // vg.highlight({by: $click}),
      vg.xDomain(vg.Fixed),
      vg.yDomain(vg.Fixed),
      // vg.axisY({tickPadding: 5, labelAnchor: "bottom"}),
      vg.panZoom({y:$xs}),
      vg.highlight({by:$click}),
      vg.gridY(),
      vg.width(chartShape.chartWidth),
      vg.height(chartShape.chartHeight)
    );

    this.d3el.select('.selectionHeader').node().appendChild(gantt_plot);
  }

  async drawMosaicGraph() {
    // cnt = mc.restConnector();
    vg.coordinator().databaseConnector(mc.restConnector());
    

//     const loc_text = "(CAST((CAST(Location AS BIGINT) >> 32) AS VARCHAR) || ' - T' || " +
// "CAST(CAST(CAST(CAST(Location AS BIGINT) AS BIT) & CAST(CAST(4294967295 AS BIGINT) AS BIT) AS BIGINT) AS VARCHAR)) " +
// "AS loc ";
    const loc_text = "CAST(Location AS BIGINT) AS loc";
    const json_file = `data/traveler_data/${this.datasetId}.json`;
    await vg.coordinator().exec([
      vg.loadJSON("intervals", json_file, 
        {
          format: 'array', maximum_depth:-1,
          select: ["enter.Timestamp AS enter_t", "leave.Timestamp AS leave_t", loc_text],
          // sample: [1000, 10]// "reservoir"]//(1000 ROWS)// REPEATABLE (100), this doesnt work
          // where: isBetween("enter_t", [290000000, 360000000])
        })
    ]);

    const $xs = vg.Selection.intersect();
    const $ys = vg.Selection.intersect();

    const bounds = this.getBounds();
    this.margin = {
      top: 0,
      right: 0,
      bottom: 0,
      left: 0
    };
    const $click = vg.Selection.single();
    const chartShape = {
      chartWidth: bounds.width - this.margin.left - this.margin.right,
      chartHeight: bounds.height - this.margin.top - this.margin.bottom
    };

    // Naive implementation using barX, this is very slow
    const gantt_plot = vg.plot(
      vg.barX(
        vg.from("intervals", {optimize: true}),
        {x1: "enter_t", x2: "leave_t", y: "loc", fill: "yellow", clip: true, fillOpacity: 0.2}
      ),
      vg.barX(
        vg.from("intervals"),
        {x1: "enter_t", x2: "leave_t", y: "loc", fill: "gray", clip: true}
      ),
      vg.yScale("band"),
      vg.toggleY({as: $click}),
      vg.toggleX({as: $click}),
      // vg.highlight({by: $click}),
      vg.xDomain(vg.Fixed),
      vg.yDomain(vg.Fixed),
      // vg.axisY({tickPadding: 5, labelAnchor: "bottom"}),
      vg.panZoom({y:$xs}),
      vg.highlight({by:$click}),
      vg.gridY(),
      vg.width(chartShape.chartWidth),
      vg.height(chartShape.chartHeight)
    );

    // // Using Heatmap, this impelementeation is very fast and uses M4 Optimization
    // const gantt_plot = vg.plot(
    //   vg.heatmap(
    //     vg.from("intervals"),
    //     {x: "enter_t", y: "loc", fill: "density", bandwidth: 3}
    //   ),
    //   vg.colorScale("symlog"),
    //   vg.colorScheme("inferno"),
    //   // vg.xAxis("top"),
    //   vg.xLabelAnchor("center"),
    //   // vg.xZero(true),
    //   vg.yLabelAnchor("center"),
    //   vg.panZoom({y:$xs}),
    //   // vg.marginTop(30),
    //   // vg.marginLeft(5),
    //   // vg.marginRight(40),
    //   vg.width(chartShape.chartWidth),
    //   vg.height(chartShape.chartHeight)
    // );

    this.d3el.select('.selectionHeader').node().appendChild(gantt_plot);
  }

  async drawPlotlyGraph() {

    // // Initialize DuckDB-Wasm
    // const worker = await duckdb.createWorker();
    // const db = new duckdb.DuckDB(worker);

    // // Connect to an in-memory database
    // await db.connect(":memory:");

    // // Create a table
    // await db.query("CREATE TABLE IF NOT EXISTS people (id INTEGER, name VARCHAR)");

    // // Insert some data
    // await db.query("INSERT INTO people VALUES (1, 'Alice'), (2, 'Bob')");

    // // Query the data
    // const result = await db.query("SELECT * FROM people");
    // console.log(result.toArray());

    // // Close the connection
    // await db.disconnect();


    var data = [{
      type: 'bar',
      y: [0,0,0,1,1,1], //locaiton
      x: [1,2,1,2,1,2], // bar length
      orientation: 'h',
      base: [0,2,5,0,3,5] // bar starting point
    }];
    
    Plotly.newPlot(this.d3el.select('.selectionHeader').node(), data);
  }
}
export default MosaicGanttView;
