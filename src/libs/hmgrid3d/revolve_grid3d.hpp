#ifndef REVOLVE_GRID3D_HPP
#define REVOLVE_GRID3D_HPP

#include "primitives2d.hpp"
#include "primitives3d.hpp"

namespace HM3D{namespace Constructor{

//returns final grid
//phi_coords in degrees
HM3D::GridData RevolveGrid2D(const HM2D::GridData& g2d,
		const vector<double>& phi_coords,
		Point pstart, Point pend, bool is_trian=true,
		std::function<int(int)> side_bt = [](int){ return 1; },
		std::function<int(int)> bt1 = [](int){ return 2; },
		std::function<int(int)> bt2 = [](int){ return 3; });


}}
#endif
