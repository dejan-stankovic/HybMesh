#ifndef HYBMESH_BGEOM_H
#define HYBMESH_BGEOM_H

#include "hmproject.h"
#include <algorithm>
#include "addalgo.hpp"
//Point
struct Point{
	double x,y;
	//constructors
	explicit Point(double _x=0, double _y=0) noexcept:x(_x), y(_y){}
	Point(const Point& p2) noexcept:x(p2.x), y(p2.y){}
	Point& operator=(const Point& p) noexcept{
		if (this!=&p){
			this->x=p.x; this->y=p.y;
		}
		return *this;
	}
	void set(double a=0, double b=0) noexcept {x=a; y=b;}
	void set(const Point& p2) noexcept {x=p2.x; y=p2.y;}
	// ============== measures and distances
	static double meas(const Point& p1, const Point& p2) noexcept{
		return (p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y);
	}
	static double dist(const Point& p1, const Point& p2) noexcept{
		return sqrt(meas(p1,p2));
	}
	//measure from p to [L1, L2]. ksi is the weight section coordinate closest to p
	static double meas_section(const Point& p, const Point& L1, const Point& L2) noexcept;
	static double meas_section(const Point& p, const Point& L1, const Point& L2, double& ksi) noexcept;
	//measure from p to [L1, L2] line
	static double meas_line(const Point& p, const Point& L1, const Point& L2) noexcept;
	//p lies to the left => positive return; to the right => negative return
	static double signed_meas_line(const Point& p, const Point& L1, const Point& L2) noexcept;
	//normalized line equation
	static std::array<double, 3> line_eq(const Point& p1, const Point& p2) noexcept;

	//finds a point between two given ones with a certain weight w
	static Point Weigh(const Point& p1, const Point& p2, double w) noexcept{
		return Point(p1.x*(1-w)+p2.x*w, p1.y*(1-w)+p2.y*w);
	}
};

//point operators
inline Point operator+(const Point& left, const Point& right) noexcept{
	return Point(left.x+right.x, left.y+right.y);
}
inline Point operator-(const Point& left, const Point& right) noexcept{
	return Point(left.x-right.x, left.y-right.y);
}
inline Point& operator+=(Point& left, const Point& right) noexcept{
	left.x+=right.x; left.y+=right.y;
	return left;
}
inline Point& operator-=(Point& left, const Point& right) noexcept{
	left.x-=right.x; left.y-=right.y;
	return left;
}
inline Point& operator/=(Point& left, double d) noexcept{
	left.x/=d; left.y/=d;
	return left;
}
inline Point& operator*=(Point& left, double d) noexcept{
	left.x*=d; left.y*=d;
	return left;
}
inline Point operator/(const Point& p, double d) noexcept{ auto x=Point(p); return std::move(x/=d); }
inline Point operator*(const Point& p, double d) noexcept{ auto x=Point(p); return std::move(x*=d); }
inline bool operator==(const Point& p1, const Point& p2) noexcept{ return (ISEQ(p1.x, p2.x) && ISEQ(p1.y, p2.y)); }
inline bool operator!=(const Point& p1, const Point& p2) noexcept{ return (!ISEQ(p1.x, p2.x) || !ISEQ(p1.y, p2.y)); }
inline bool operator<(const Point& p1, const Point& p2) noexcept{
	return (ISEQ(p1.x, p2.x)) ? ISLOWER(p1.y, p2.y) : ISLOWER(p1.x, p2.x);
}

inline std::ostream& operator<<(std::ostream& os, const Point& p){
	os<<p.x<<" "<<p.y;
	return os;
}

bool isOnSection(const Point& p, const Point& start, const Point& end, double& ksi, double eps=geps) noexcept;

//Finds a cross point between two sections: (p1S,p1E) and (p2S, p2E).
//ksieta -- ouput cross weights: [0] -- weight in first section, [1] -- weight in second section
//Returns true if 0<=ksieta[0,1]<=1.
//if sections are parallel: ksieta[0,1]=gbig, returns false
bool SectCross(const Point& p1S, const Point& p1E, const Point& p2S, const Point& p2E, double* ksieta) noexcept;
//same but with internal renorming. Gives better results aber it is slower.
bool SectCrossWRenorm(const Point& p1S, const Point& p1E, const Point& p2S, const Point& p2E, double* ksieta) noexcept;

