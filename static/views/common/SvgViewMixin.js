const SvgViewMixin = function (superclass) {
  const SvgView = class extends superclass {
    constructor () {
      super(...arguments);
      this._instanceOfSvgViewMixin = true;
    }
    setupContentElement () {
      return this.d3el.append('svg');
    }
    getContentBounds (content = this.content) {
      const bounds = content.node().parentNode.getBoundingClientRect();
      content.attr('width', bounds.width)
        .attr('height', bounds.height);
      return bounds;
    }
  };
  return SvgView;
};
Object.defineProperty(SvgViewMixin, Symbol.hasInstance, {
  value: i => !!i._instanceOfSvgViewMixin
});
export default SvgViewMixin;
