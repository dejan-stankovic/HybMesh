#include "canonic_bgrid.hpp"
#include "hmtimer.hpp"
#include "procgrid.h"
#include "constructor.hpp"
#include "algos.hpp"
#include "treverter2d.hpp"
#include "cont_assembler.hpp"
#include "treverter2d.hpp"

#define USE_ANALYTICAL_MAPPINGS false

using namespace HMBlay::Impl;

namespace{
bool ffequal(const HM2D::EdgeData& c1, const HM2D::EdgeData& c2){
	return *HM2D::Contour::First(c1) == *HM2D::Contour::First(c2);
}
bool flequal(const HM2D::EdgeData& c1, const HM2D::EdgeData& c2){
	return *HM2D::Contour::First(c1) == *HM2D::Contour::Last(c2);
}
bool llequal(const HM2D::EdgeData& c1, const HM2D::EdgeData& c2){
	return *HM2D::Contour::Last(c1) == *HM2D::Contour::Last(c2);
}
}

shared_ptr<MappedRect>
MappedRect::Factory(HM2D::EdgeData& left, HM2D::EdgeData& right,
		HM2D::EdgeData& bottom, HM2D::EdgeData& top,
		int femn, bool use_rect_approx){
	assert(ffequal(left, bottom));
	assert(flequal(top, left));
	assert(flequal(right, bottom));
	assert(llequal(right, top));

	//if left/right coincide we need mapping to a ring
	auto leftcrn = HM2D::Contour::CornerPoints(left);
	auto rightcrn = HM2D::Contour::CornerPoints(right);
	if (leftcrn.size() == rightcrn.size()){
		if (std::equal(leftcrn.begin(), leftcrn.end(), rightcrn.begin(),
				[](shared_ptr<HM2D::Vertex> p1, shared_ptr<HM2D::Vertex> p2){ return *p1 == *p2; })){
			return RectForClosedArea::Build(bottom, top);
		}
	}
	//left/right do not coincide. We need mapping to rectangle.
	return std::make_shared<RectForOpenArea>(
		left, right, bottom, top, femn, use_rect_approx
	);
};

//takes two closed contours (c1, c2) and builds
//averaged one which goes from p1 (in c1) to p2 (in c2)
HM2D::EdgeData
HMBlay::Impl::ContoursWeight(const HM2D::EdgeData& c1, Point p1,
		const HM2D::EdgeData& c2, Point p2){
	assert(HM2D::Contour::IsClosed(c1) && HM2D::Contour::IsClosed(c2));
	//This check should be revised due to errors in test13
	//assert(ISZERO(Point::dist(p1, c1.ClosestPoint(p1))) &&
	       //ISZERO(Point::dist(p2, c2.ClosestPoint(p2))));
	assert(HM2D::Contour::Area(c1)*HM2D::Contour::Area(c2) > 0);
	//place points p1, p2 to both contours
	auto line1 = HM2D::Contour::Constructor::CutContour(c1, p1, p2);
	auto line2 = HM2D::Contour::Constructor::CutContour(c2, p1, p2);
	//assemble points->weights map
	std::map<double, Point*> line1map, line2map;
	auto ew1 = HM2D::Contour::EWeights(line1);
	auto op1 = HM2D::Contour::OrderedPoints(line1);
	for (int i=0; i<ew1.size(); ++i) line1map[ew1[i]] = op1[i].get();
	auto ew2 = HM2D::Contour::EWeights(line2);
	auto op2 = HM2D::Contour::OrderedPoints(line2);
	for (int i=0; i<ew2.size(); ++i) line2map[ew2[i]] = op2[i].get();
	
	//concatenate maps
	std::map<double, std::pair<Point, Point>> fullmap;
	for (auto& it: line1map){
		if (fullmap.find(it.first) != fullmap.end()) continue;
		std::pair<Point, Point> dt;
		dt.first = *it.second;
		dt.second = HM2D::Contour::WeightPoint(line2, it.first);
		fullmap[it.first] = dt;
	}
	for (auto& it: line2map){
		if (fullmap.find(it.first) != fullmap.end()) continue;
		std::pair<Point, Point> dt;
		dt.second = *it.second;
		dt.first = HM2D::Contour::WeightPoint(line1, it.first);
		fullmap[it.first] = dt;
	}
	//remove entries which lie to close to each other
	std::map<double, std::pair<Point, Point>> fullmap1;
	fullmap1.insert(*fullmap.begin());
	fullmap1.insert(*fullmap.rbegin());
	double lastw = 0;
	for (auto& it: fullmap){
		if (fabs(it.first-lastw)<0.01 || fabs(it.first-1.0)<0.01) continue;
		fullmap1.insert(it);
		lastw = it.first;
	}
	//build points and return
	std::vector<Point> ret;
	for (auto& it: fullmap1){
		ret.push_back(Point::Weigh(it.second.first,
					it.second.second, it.first));
	}
	return HM2D::Contour::Constructor::FromPoints(ret);
}