struct TSectCrossGeps{ 
	//-2 lies on (P1,P2) line left to P1
	//-1 equals P1;
	// 0 lies on (P1,P2) line in (P1, P2) segment
	// 1 equals P2;
	// 2 lies on (P1,P2) line right to P2
	// 3 lies in the left halfplane of (P1, P2) 
	// 4 lies in the right halfplane of (P1, P2) 
	char posA, posB;

	// 0 - same line
	// 1 - parallel lines
	// 2 - non-parallel lines
	char posAB;

	//cross constants for P1P2 and AB respectively. Only for posAB==2
	//-2, before first pnt
	//-1, = first pnt
	// 0, within
	// 1, = last pnt
	// 2, after second pnt
	char ksi, eta;

	bool inner_cross() const {
		return posAB == 2 && ksi==0 && eta==0;
	}
	bool segment_cross() const{
		return (posAB == 0 && posA*posB <=0);
	}
	bool a_on_line() const { return abs(posA)<2; }
	bool b_on_line() const { return abs(posB)<2; }
	bool has_contact() const {
		if (posAB == 0){
			return a_on_line() || b_on_line() ||
				(posA == -2 && posB == 2) ||
				(posA == 2 && posB == -2);
		} else if (posAB == 2){
			return abs(ksi)<2 && abs(eta)<2;
		} else return false;
	}
};
TSectCrossGeps SectCrossGeps(const Point& P1, const Point& P2, Point A, Point B);

//=>
//  0 if p lies to the left of [L1->L2] line
//  1 if p lies on [L1->L2] line
//  2 if p lies to the right of [L1->L2] line
// -1 if (L1 == L2) 
int LinePointWhereIs(const Point& p, const Point& L1, const Point& L2) noexcept;

//scaling
struct ScaleBase{
	Point p0;
	double L;
	explicit ScaleBase(double x0=0, double y0=0, double _L=1): p0(x0, y0), L(_L){}

	//scale and unscale procedures
	void scale(Point& p) const noexcept {p-=p0; p/=L; }
	void unscale(Point& p) const noexcept {p*=L; p+=p0; }

	//scale points in container with Points
	template<class FirstIter, class LastIter>
	void scale(FirstIter start, LastIter end) const noexcept {
		for (auto it=start; it!=end; ++it){ scale(*it); }
	}
	template<class FirstIter, class LastIter>
	void unscale(FirstIter start, LastIter end) const noexcept{
		for (auto it=start; it!=end; ++it){ unscale(*it); }
	}
	//forces all points in container be in 1x1x1 square preserving aspect ratio
	template<class FirstIter, class LastIter>
	static ScaleBase doscale(FirstIter start, LastIter end, double a=1.0) noexcept;
	
	//scale points in container with Points*
	template<class FirstIter, class LastIter>
	void p_scale(FirstIter start, LastIter end) const noexcept {
		for (auto it=start; it!=end; ++it){ scale(**it); }
	}
	template<class FirstIter, class LastIter>
	void p_unscale(FirstIter start, LastIter end) const noexcept{
		for (auto it=start; it!=end; ++it){ unscale(**it); }
	}
	template<class Iter>
	static ScaleBase p_doscale(Iter start, Iter end, double a=1.0) noexcept;
};

//find a node by coordinate
class NodeFinder{
	const int Nx, Ny;
	const double eps;
	const double x0, y0, x1, y1, hx, hy;
	vector<int> get_index(const Point* p) const;
	int to_glob(int i, int j) const { return j*Nx + i; }
	std::vector<std::vector<const Point*>> data;
	bool is_equal_point(const Point* p1, const Point* p2) const;
public:
	NodeFinder(Point p0, double Lx, double Ly, int Nx=30, int Ny=30, double eps=geps);
	NodeFinder(const std::pair<Point, Point>& rect, int Nx=30, int Ny=30, double eps=geps);
	//adds point to data list if necessary
	//returns pointer to previously added point if p was already added,
	//p if point was added to point data
	//and 0 if p lies outside defined rectangle
	const Point* add(const Point* p);
	//returns pointer to previously added point if p was already added
	//and 0 otherwise
	const Point* find(const Point* p) const;
};

