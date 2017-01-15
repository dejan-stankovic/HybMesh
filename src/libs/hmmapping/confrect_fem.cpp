#include "confrect_fem.hpp"
#include "hmfem.hpp"
#include "femassembly.hpp"
#include "constructor.hpp"
#include "cont_assembler.hpp"
#include "algos.hpp"
#include "contabs2d.hpp"
#include "finder2d.hpp"

#define FEM_NMAX 1000000
#define APPROX_PART 200

using namespace HMMap::Conformal::Impl::ConfFem;
namespace{

double IntegralGrad2(const HM2D::GridData* grid, const vector<double>& v){
	using namespace HMFem;
	shared_ptr<HMMath::Mat> ddx = Assemble::DDx(*grid);
	shared_ptr<HMMath::Mat> ddy = Assemble::DDy(*grid);
	vector<double> vdx(ddx->rows()), vdy(ddx->rows()), mass = Assemble::LumpMass(*grid);
	ddx->MultVec(v, vdx); 
	ddy->MultVec(v, vdy);
	for (int i=0; i<mass.size(); ++i){ vdx[i] /= mass[i]; vdy[i] /= mass[i]; }
	//3.2) |grad|^2 -> vdx
	std::transform(vdx.begin(), vdx.end(), vdy.begin(), vdx.begin(),
			[](double a, double b){ return a*a + b*b; });
	//3.3) integral
	return std::inner_product(vdx.begin(), vdx.end(), mass.begin(), 0.0);
}

void GradVectors(const HM2D::GridData* grid, const vector<double>& v,
		vector<double>& dx, vector<double>& dy){
	using namespace HMFem;
	shared_ptr<HMMath::Mat> ddx = Assemble::DDx(*grid);
	shared_ptr<HMMath::Mat> ddy = Assemble::DDy(*grid);
	vector<double> mass = Assemble::LumpMass(*grid);
	dx.resize(grid->vvert.size()); dy.resize(grid->vvert.size());
	ddx->MultVec(v, dx); 
	ddy->MultVec(v, dy);
	for (int i=0; i<mass.size(); ++i){ dx[i] /= mass[i]; dy[i] /= mass[i]; }
}

HM2D::VertexData ExtractPoints(const HM2D::GridData& grid, const vector<int>& ind){
	HM2D::VertexData ret;
	for (int i: ind) ret.push_back(grid.vvert[i]);
	return ret;
}

double LinTriangleSize(double area, int ncells){ return 1.5*sqrt(4.0*(area/ncells)/sqrt(3));}


}

HMCallback::FunctionWithCallback<ToRect::TBuild> ToRect::Build;
ToRect ToRect::TBuild::_run(const vector<Point>& path, int i1, int i2, int i3, const Options& opt){
	ToRect ret;

	//1) build fem grid: fill grid, origs, HM2D::EdgeData data
	BuildGrid(ret, path, i1, i2, i3, opt.fem_nrec);
	//2) compute mapping: fill u, v, module, approx
	DoMapping(ret);
	//3) build inverse
	BuildInverse(ret);

	return ret;
}

void ToRect::TBuild::BuildGrid(ToRect& r, const vector<Point>& path, int i1, int i2, int i3, int n){
	//1) build grid
	auto subcaller = callback->bottom_line_subrange(45);
	auto cont = HM2D::Contour::Constructor::FromPoints(path, true);
	r.grid = std::make_shared<HM2D::GridData>(
		HMFem::AuxGrid3.UseCallback(subcaller, cont, n, FEM_NMAX));

	callback->step_after(5, "Assembling original contour");
	r.approx = std::make_shared<HMFem::Grid43::Approximator>(
		r.grid.get(), 40);
	//2) restore origs
	HM2D::EdgeData c = HM2D::Contour::Assembler::GridBoundary1(*r.grid);
	HM2D::VertexData cverts = HM2D::AllVertices(c);
	HM2D::Finder::VertexMatch cmatch(cverts);
	aa::enumerate_ids_pvec(r.grid->vvert);
	r.origs = aa::get_ids(cmatch.find(path));
	//3) build orig contours
	HM2D::Vertex *p0, *p1, *p2, *p3;
	p0 = r.grid->vvert[r.origs[0]].get();
	p1 = r.grid->vvert[r.origs[i1]].get();
	p2 = r.grid->vvert[r.origs[i2]].get();
	p3 = r.grid->vvert[r.origs[i3]].get();
	auto left = HM2D::Contour::Assembler::ShrinkContour(c, p0, p1);
	auto bottom = HM2D::Contour::Assembler::ShrinkContour(c, p1, p2);
	auto right = HM2D::Contour::Assembler::ShrinkContour(c, p2, p3);
	auto top = HM2D::Contour::Assembler::ShrinkContour(c, p3, p0);

	aa::enumerate_ids_pvec(r.grid->vvert);
	r.ileft = aa::get_ids(HM2D::Contour::OrderedPoints(left));
	r.iright = aa::get_ids(HM2D::Contour::OrderedPoints(right));
	r.itop = aa::get_ids(HM2D::Contour::OrderedPoints(top));
	r.ibottom = aa::get_ids(HM2D::Contour::OrderedPoints(bottom));
}