shared_ptr<MappedRect>
MappedRect::Factory(HM2D::EdgeData& left, HM2D::EdgeData& right,
		HM2D::EdgeData& bottom, double h,
		int femn, bool use_rect_approx){
	assert(ffequal(left, bottom));
	assert(flequal(right, bottom));

	//1) if bottom is a closed line draw outer contour at h from bottom
	if (flequal(bottom, bottom)){
		return RectForClosedArea::Build(bottom, HM2D::Contour::First(bottom).get(), h);
	}

	//2) cut left and right to size h since we need it
	vector<Point> cutp0 = HM2D::Contour::WeightPointsByLen(left, {0, h});
	vector<Point> cutp1 = HM2D::Contour::WeightPointsByLen(right, {0, h});
	auto left2 = HM2D::Contour::Constructor::CutContour(left, cutp0[0], cutp0[1]);
	auto right2 = HM2D::Contour::Constructor::CutContour(right, cutp1[0], cutp1[1]);
	bool straight_left = HM2D::Contour::CornerPoints(left2).size() == 2;
	bool straight_right = HM2D::Contour::CornerPoints(right2).size() == 2;
	bool straight_bottom = HM2D::Contour::CornerPoints(bottom).size() == 2;

	//3) if all lines are straight and are at angles (pi/2 +/- pi/16)
	//   -> simply connect end points
	auto op_left = HM2D::Contour::OrderedPoints(left2);
	auto op_right = HM2D::Contour::OrderedPoints(right2);
	auto op_bottom = HM2D::Contour::OrderedPoints(bottom);
	double an1 = Angle(*op_left[1], *op_left[0], *op_bottom[1]);
	double an2 = Angle(*op_bottom[op_bottom.size()-2], *op_right[0], *op_right[1]);
	auto is90 = [](double a)->bool { double u = a*32/M_PI; return u>14.9 && u<17.1; };
	if (straight_left && straight_right && straight_bottom &&
			is90(an1) && is90(an2)){
		HM2D::EdgeData top;
		top.emplace_back(new HM2D::Edge(op_left.back(), op_right.back()));
		return Factory(left2, right2, bottom, top, femn, use_rect_approx);
	}

	//4) if left and right are straight and angles are >= pi/2
	//   -> draw equidistant polygon around bot
	HM2D::EdgeData top;
	if (straight_left && straight_right &&
			ISEQGREATER(an1, M_PI/2) && ISEQGREATER(an2, M_PI/2)){
		auto ellipse = HM2D::Contour::Algos::Offset1(bottom, h);
		
		auto pl = HM2D::Contour::GuaranteePoint(ellipse, *HM2D::Contour::Last(left2));
		auto pr = HM2D::Contour::GuaranteePoint(ellipse, *HM2D::Contour::Last(right2));
		top = HM2D::Contour::Constructor::CutContour(ellipse,
				*HM2D::Contour::Last(right2),
				*HM2D::Contour::Last(left2));
		HM2D::Contour::R::ReallyRevert::Permanent(top);
		//getting rid of numerical errors
		//*left2.last() = *top.first();
		//*right2.last() = *top.last();
		HM2D::Contour::First(top)->set(*HM2D::Contour::Last(left2));
		HM2D::Contour::Last(top)->set(*HM2D::Contour::Last(right2));
		goto GEOMETRY_RESULT_CHECK;
	}

	//6) try weighted geometric procedures
	{
		//central ellipse and central point
		auto cellipse = HM2D::Contour::Algos::Offset1(bottom, h);
		HM2D::Contour::R::ReallyRevert::Permanent(cellipse);
		Point vecp1 = HM2D::Contour::WeightPoint(bottom, 0.5);
		Point vecp2 = HM2D::Contour::WeightPoint(bottom, 0.51);
		Vect v=vecp2-vecp1; vecSetLen(v, 100); v=vecRotate(v, M_PI/2);
		auto perp = HM2D::Contour::Constructor::FromPoints({vecp1, vecp1+v});
		auto crres = HM2D::Contour::Algos::Cross(perp, cellipse);
		assert(std::get<0>(crres));
		Point cpoint = std::get<1>(crres);
		//get distances
		double hleft = std::get<1>(HM2D::FindClosestEdge(bottom, *HM2D::Contour::Last(left2)));
		double hright = std::get<1>(HM2D::FindClosestEdge(bottom, *HM2D::Contour::Last(right2)));
		//weight from left
		HM2D::EdgeData top1;
		if (!ISEQ(hleft, h)){
			auto ellipse2 = HM2D::Contour::Algos::Offset1(bottom, hleft);
			HM2D::Contour::R::ReallyRevert::Permanent(ellipse2);
			top1 = ContoursWeight(ellipse2, *HM2D::Contour::Last(left2),
					cellipse, cpoint);
		} else top1 = HM2D::Contour::Constructor::CutContour(cellipse, *HM2D::Contour::Last(left2), cpoint);
		//weight from right
		HM2D::EdgeData top2;
		if (!ISEQ(hright, h)){
			auto ellipse2 = HM2D::Contour::Algos::Offset1(bottom, hright);
			HM2D::Contour::R::ReallyRevert::Permanent(ellipse2);
			top2 = ContoursWeight(cellipse, cpoint,
					ellipse2, *HM2D::Contour::Last(right2));
		} else top2 = HM2D::Contour::Constructor::CutContour(cellipse, cpoint, *HM2D::Contour::Last(right2));
		assert(flequal(top2, top1));
		//assemple top
		vector<Point> topp;
		for (auto p: HM2D::Contour::OrderedPoints(top1)) topp.push_back(*p);
		auto _tp2 = HM2D::Contour::OrderedPoints(top2);
		for (auto p = _tp2.begin() + 1; p!=_tp2.end(); ++p) topp.push_back(**p);
		top = HM2D::Contour::Constructor::FromPoints(topp);
		//getting rid of numerical errors
		//*left2.last() = *top.first();
		//*right2.last() = *top.last();
		HM2D::Contour::First(top)->set(*HM2D::Contour::Last(left2));
		HM2D::Contour::Last(top)->set(*HM2D::Contour::Last(right2));
		goto GEOMETRY_RESULT_CHECK;
	}

GEOMETRY_RESULT_CHECK:
	if (top.size() > 0){
		//no crosses for bottom,
		//cross at the end point for left, right
		auto cr1 = HM2D::Contour::Algos::Cross(bottom, top);
		if (std::get<0>(cr1) == true) goto GEOMETRY_FAILED;
		auto cr2 = HM2D::Contour::Algos::Cross(left2, top);
		if (std::get<0>(cr2) && !ISEQ(std::get<2>(cr2), 1.0)) goto GEOMETRY_FAILED;
		auto cr3 = HM2D::Contour::Algos::Cross(right2, top);
		if (std::get<0>(cr3) && !ISEQ(std::get<2>(cr3), 1.0)) goto GEOMETRY_FAILED;
		//all checks were passed
		return Factory(left2, right2, bottom, top, femn, use_rect_approx);
	}

GEOMETRY_FAILED:
	//7) Failed to connect nodes geometrically.
	//   We need a really smart algorithm here. Smth like
	//   D. Gaier, On an area problem in conformal mapping, Results in Mathematics 10 (1986) 66-81.
	_THROW_NOT_IMP_;
}

