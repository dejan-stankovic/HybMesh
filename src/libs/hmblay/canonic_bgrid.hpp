#ifndef HYBMESH_HMBLAY_CANONIC_HPP
#define HYBMESH_HMBLAY_CANONIC_HPP
#include "bgrid.hpp"
#include "hmconformal.hpp"

namespace HMBlay{ namespace Impl{


class MappedRect{
	bool _use_rect_approx;
protected:
	HMCont2D::Container<HMCont2D::Contour> left, right, bottom, top;

	MappedRect(HMCont2D::Contour& _left, HMCont2D::Contour& _right,
			HMCont2D::Contour& _bottom, HMCont2D::Contour& _top,
			bool rect_approx):
		_use_rect_approx(rect_approx),
		left(HMCont2D::Container<HMCont2D::Contour>::DeepCopy(_left)),
		right(HMCont2D::Container<HMCont2D::Contour>::DeepCopy(_right)),
		bottom(HMCont2D::Container<HMCont2D::Contour>::DeepCopy(_bottom)),
		top(HMCont2D::Container<HMCont2D::Contour>::DeepCopy(_top))
	{}
public:
	bool use_rect_approx() const { return _use_rect_approx; }
	//real contour mapped by {u=0->1, v=1 }
	HMCont2D::Contour TopContour() const { return HMCont2D::Contour(top); }
	//real contour mapped by {u=0->1, v=0 }
	HMCont2D::Contour BottomContour() const { return HMCont2D::Contour(bottom); }
	//real contour mapped by {u=0, v=0->1 }
	HMCont2D::Contour LeftContour() const { return HMCont2D::Contour(left); }
	//real contour mapped by {u=1, v=0->1 }
	HMCont2D::Contour RightContour() const { return HMCont2D::Contour(right); }
	//real coordinates from normalized in [0,1] square
	virtual HMCont2D::PCollection MapToReal(const vector<const Point*>& p) const = 0;
	Point MapToReal(const Point& p) const{ return *(MapToReal({&p}).point(0)); }
	//square coordinates from real coordinates
	virtual HMCont2D::PCollection MapToSquare(const vector<const Point*>& p) const = 0;
	HMCont2D::PCollection MapToSquare(const vector<Point*>& p) const{
		return MapToSquare(vector<const Point*>(p.begin(), p.end()));
	}
	Point MapToSquare(const Point& p) const{ return *(MapToSquare({&p}).point(0)); }

	// ================ constructors:
	//1. From four lines
	//direction of contours:
	//left   (u=0;    v: 0->1)
	//right  (u=1;    v: 0->1)
	//bottom (u=0->1; v: 0)
	//top    (u=0->1; v: 1)
	static shared_ptr<MappedRect>
	Factory(HMCont2D::Contour& left, HMCont2D::Contour& right,
			HMCont2D::Contour& bottom, HMCont2D::Contour& top,
			bool use_rect_approx);

	//2. From three lines (no top) and vertical distance h
	static shared_ptr<MappedRect>
	Factory(HMCont2D::Contour& left, HMCont2D::Contour& right,
			HMCont2D::Contour& bottom, double h,
			bool use_rect_approx);
};

class RectForOpenArea: public MappedRect{
	shared_ptr<HMMath::Conformal::Rect> core;
public:
	RectForOpenArea(HMCont2D::Contour& left, HMCont2D::Contour& right,
			HMCont2D::Contour& bottom, HMCont2D::Contour& top,
			bool use_rect_approx);
	HMCont2D::PCollection MapToReal(const vector<const Point*>& p) const override;

	HMCont2D::PCollection MapToSquare(const vector<const Point*>& p) const override;
};


class RectForClosedArea: public MappedRect{
public:
	RectForClosedArea(HMCont2D::Contour& side, HMCont2D::Contour& bottom,
			HMCont2D::Contour& top, bool rect_approx);
	HMCont2D::PCollection MapToReal(const vector<const Point*>& p) const override;
	HMCont2D::PCollection MapToSquare(const vector<const Point*>& p) const override {_THROW_NOT_IMP_;}
};


// ============================== Meshers
class MappedMesher{
public:
	//start and end square x coordinates of meshing
	double wstart, wend;
	MappedRect* rect;
	MappedMesher(MappedRect* r, double w1, double w2){
		rect = r; wstart = w1; wend = w2;
	};

	HMCont2D::PCollection right_points, left_points;
	BGrid result;

	//takes weight start, weight end of source line.
	//returns vector of desired partition points by weight coordinate
	//including start and end weights
	typedef std::function<vector<double>(double, double)> TBotPart;
	//takes point by weight coordinate on source line,
	//return weight depths of blayers starting from 0.0
	//depth = 1 is the height of rect at certain weight
	typedef std::function<vector<double>(double)>  TVertPart;

	//Build a grid
	void Fill(TBotPart bottom_partitioner, TVertPart vertical_partitioner);

	//returns grid boundaries as shallow copy
	HMCont2D::Contour LeftContour();
	HMCont2D::Contour RightContour();
	
	//weights transformer
	double conf2top(double w) const;
	double conf2bot(double w) const;
	double top2conf(double w) const;
	double bot2conf(double w) const;
	double top2bot(double w) const;
	double bot2top(double w) const;
	double right2conf(double w) const;
	double left2conf(double w) const;
};



}}
#endif