void ToRect::TBuild::DoMapping(ToRect& r){
	using namespace HMFem;
	r.u.resize(r.grid->vvert.size(), 0.0);
	r.v.resize(r.grid->vvert.size(), 0.0);
	//1) assemble pure laplas operator
	callback->step_after(10, "Laplace operator");
	auto laplas = Assemble::PureLaplas(*r.grid);
	//2.1) Laplas problem for u
	callback->step_after(10, "U-problem");
	auto ulaplas = LaplasProblem(*r.grid, laplas);
	ulaplas.SetDirichlet(ExtractPoints(*r.grid, r.ileft), [](const HM2D::Vertex*){ return 0; });
	ulaplas.SetDirichlet(ExtractPoints(*r.grid, r.iright), [](const HM2D::Vertex*){ return 1; });
	ulaplas.Solve(r.u);
	//2.2) Laplas problem for v
	callback->step_after(10, "V-problem");
	auto vlaplas = LaplasProblem(*r.grid, laplas);
	vlaplas.SetDirichlet(ExtractPoints(*r.grid, r.ibottom), [](const HM2D::Vertex*){ return 0; });
	vlaplas.SetDirichlet(ExtractPoints(*r.grid, r.itop), [](const HM2D::Vertex*){ return 1; });
	vlaplas.Solve(r.v);

	//3) Compute modulus
	callback->step_after(5, "Modulus");
	//Calculating using area integration.
	//Taking into account 
	//    Int(dvdn)dtop = Int (dvdn*v) dG = Int (grad v)^2 dD
	r._module = IntegralGrad2(r.grid.get(), r.v);
	
	//4) correct u according to modulus
	for (auto& x: r.u) x *= r._module;
}

void ToRect::TBuild::BuildInverse(ToRect& r){
	callback->step_after(15, "Inversion");
	//grid
	r.inv_grid = std::make_shared<HM2D::GridData>();
	HM2D::DeepCopy(*r.grid, *r.inv_grid);
	for (int i=0; i<r.u.size(); ++i){
		r.inv_grid->vvert[i]->set(r.u[i], r.v[i]);
	}
	//approximator
	r.inv_approx = std::make_shared<HMFem::Grid43::Approximator>(
		r.inv_grid.get(), 40);
	//inverse functions
	r.inv_u.resize(r.inv_grid->vvert.size());
	r.inv_v.resize(r.inv_grid->vvert.size());
	for (int i=0; i<r.inv_u.size(); ++i){
		r.inv_u[i] = r.grid->vvert[i]->x;
		r.inv_v[i] = r.grid->vvert[i]->y;
	}
}

vector<Point> ToRect::MapToPolygon(const vector<Point>& input) const{
	vector<Point> ret;
	vector<const vector<double>*> funs {&inv_u, &inv_v};
	for (auto& p: input){
		auto s = inv_approx->Vals(p, funs);
		ret.push_back(Point(s[0], s[1]));
	}
	return ret;
}

vector<Point> ToRect::MapToRectangle(const vector<Point>& input) const{
	vector<Point> ret;
	vector<const vector<double>*> funs {&u, &v};
	for (auto& p: input){
		auto s = approx->Vals(p, funs);
		ret.push_back(Point(s[0], s[1]));
	}
	return ret;
}

vector<Point> ToRect::RectPoints() const{
	vector<Point> ret;
	for (auto i: origs) ret.push_back(Point(u[i], v[i]));
	return ret;
}