double MappedRect::top2bot(double w) const{ return conf2bot(top2conf(w)); }
double MappedRect::bot2top(double w) const{ return conf2top(bot2conf(w)); }


// ============ Open area
RectForOpenArea::RectForOpenArea(HM2D::EdgeData& left, HM2D::EdgeData& right,
			HM2D::EdgeData& bottom, HM2D::EdgeData& top,
			int femn, bool use_rect_approx, bool force_rect_approx):
			MappedRect(left, right, bottom, top, use_rect_approx){
	if (force_rect_approx){
		core = HMMap::Conformal::Impl::RectApprox::Build(left, right, bottom, top);
		return;
	}
	//1. assemble polygon
	std::vector<Point> path;
	std::array<int, 4> corners;
	corners[0] = 0;
	auto tmp = HM2D::Contour::OrderedPoints(left); std::reverse(tmp.begin(), tmp.end());
	for (auto p: tmp) path.push_back(*p);
	path.pop_back();
	corners[1] = path.size();
	for (auto p: HM2D::Contour::OrderedPoints(bottom)) path.push_back(*p);
	path.pop_back();
	corners[2] = path.size();
	for (auto p: HM2D::Contour::OrderedPoints(right)) path.push_back(*p);
	path.pop_back();
	corners[3] = path.size();
	tmp = HM2D::Contour::OrderedPoints(top); std::reverse(tmp.begin(), tmp.end());
	for (auto p: tmp) path.push_back(*p);
	path.pop_back();
	//2. build conformal transformation into rectangle [0,m]x[0,1]
	HMMap::Conformal::Options opt;
	opt.use_rect_approx = use_rect_approx;
	opt.right_angle_eps = M_PI/8.0;
	opt.length_weight = 1.01;
	opt.use_scpack = USE_ANALYTICAL_MAPPINGS;
	opt.fem_nrec = std::min(10000, std::max(100, femn));
	core = HMMap::Conformal::Rect::Factory(path, corners, opt);
};

