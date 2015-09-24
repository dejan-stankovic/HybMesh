#ifndef HYBMESH_HMMATH_SCPACK_PORT_HPP
#define HYBMESH_HMMATH_SCPACK_PORT_HPP
#include "hmconformal.hpp"

namespace HMMath{ namespace Conformal{ namespace Impl{
namespace ScPack{

struct Pt{
	double x, y;
};

class ToRect: public HMMath::Conformal::Rect{
	//input data
	vector<Pt> wcoords;
	Pt w0;
	std::array<int, 4> corners;

	//internal
	vector<double> betam;
	Pt factor, factor2;
	vector<Pt> zcoords, zcoords2;
	vector<double> qwork, qwork2;
	vector<Pt> wcoords2;
	Pt w02;

	//opt
	const int N;
	const int prec;

	//conformal module
	double _module;

	ToRect(const vector<Pt>& pnt, Pt p0, std::array<int, 4> cor, int prec);
public:
	//assemble from path given by ordered set of points
	//in an anti-clockwise direction
	//corners are (0, i1, i2, i3)
	static shared_ptr<ToRect>
	Build(const vector<Point>& pnt, int i1, int i2, int i3);

	//assemble from 4 open contours directed in such a way that
	//area is bounded in an anti-clockwise direction by
	//  reverse(left), bottom, right, reverse(top)
	static shared_ptr<ToRect>
	Build(const HMCont2D::Contour& left, const HMCont2D::Contour& right,
		const HMCont2D::Contour& bot, const HMCont2D::Contour& top);

	double module() const override { return _module; }

	//Points from rectangle to polygon
	vector<Point> MapToPolygon(const vector<Point>& input) const override;

	//Points from polygon to rectangle
	vector<Point> MapToRectangle(const vector<Point>& input) const override;

	//Mapped Rectangle
	vector<Point> RectPoints() const override;
};


}
}}}
#endif
