#include "gridmap.hpp"
#include "domapping.hpp"

HMCallback::FunctionWithCallback<HMMap::TMapGrid> HMMap::MapGrid;

HM2D::GridData HMMap::TMapGrid::_run(const HM2D::GridData& base,
		const HM2D::EdgeData& area,
		vector<Point> base_points,
		vector<Point> mapped_points,
		bool reversed,
		Options opt){
	//1) scale
	callback->step_after(5, "Initializing");
	//2) set input data
	shared_ptr<HMMap::Impl::DoMapping> dm;
	if (opt.algo == "inverse-laplace"){
		dm.reset(new HMMap::Impl::InverseMapping(opt, reversed));
	} else if (opt.algo == "direct-laplace"){
		dm.reset(new HMMap::Impl::DirectMapping(opt, reversed));
	} else {
		assert(false);
	}
	dm->set_grid(base);
	dm->set_contour(area);
	dm->set_points(base_points, mapped_points);
	//3) build
	HM2D::GridData ret = dm->run(*callback);
	//4) unscale and return
	callback->step_after(5, "Finilizing");
	return ret;
}