HM2D::VertexData RectForOpenArea::MapToReal(const vector<const Point*>& p) const{
	vector<Point> ret(p.size());
	double m = core->module();
	//stretch to conformal rectangle
	std::transform(p.begin(), p.end(), ret.begin(), [&m](const Point* it){
			return Point(it->x*m, it->y);
	});
	//run transform
	ret = core->MapToPolygon(ret);
	//return
	HM2D::VertexData col;
	for (auto& it: ret){
		col.push_back(std::make_shared<HM2D::Vertex>(it));
	}
	return col;
}

HM2D::VertexData RectForOpenArea::MapToSquare(const vector<const Point*>& p) const{
	vector<Point> ret(p.size());
	std::transform(p.begin(), p.end(), ret.begin(), [](const Point* it){
			return Point(it->x, it->y);
	});
	//run transform
	ret = core->MapToRectangle(ret);
	//stretch to conformal rectangle
	double m = core->module();
	for (auto& p: ret) p.x/=m;
	//return
	HM2D::VertexData col;
	for (auto& it: ret){
		col.push_back(std::make_shared<HM2D::Vertex>(it));
	}
	return col;
}

double RectForOpenArea::conf2top(double w) const {
	auto p = MappedRect::MapBndToReal(Point(w, 1));
	return std::get<1>(HM2D::Contour::CoordAt(TopContour(), p));
};
double RectForOpenArea::conf2bot(double w) const {
	auto p = MappedRect::MapBndToReal(Point(w, 0));
	return std::get<1>(HM2D::Contour::CoordAt(BottomContour(), p));
}
double RectForOpenArea::top2conf(double w) const {
	auto col = HM2D::Contour::WeightPoints(TopContour(), {w});
	return MappedRect::MapBndToSquare(col[0]).x;
}
double RectForOpenArea::bot2conf(double w) const {
	auto col = HM2D::Contour::WeightPoints(BottomContour(), {w});
	return MappedRect::MapBndToSquare(col[0]).x;
}
double RectForOpenArea::right2conf(double w) const{
	auto col = HM2D::Contour::WeightPoints(RightContour(), {w});
	return MappedRect::MapBndToSquare(col[0]).y;
}
double RectForOpenArea::left2conf(double w) const{
	auto col = HM2D::Contour::WeightPoints(LeftContour(), {w});
	return MappedRect::MapBndToSquare(col[0]).y;
}