// ================== Annulus
shared_ptr<ToAnnulus>
ToAnnulus::Build(const vector<Point>& outer_path, const vector<Point>& inner_path, const Options& opt){
	// - build mapping
	shared_ptr<ToAnnulus> ret(new ToAnnulus(outer_path, inner_path, opt));
	// - return
	if (ret->module() > 0) return ret;
	else return 0;
}

void ToAnnulus::BuildGrid1(const vector<Point>& outer_path, const vector<Point>& inner_path,
		int n){
	//grid
	auto c1 = HM2D::Contour::Constructor::FromPoints(inner_path, true);
	auto c2 = HM2D::Contour::Constructor::FromPoints(outer_path, true);
	HM2D::Contour::Tree pretree;
	pretree.add_contour(c1);
	pretree.add_contour(c2);
	grid = std::make_shared<HM2D::GridData>(
		HMFem::AuxGrid3(pretree, n, FEM_NMAX));
	approx = std::make_shared<HMFem::Grid43::Approximator>(
		grid.get(), 40);
	//boundary indicies
	HM2D::Contour::Tree ct = HM2D::Contour::Tree::GridBoundary(*grid);
	auto outerc = HM2D::Contour::OrderedPoints(ct.roots()[0]->contour);
	auto innerc  = HM2D::Contour::OrderedPoints(ct.roots()[0]->children[0].lock()->contour);
	aa::enumerate_ids_pvec(grid->vvert);
	inner = aa::get_ids(innerc);
	outer = aa::get_ids(outerc);
}

void ToAnnulus::DoMappingU(){
	//laplas problem
	auto ulaplas = HMFem::LaplasProblem(*grid);
	//dirichlet: one on inner, zero on outer
	ulaplas.SetDirichlet(ExtractPoints(*grid, outer), [](const HM2D::Vertex*){return 0.0;});
	ulaplas.SetDirichlet(ExtractPoints(*grid, inner), [](const HM2D::Vertex*){return 1.0;});
	//solution
	u.resize(grid->vvert.size(), 0.0);
	ulaplas.Solve(u);
	//Modulus
	_module = exp( -2*M_PI/IntegralGrad2(grid.get(), u) );
}

HM2D::EdgeData ToAnnulus::SteepestDescentUCurve(){
	auto tree = HM2D::Contour::Tree::GridBoundary(*grid);
	//get derivatives
	vector<double> dx, dy;
	GradVectors(grid.get(), u, dx, dy);
	//step
	double h = sqrt(tree.area()/grid->vcells.size())/2.0;
	//outer contour
	HM2D::EdgeData outerc = tree.roots()[0]->contour;
	//go
	vector<Point> ret; ret.push_back(pzero);
	while (HM2D::Contour::Finder::WhereIs(outerc, ret.back()) == INSIDE){
		vector<double> grad = approx->Vals(ret.back(), {&dx, &dy});
		Vect gradv {grad[0], grad[1]};
		vecSetLen(gradv, h);
		ret.push_back(ret.back() - gradv);
	}
	//assemble contour
	auto cont = HM2D::Contour::Constructor::FromPoints(ret);
	//if last point lies outside outerc cut cont
	if (HM2D::Contour::Finder::WhereIs(outerc, ret.back()) == OUTSIDE){
		auto cr = HM2D::Contour::Finder::Cross(outerc, cont);
		*(HM2D::Contour::Last(cont)) = std::get<1>(cr);
	}
	//if distance beween last point and one before last is very short
	//remove point before last
	if (cont.back()->length()<h/4){
		auto pp = HM2D::Contour::Last(cont);
		cont.pop_back();
		*HM2D::Contour::Last(cont) = *pp;
	}
	return cont;
}


