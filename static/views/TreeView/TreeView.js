/* globals d3 */
import GoldenLayoutView from '../common/GoldenLayoutView.js';
import SingleDatasetMixin from '../common/SingleDatasetMixin.js';
import SvgViewMixin from '../common/SvgViewMixin.js';

class TreeView extends SvgViewMixin(SingleDatasetMixin(GoldenLayoutView)) {
  constructor (argObj) {
    argObj.resources = [
      { type: 'less', url: 'views/TreeView/style.less' },
      { type: 'text', url: 'views/TreeView/shapeKey.html' }
    ];
    super(argObj);

    (async () => {
      try {
        this.tree = await d3.json(`/datasets/${encodeURIComponent(argObj.state.label)}/tree`);
      } catch (err) {
        this.tree = err;
      }
      this.render();
    })();
  }
  setup () {
    super.setup();
    this.shapeKey = this.d3el.append('div')
      .classed('shapeKey', true)
      .html(this.resources[1]);
    this.legend = this.d3el.append('div')
      .attr('id', 'legend');
  }
  draw () {
    super.draw();

    if (this.tree === undefined || this.regions === undefined) {
      return;
    } else if (this.tree instanceof Error || this.regions instanceof Error) {
      this.emptyStateDiv.html('<p>Error communicating with the server</p>');
    }
    console.log(this.tree);
  }
}
export default TreeView;