//============= CLosed area
RectForClosedArea::RectForClosedArea(const HM2D::EdgeData& side, const HM2D::EdgeData& bottom,
		const HM2D::EdgeData& top):
		MappedRect(side, side, bottom, top, false){
	assert(HM2D::Contour::IsClosed(bottom));
	assert(HM2D::Contour::IsClosed(top));
	//collect points from top
	vector<Point> ptop; ptop.reserve(top.size());
	for (auto p: HM2D::Contour::CornerPoints(top)) ptop.push_back(*p);
	if (ptop[0] != *HM2D::Contour::First(top)) ptop.insert(ptop.begin(), *HM2D::Contour::First(top));
	//collect points from bot
	vector<Point> pbot; pbot.reserve(bottom.size());
	for (auto p: HM2D::Contour::CornerPoints(bottom)) pbot.push_back(*p);
	if (pbot[0] != *HM2D::Contour::First(bottom)) pbot.insert(pbot.begin(), *HM2D::Contour::First(bottom));
	//correct direction
	double a1 = HM2D::Contour::Area(top);
	double a2 = HM2D::Contour::Area(bottom);
	assert(a1*a2>0);
	if (a1<0){
		std::reverse(ptop.begin()+1, ptop.end());
		std::reverse(pbot.begin()+1, pbot.end());
	}
	//set top, bottom so that top be outer and bot - inner contour
	if (fabs(a1)<fabs(a2)) std::swap(ptop, pbot);
	top_is_outer = (fabs(a1)>fabs(a2));
	//assemble conformal mappint to an annulus
	HMMap::Conformal::Options opt;
	opt.use_scpack = USE_ANALYTICAL_MAPPINGS;
	core = HMMap::Conformal::Annulus::Factory(ptop, pbot, opt);
}

shared_ptr<RectForClosedArea>
RectForClosedArea::Build(const HM2D::EdgeData& bottom, const Point* pstart, double h){
	HM2D::Contour::Tree toptree =
		HM2D::Contour::Algos::Offset(bottom, -h, HM2D::Contour::Algos::OffsetTp::RC_CLOSED_POLY);
	//if failed to build single connected area -> do smth
	if (toptree.nodes.size() != 1) _THROW_NOT_IMP_;
	auto top = toptree.nodes[0]->contour;
	if (HM2D::Contour::Area(bottom) * HM2D::Contour::Area(top) < 0){
		HM2D::Contour::Reverse(top);
	}
	//find point on a cross between normal from bottom.first() and top contour
	//and set it as a start point for top
	auto gp = HM2D::Contour::GuaranteePoint(top, *HM2D::Contour::First(bottom));
	//auto top2 = HM2D::Contour::Assembler::ShrinkContour(top, std::get<1>(gp).get(), std::get<1>(gp).get());
	HM2D::Contour::R::ForceFirst::Permanent(top, *std::get<1>(gp));
	//all 4 contours were set. call main routine
	return Build(bottom, top);
}


shared_ptr<RectForClosedArea>
RectForClosedArea::Build(const HM2D::EdgeData& bottom, const HM2D::EdgeData& top){
	HM2D::EdgeData side;
	side.emplace_back(new HM2D::Edge(HM2D::Contour::First(bottom), HM2D::Contour::First(top)));
	return shared_ptr<RectForClosedArea>(new RectForClosedArea(side, bottom, top));
}



HM2D::VertexData RectForClosedArea::MapToReal(const vector<const Point*>& pnt) const{
	//length of curve of first point of inner circle
	double phi0 = (top_is_outer) ? core->PhiInner(0) : core->PhiOuter(0);
	//vector of points in canonic area
	vector<Point> cp;
	for (auto p: pnt){
		double an, r;
		an = ToAngle(2*M_PI*p->x + phi0);
		if (top_is_outer){
			r  = p->y*(1-core->module()) + core->module();
		} else {
			r  = (1 - p->y)*(1-core->module()) + core->module();
		}
		//point
		cp.push_back(Point(r*cos(an), r*sin(an)));
	}
	//do mapping
	cp = core->MapToOriginal(cp);
	//write answer
	HM2D::VertexData ret;
	for (auto p: cp) ret.push_back(std::make_shared<HM2D::Vertex>(p));
	return ret;
}