//Angles
//to angle in [0, 2pi]
inline double ToAngle(double angle, double eps=0.0){
	if (fabs(angle)<eps || fabs(angle-2*M_PI)<eps) return 0.0;
	if (angle<0) return ToAngle(angle+2*M_PI);
	else if (angle>=2*M_PI) return ToAngle(angle-2*M_PI, eps);
	else return angle;
}
inline double DegToAngle(double deg){
	return ToAngle(deg/180*M_PI);
}
inline double AngleAdd(double angle, double add, double eps=0.0){ return ToAngle(angle+add,eps); }
//p3->p1 counterclockwise turn around p2. Lies in [0, 2 pi]
inline double Angle(const Point& p1, const Point& p2, const Point& p3){
	return ToAngle(atan2(p1.y-p2.y, p1.x-p2.x) - atan2(p3.y-p2.y, p3.x-p2.x));
}

//Vector
typedef Point Vect;
inline double vecDot(const Vect& u, const Vect& v){ return u.x*v.x+u.y*v.y; }
inline double vecLen(const Vect& a){ return sqrt(vecDot(a,a)); }
inline void vecSetLen(Vect& a, double Len){ a/=(vecLen(a)/Len); }
inline void vecNormalize(Vect& a){ a/=vecLen(a); }
inline double vecCrossZ(const Vect& a, const Vect& b){ return a.x*b.y-a.y*b.x; }
inline Vect vecRotate(const Vect& u, double angle){
	double cs = cos(angle), sn = sin(angle);
	return Point(u.x*cs - u.y*sn, u.x*sn+u.y*cs);
}

//triangle procedures
inline double triarea(const Point& p1, const Point& p2, const Point& p3){
	return 0.5*vecCrossZ(p1-p3, p2-p3);
}

//add refinement points within [0, Len] section. 0 and Len are not included.
vector<double> RefineSection(double a, double b, double Len, double Den);

struct BoundingBox{
	double xmin, xmax, ymin, ymax;

	BoundingBox():xmin(0), xmax(0), ymin(0), ymax(0){}
	BoundingBox(double x0, double y0, double x1, double y1):xmin(x0), xmax(x1), ymin(y0), ymax(y1){}
	BoundingBox(const vector<BoundingBox>&, double e=0.0);
	BoundingBox(const Point& p1, const Point& p2, double e=0.0);
	BoundingBox(const Point& p1, double e=0.0);

	//Container<Point> -> BoundingBox
	template<class Iter>
	static BoundingBox Build(Iter first, Iter last, double e=0.){
		if (first == last) return BoundingBox();
		BoundingBox ret(first->x, first->y, first->x, first->y);
		while (first!=last) {ret.widen(*first); ++first;}
		ret.widen(e);
		return ret;
	}
	//Container<Point*> -> BoundingBox
	template<class Iter>
	static BoundingBox PBuild(Iter first, Iter last, double e=0.){
		if (first == last) return BoundingBox();
		BoundingBox ret((*first)->x, (*first)->y, (*first)->x, (*first)->y);
		while (first!=last) {ret.widen(**first); ++first;}
		ret.widen(e);
		return ret;
	}

	//Enlarge if point lies outside box
	void widen(const Point& p);
	//widen to all directions at certain distance
	void widen(double e);
	//sum of boundingboxes
	void widen(const BoundingBox& p);

	Point center() const;