void ToAnnulus::BuildGrid2(const vector<Point>& outer_path,
		const vector<Point>& inner_path, int n,
		const HM2D::EdgeData& razor){
	//starting from grid/approx filled with doubly-connected grid
	//0) assemble inner and outer contours
	auto innerc = std::make_shared<HM2D::EdgeData>(
			HM2D::Contour::Constructor::FromPoints(inner_path, true));
	auto outerc = std::make_shared<HM2D::EdgeData>(
			HM2D::Contour::Constructor::FromPoints(outer_path, true));
	HM2D::Contour::Tree ct;
	ct.add_contour(*innerc);
	ct.add_contour(*outerc);
	
	//1) assemble doubly-connected contour
	grid = std::make_shared<HM2D::GridData>(
		HMFem::AuxGrid3(ct, vector<HM2D::EdgeData> {razor}, n, FEM_NMAX));
	//2) rip double-connected contour by razor polyline
	//2.1) get points which lie on razor
	std::map<const HM2D::Vertex*, double> razor_points;
	std::map<const HM2D::Vertex*, shared_ptr<HM2D::Vertex>> m12;
	auto razorbox = HM2D::BBox(razor, geps);
	for (int i=0; i<grid->vvert.size(); ++i){
		auto& v = *grid->vvert[i];
		if (razorbox.whereis(v) == OUTSIDE) continue;
		auto cr = HM2D::Contour::CoordAt(razor, v);
		if (ISZERO(std::get<4>(cr))){
			razor_points.emplace(&v, std::get<0>(cr));
			//double razor points and add em to grid
			auto np = std::make_shared<HM2D::Vertex>(v);
			grid->vvert.push_back(np);
			m12.emplace(&v,  np);
		}
	}

	//2.3) change razor->razor2 for cells which lie to the right
	aa::constant_ids_pvec(grid->vvert, 0);
	for (auto& m: m12){
		m.first->id = 1;
		m.second->id = 2;
	}
	vector<HM2D::Edge*> razor_edges;
	for (auto& e: grid->vedges){
		if (e->first()->id == 1 && e->last()->id == 1){
			double d1 = razor_points[e->first().get()];
			double d2 = razor_points[e->last().get()];
			if (d1 > d2) e->reverse();
			razor_edges.push_back(e.get());
		}
	}
	for (auto& e: razor_edges){
		auto rc = e->right.lock();
		auto ne = std::make_shared<HM2D::Edge>(*e);
		e->right.reset();
		ne->left.reset();
		for (auto& re: rc->edges){
			if (re.get() == e) re = ne;
			for (auto& v: re->vertices) if (v->id == 1){
				v = m12[v.get()];
			}
		}
		grid->vedges.push_back(ne);
	}

	//3) origs
	HM2D::EdgeData ct2 = HM2D::Contour::Assembler::GridBoundary1(*grid);
	HM2D::VertexData allp = HM2D::AllVertices(ct2);
	HM2D::Finder::VertexMatch cmatch(allp);
	aa::enumerate_ids_pvec(grid->vvert);
	inner_origs = aa::get_ids(cmatch.find(inner_path));
	outer_origs = aa::get_ids(cmatch.find(outer_path));
	//4) inner/outer
	// grid has doubled points at razor sides.
	// therefore cmatch doesn't represent all grid boundary points.
	// hence we use __bp instead of _bp.
	inner.clear(); outer.clear();
	for (auto v: allp){
		auto fnd1 = HM2D::Finder::ClosestEdge(*outerc, *v);
		if (ISZERO(std::get<1>(fnd1))){
			outer.push_back(v->id);
			continue;
		} 
		auto fnd2 = HM2D::Finder::ClosestEdge(*innerc, *v);
		if (ISZERO(std::get<1>(fnd2))){
			inner.push_back(v->id);
			continue;
		} 
	}
	//5) razor io/oi
	raz_io.clear(); raz_oi.clear();
	for (auto m: m12){
		raz_io.push_back(m.first->id);
		raz_oi.push_back(m.second->id);
	}
	//6) approx
	approx = std::make_shared<HMFem::Grid43::Approximator>(
		grid.get(), 40);
}

void ToAnnulus::DoMapping(){
	//laplas matrix
	auto lap = HMFem::Assemble::PureLaplas(*grid);
	//laplas problem for v
	auto vlaplas = HMFem::LaplasProblem(*grid, lap);
	//dirichlet: 2*pi on razor_oi, zero on razor_io
	vlaplas.SetDirichlet(ExtractPoints(*grid, raz_io), [](const HM2D::Vertex*){return 0.0;});
	vlaplas.SetDirichlet(ExtractPoints(*grid, raz_oi), [](const HM2D::Vertex*){return 2.0*M_PI;});
	//solution
	v.resize(grid->vvert.size(), 0.0);
	vlaplas.Solve(v);

	//laplas problem for u
	auto ulaplas = HMFem::LaplasProblem(*grid, lap);
	//dirichlet: 1 on top, _module on bot
	ulaplas.SetDirichlet(ExtractPoints(*grid, outer), [](const HM2D::Vertex*){return 1;});
	ulaplas.SetDirichlet(ExtractPoints(*grid, inner), [&](const HM2D::Vertex*){return _module;});
	//solution
	u.resize(grid->vvert.size(), 0.0);
	ulaplas.Solve(u);
}