HM2D::VertexData RectForClosedArea::MapToSquare(const vector<const Point*>& pin) const{
	//map to canonic
	vector<Point> pout(pin.size());
	std::transform(pin.begin(), pin.end(), pout.begin(), [](const Point* p){ return *p; });
	pout = core->MapToAnnulus(pout);
	//(x, y) -> (rad, phi)
	std::transform(pout.begin(), pout.end(), pout.begin(), [](const Point& p){
		return Point(sqrt(sqr(p.x) + sqr(p.y)), atan2(p.y, p.x));
	});
	//(rad, phi) -> (bottom length, rad - inner_radius)
	double m = core->module();
	std::transform(pout.begin(), pout.end(), pout.begin(), [&m](const Point& p){
		return Point(p.y*m, p.x - m);
	});
	//normalize
	double maxx = 2*M_PI*m, maxy = 1.0 - m; 
	double phi0 = (top_is_outer)?core->PhiInner(0):core->PhiOuter(0);
	double x0 = core->module()*phi0;
	std::transform(pout.begin(), pout.end(), pout.begin(), [&](const Point& p){
		Point ret(p.x - x0, p.y);
		while (ret.x < 0) ret.x += 2*M_PI*m;
		ret.x/=maxx; ret.y/=maxy;
		return ret;
	});
	//if top is not out -> reverse y
	if (!top_is_outer){
		std::transform(pout.begin(), pout.end(), pout.begin(), [](const Point& p){
			return Point(p.x, 1.0 - p.y);
		});
	}
	//write answer
	HM2D::VertexData ret;
	std::for_each(pout.begin(), pout.end(), [&ret](const Point& p){
		ret.push_back(std::make_shared<HM2D::Vertex>(p));
	});
	return ret;
}

Point RectForClosedArea::MapBndToReal(const Point& p) const{
	//length of curve of first point of inner circle
	double phi0 = (top_is_outer) ? core->PhiInner(0) : core->PhiOuter(0);
	//vector of points in canonic area
	double an, r;
	an = ToAngle(2*M_PI*p.x + phi0);
	if (top_is_outer){
		r  = p.y*(1-core->module()) + core->module();
	} else {
		r  = (1 - p.y)*(1-core->module()) + core->module();
	}
	//do mapping
	return core->MapToOriginalBnd(Point(r*cos(an), r*sin(an)));
}
Point RectForClosedArea::MapBndToSquare(const Point& p) const{
	//map to canonic
	Point p2 = core->MapToAnnulusBnd(p);
	//(x, y) -> (rad, phi)
	p2 = Point(sqrt(sqr(p2.x) + sqr(p2.y)), atan2(p2.y, p2.x));
	//(rad, phi) -> (bottom length, rad - inner_radius)
	double m = core->module();
	p2 = Point(p2.y*m, p2.x - m);
	//normalize
	double maxx = 2*M_PI*m, maxy = 1.0 - m; 
	double phi0 = (top_is_outer)?core->PhiInner(0):core->PhiOuter(0);
	double x0 = core->module()*phi0;
	p2 = [&](const Point& p){
		Point ret(p.x - x0, p.y);
		while (ret.x < 0) ret.x += 2*M_PI*m;
		ret.x/=maxx; ret.y/=maxy;
		return ret;
	}(p2);
	//if top is not out -> reverse y
	if (!top_is_outer) p2 = Point(p2.x, 1.0 - p2.y);

	return p2;
}

//TODO: move to MappedRect maybe?
double RectForClosedArea::conf2top(double w) const {
	auto p = MapBndToReal(Point(w, 1));
	return std::get<1>(HM2D::Contour::CoordAt(TopContour(),p));
};
double RectForClosedArea::conf2bot(double w) const {
	_THROW_NOT_IMP_;
	auto p = MapBndToReal(Point(w, 0));
	return std::get<1>(HM2D::Contour::CoordAt(BottomContour(),p));
}
double RectForClosedArea::top2conf(double w) const {
	_THROW_NOT_IMP_;
	auto col = HM2D::Contour::WeightPoints(TopContour(), {w});
	return MapBndToSquare(col[0]).x;
}
double RectForClosedArea::bot2conf(double w) const {
	auto p = HM2D::Contour::WeightPoint(BottomContour(), w);
	return MapBndToSquare(p).x;
}
double RectForClosedArea::right2conf(double w) const{
	_THROW_NOT_IMP_;
	auto col = HM2D::Contour::WeightPoints(RightContour(), {w});
	return MapBndToSquare(col[0]).y;
}
double RectForClosedArea::left2conf(double w) const{
	_THROW_NOT_IMP_;
	auto col = HM2D::Contour::WeightPoints(LeftContour(), {w});
	return MapBndToSquare(col[0]).y;
}