	double area() const;
	double lenx() const { return xmax-xmin; }
	double leny() const { return ymax-ymin; }
	double maxlen() const { return std::max(lenx(), leny()); }
	double lendiag() const { return sqrt(lenx()*lenx() + leny()*leny()); }
	Point pmin() const { return Point(xmin, ymin); }
	Point pmax() const { return Point(xmax, ymax); }
	std::array<Point, 4> four_points() const;
	ScaleBase to_scale() const { return ScaleBase(xmin, ymin, std::max(lenx(), leny())); }

	//-> INSIDE, OUTSIDE, BOUND
	int whereis(const Point& p) const;
	//does this have any intersections or tangent segments with another segment
	//returns false only if bb lies strictly outside this
	bool has_common_points(const BoundingBox& bb) const;
	//does this contain any part of [p1, p2] segment
	bool contains(const Point& p1, const Point& p2) const;

	//returns:
	//0 - bb equals this
	//1 - bb is inside this (with possible touched faces)
	//2 - this is inside bb (with possible touched faces)
	//3 - bb and this doesn't intersect (with possible touched faces)
	//4 - faces of bb and this cross each other
	int relation(const BoundingBox& bb) const;

	//Filter points from container<Point*>
	template<class Container>
	Container Filter(const Container& data,
			bool inside, bool bound, bool outside){
		Container out;
		for (auto& p: data){
			int pos = whereis(*p);
			if (inside && pos == INSIDE) out.push_back(p);
			else if (bound && pos == BOUND) out.push_back(p);
			else if (outside && pos == OUTSIDE) out.push_back(p);
		}
		return out;
	}
};

struct BoundingBoxFinder{
	//L - step size
	BoundingBoxFinder(const BoundingBox& area, double L);

	void addentry(const BoundingBox& e);
	void insertentry(const BoundingBox& e, int ind);
	void raw_addentry(const vector<int>& isqr);
	void raw_insertentry(const vector<int>& isqr, int ind);
	vector<int> suspects(const BoundingBox& bb) const;
	vector<int> suspects(const Point& bb) const;
	//returns only data from boxes which contain segment
	vector<int> suspects(const Point&, const Point&) const;

	//data access
	vector<int> sqrs_by_entry(int e) const;
	vector<int> sqrs_by_point(const Point&) const;
	//returns only boxes which contain segment
	vector<int> sqrs_by_segment(const Point&, const Point&) const;
	vector<int> sqrs_by_bbox(const BoundingBox&) const;

	int nsqr() const { return (mx+1)*(my+1); }
	const vector<int>& sqr_entries(int i) const { return data[i]; }
	vector<int> sqr_entries(const std::vector<int>& isqr);
	Point sqr_center(int i) const;
	int nx() const { return mx+1; }
	int ny() const { return my+1; }
	double stepx() const { return hx; }
	double stepy() const { return hy; }
	Point pmin() const { return Point(x0, y0); }
	Point pmax() const { return Point(x0 + hx*nx(), y0 + hy*ny()); }
private:
	double x0, y0, hx, hy;
	int mx, my;
	int totaldata;
	vector<vector<int>> data;
	vector<int> allsuspects(int x0, int x1, int y0, int y1) const;
	void addsuspects(int ix, int iy, vector<int>& ret) const;
	int get_xstart(double x) const;
	int get_ystart(double y) const;
	int get_xend(double x) const;
	int get_yend(double y) const;
};

// ============================ template implementations
template<class FirstIter, class LastIter>
ScaleBase ScaleBase::doscale(FirstIter start, LastIter end, double a) noexcept{
	if (start >= end) return ScaleBase();
	auto bb = BoundingBox::Build(start, end);
	Point p0 = bb.pmin();
	double L = bb.maxlen() / a;
	ScaleBase ret(p0.x, p0.y, L);
	ret.scale(start, end);
	return ret;
}

template<class Iter>
ScaleBase ScaleBase::p_doscale(Iter start, Iter end, double a) noexcept{
	if (start >= end) return ScaleBase();
	auto bb = BoundingBox::PBuild(start, end);
	Point p0 = bb.pmin();
	double L = bb.maxlen() / a;
	ScaleBase ret(p0.x, p0.y, L);
	ret.p_scale(start, end);
	return ret;
}

#endif