void ToAnnulus::BuildInverse(){
	//ugrid
	inv_grid = std::make_shared<HM2D::GridData>();
	DeepCopy(*grid, *inv_grid);
	for (int i=0; i<grid->vvert.size(); ++i){
		inv_grid->vvert[i]->set(u[i]*cos(v[i]), u[i]*sin(v[i]));
	}
	//approximator
	inv_approx = std::make_shared<HMFem::Grid43::Approximator>(
		inv_grid.get(), 40);
	//inverse functions
	inv_u.resize(inv_grid->vvert.size());
	inv_v.resize(inv_grid->vvert.size());
	for (int i=0; i<inv_u.size(); ++i){
		inv_u[i] = grid->vvert[i]->x;
		inv_v[i] = grid->vvert[i]->y;
	}
}

ToAnnulus::ToAnnulus(const vector<Point>& outer_path, const vector<Point>& inner_path,
		const Options& opt){
	pzero = inner_path[0];
	//1) Build a grid for modulus+cirlce computation
	BuildGrid1(outer_path, inner_path, opt.fem_nrec);
	//2) Compute u and modulus
	DoMappingU();
	//3) Build a steepest descent curve
	auto sd = SteepestDescentUCurve();
	//4) Build a grid for conjugate function solution
	BuildGrid2(outer_path, inner_path, opt.fem_nrec, sd);
	//5) Recompute u on singly connected grid, Compute v
	DoMapping();
	//7) Build inverse grid
	BuildInverse();
}

vector<Point> ToAnnulus::InnerCirclePoints() const{
	vector<Point> ret;
	for (int i=0; i<inner_origs.size(); ++i){
		int j = inner_origs[i];
		ret.push_back(Point(u[j], v[j]));
	}
	return ret;
}

vector<Point> ToAnnulus::OuterCirclePoints() const{
	vector<Point> ret;
	for (int i=0; i<outer_origs.size(); ++i){
		int j = outer_origs[i];
		ret.push_back(Point(u[j], v[j]));
	}
	return ret;
}

vector<Point> ToAnnulus::MapToAnnulus(const vector<Point>& input) const{
	vector<Point> ret;
	vector<const vector<double>*> funs {&u, &v};
	for (auto& p: input){
		auto s = approx->Vals(p, funs);
		double x = s[0]*cos(s[1]);
		double y = s[0]*sin(s[1]);
		ret.push_back(Point(x, y));
	}
	return ret;
}

vector<Point> ToAnnulus::MapToOriginal(const vector<Point>& input) const{
	vector<Point> ret;
	vector<const vector<double>*> funs {&inv_u, &inv_v};
	vector<double> s(2);
	for (auto& p: input){
		try{
			s = inv_approx->Vals(p, funs);
		} catch (HMFem::Grid43::Approximator::EOutOfArea& e){
			//if point was not found due to circle geom. approximation error
			//project point to inv_grid boundary and try again
			double rad = vecLen(p);
			Point pnew;
			if (fabs(rad-1.0)<geps || fabs(rad-_module)<geps){
				pnew = HM2D::Finder::ClosestEPoint(*InvGridContour(), p);
				s = inv_approx->Vals(pnew, funs);
			} else throw;
		}
		ret.push_back(Point(s[0], s[1]));
	}
	return ret;
}

double ToAnnulus::PhiInner(int i) const{
	return v[inner_origs[i]];
}
double ToAnnulus::PhiOuter(int i) const{
	return v[outer_origs[i]];
}

const HM2D::EdgeData* ToAnnulus::InvGridContour() const{
	if (!_inv_cont){
		_inv_cont = std::make_shared<HM2D::EdgeData>(
			HM2D::Contour::Assembler::GridBoundary1(*inv_grid));
	}
	return _inv_cont.get();
}

Point ToAnnulus::MapToAnnulusBnd(const Point& p) const{
	vector<double> s = approx->BndVals(p, {&u, &v});
	return Point(s[0]*cos(s[1]), s[0]*sin(s[1]));
}
Point ToAnnulus::MapToOriginalBnd(const Point& p) const{
	vector<double> s = inv_approx->BndVals(p, {&inv_u, &inv_v});
	return Point(s[0], s[1]);
}