// ========================== MappedMesher
void MappedMesher::Fill(TBotPart bottom_partitioner, TVertPart vertical_partitioner, int source){
	HM2D::EdgeData bt = rect->BottomContour();
	//1) get real weights from conformal weights at bottom source
	double wbot_start = (ISEQ(wstart, 0.0)) ? 0.0 : rect->conf2bot(wstart);
	double wbot_end = (ISEQ(wend, 1.0)) ? 1.0 : rect->conf2bot(wend);

	//if bottom is one-edge contour with wrong direction
	assert(wbot_start<wbot_end);

	//2) Build bottom partitions
	auto bpart = bottom_partitioner(wbot_start, wbot_end);

	//3) Build partitions of vertical lines
	vector<vector<double>> vlines;
	for (auto v: bpart) vlines.push_back(vertical_partitioner(v));

	int isz = bpart.size();
	int jsz = std::max_element(vlines.begin(), vlines.end(),
		[](vector<double>& x, vector<double>& y){ return x.size() < y.size(); })->size();

	//4) build regular grid and get vector of bottom side points
	GridGeom g4 = GGeom::Constructor::RectGrid01(isz-1, jsz-1);
	ShpVector<GridPoint> botpts;
	for (auto p: GGeom::Info::BoundaryPoints(g4)){
		if (ISZERO(p->y)) botpts.push_back(p);
	}
	//5) fill layer weights and feature
	std::map<const Cell*, int> lweights;
	shared_ptr<int> pfeat(new int());
	std::map<const Cell*, shared_ptr<int>> feat;
	int k = 0;
	for (int j = 0; j<jsz-1; ++j){
		for (int i=0; i<isz-1; ++i){
			int w;
			switch (source){
				case 1: w = i+1; break;
				case 2: w = j+1; break;
				case 3: w = isz-1-i; break;
				case 4: w = jsz-1-j; break;
				default: assert(false);
			}
			lweights[g4.get_cell(k)] = w;
			feat[g4.get_cell(k)] = pfeat;
			++k;
		}
	}
	//6) save boundary points
	vector<std::weak_ptr<GridPoint>> wleft, wright, wtop, wbot;
	{
		auto shp = GGeom::Info::SharePoints(g4);
		for (int i=0; i<isz; ++i){
			wbot.push_back(shp[i]);
			wtop.push_back(shp[isz*(jsz-1)+i]);
		}
		for (int j=0; j<jsz; ++j){
			wleft.push_back(shp[isz*j]);
			wright.push_back(shp[isz*j+isz-1]);
		}
	}

	//6) copy left/right points to pcollections
	vector<int> right_indicies, left_indicies;
	for (int j=0; j<jsz; ++j){
		left_indicies.push_back(j*isz);
		right_indicies.push_back(j*isz + isz - 1);
	}
	for (int i: left_indicies){
		left_points.push_back(g4.get_point(i));
	}
	for (int i: right_indicies){
		right_points.push_back(g4.get_point(i));
	}
	//7) delete unused cells
	vector<const Cell*> ucells;
	int kc = 0;
	for (int j=0; j<jsz-1; ++j){
		for (int i=0; i<isz-1; ++i){
			if (vlines[i].size() < j+2 || vlines[i+1].size() < j+2){
				ucells.push_back(g4.get_cell(kc));
				auto fnd1 = lweights.find(ucells.back());
				auto fnd2 = feat.find(ucells.back());
				if (fnd1 != lweights.end()) lweights.erase(fnd1);
				if (fnd2 != feat.end()) feat.erase(fnd2);
			}
			++kc;
		}
	}
	GGeom::Modify::RemoveCells(g4, ucells);
	//8.1) Remove deleted points from left/right
	auto _bp = g4.get_bnd_points();
	auto s1 = std::remove_if(left_points.begin(), left_points.end(),
		[&_bp](const GridPoint* p){ return _bp.find(p) == _bp.end(); });
	left_points.resize(s1 - left_points.begin());
	auto s2 = std::remove_if(right_points.begin(), right_points.end(),
		[&_bp](const GridPoint* p){ return _bp.find(p) == _bp.end(); });
	right_points.resize(s2 - right_points.begin());

	//9) weight coordinates
	for_each(bpart.begin(), bpart.end(), [&](double& x){ x = rect->bot2conf(x); });
	double hx = 1.0/(isz-1);
	double hy = 1.0/(jsz-1);
	auto toweights = [&](GridPoint* p){
		int i = lround(p->x/hx), j = lround(p->y/hy);
		assert(i<bpart.size());
		assert(i<vlines.size());
		assert(j<vlines[i].size());
		p->set(bpart[i], vlines[i][j]);
	};
	GGeom::Modify::PointModify(g4, toweights);
	for (int i=0; i<vlines.size(); ++i){
		if (!ISEQ(vlines[i].back(), 1.0)) wtop[i].reset();
	}
	
	//10) modify points using mapping
	vector<Point*> ap(g4.n_points());
	for (int i=0; i<g4.n_points(); ++i) ap[i] = g4.get_point(i);
	HM2D::VertexData mp = rect->MapToReal(ap);
	for (int i=0; i<g4.n_points(); ++i) ap[i]->set(*mp[i]);

	//10) all bt points should present in g4 (e.g. for IGNORE_ALL stepping)
	if (HM2D::Contour::IsOpen(bt) && ISZERO(wstart)) for (int i=0; i<wleft.size(); ++i){
		if (!wleft[i].expired()){
			//TODO
		}
	}
	if (HM2D::Contour::IsOpen(bt) && ISEQ(wend, 1.0)) for (int i=0; i<wright.size(); ++i){
		if (!wright[i].expired()){
			//TODO
		}
	}
	bpart = bottom_partitioner(wbot_start, wbot_end);
	for (int i=0; i<wbot.size(); ++i){
		if (!wbot[i].expired()){
			Point& p = *wbot[i].lock();
			p.set(HM2D::Contour::WeightPoint(rect->BottomContour(), bpart[i]));
		}
	}
	for (int i=0; i<wtop.size(); ++i){
		if (!wtop[i].expired()){
			Point& p = *wtop[i].lock();
			double w = rect->bot2top(bpart[i]);
			p.set(HM2D::Contour::WeightPoint(rect->TopContour(), w));
		}
	}

	//bottom contour
	vector<GridPoint*> ppbot;
	for (auto p: botpts) ppbot.push_back(p.get());
	GGeom::Modify::SnapToContour(g4, bt, ppbot); 
	//left and right only if they coincide with physical boundaries
	if (HM2D::Contour::IsOpen(bt)){
		if (wbot_start == 0.0){ 
			ppbot.clear();
			for (auto p: left_points){
				ppbot.push_back(p);
			}
			//reversing so that snapping contour have grid on its left side
			HM2D::EdgeData lc;
			HM2D::DeepCopy(rect->LeftContour(), lc);
			HM2D::Contour::R::ReallyRevert::Permanent(lc);
			GGeom::Modify::SnapToContour(g4, lc, ppbot); 
			//!!! here we have to refill left_points array but until now
			//there was no need in this because grids with curved left/right
			//contours are not used by complicated connectors.
			//This could be changed in future and this procedure should be written
		}
		if (wbot_end == 1.0){
			ppbot.clear();
			for (auto p: right_points){ ppbot.push_back(p); }
			//no need to reverse because grid lies to the left from RightContour
			GGeom::Modify::SnapToContour(g4, rect->RightContour(), ppbot); 
		}
	}

	//11) copy to results
	GGeom::Modify::ShallowAdd(&g4, &result);
	GGeom::Repair::Heal(result);

	//12) fill weights
	result.AddWeights(lweights);
	result.AddSourceFeat(feat);
}

HM2D::EdgeData MappedMesher::LeftContour(){
	HM2D::VertexData lp;
	for (auto p: left_points) lp.push_back(std::make_shared<HM2D::Vertex>(*p));
	return HM2D::Contour::Assembler::Contour1(lp, false);
}

HM2D::EdgeData MappedMesher::RightContour(){
	HM2D::VertexData rp;
	for (auto p: right_points) rp.push_back(std::make_shared<HM2D::Vertex>(*p));
	return HM2D::Contour::Assembler::Contour1(rp, false);
}
