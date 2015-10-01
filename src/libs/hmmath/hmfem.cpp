#include "hmfem.hpp"
#include "femassembly.hpp"

using namespace HMFem;

LaplasProblem::LaplasProblem(GridGeom* g):
		grid(Grid43::Build(g)),
		laplas_mat(HMFem::Assemble::PureLaplas(*grid)),
		solution_mat(),
		rhs(grid->n_points(), 0.0){}

LaplasProblem::LaplasProblem(shared_ptr<Grid43> g):
		grid(g),
		laplas_mat(HMFem::Assemble::PureLaplas(*grid)),
		solution_mat(),
		rhs(grid->n_points(), 0.0){}

LaplasProblem::LaplasProblem(shared_ptr<Grid43> g, shared_ptr<Mat> lap):
		grid(g), laplas_mat(lap),
		solution_mat(),
		rhs(grid->n_points(), 0.0){}

void LaplasProblem::SetDirichlet(const vector<const GridPoint*>& pts, TDirFunc f){
	_dirfunc.push_back(f);
	for (auto p: pts){
		TDirData dt {p, &_dirfunc.back()};
		dirichlet_data.insert(dt);
	}
}
void LaplasProblem::SetDirichlet(const HMCont2D::Contour& pts, TDirFunc f){
	vector<const GridPoint*> pts2; pts2.reserve(pts.size());
	for(const Point* p: pts.ordered_points())
		pts2.push_back(
			static_cast<const GridPoint*>(p)
		);
	SetDirichlet(pts2, f);
}

void LaplasProblem::SetNeumann(const vector<const GridPoint*>& pts, TNeuFunc f){
	_THROW_NOT_IMP_;
}

void LaplasProblem::RebuildSolutionMatrix(){
	std::fill(rhs.begin(), rhs.end(), 0.0);
	solution_mat = *laplas_mat;

	//Neumann
	for (auto& nc: neumann_data){
		//val = (df/dn)*L/2;
		double val = ((*nc.fun)(nc.point1, nc.point2)) * nc.dist / 2;
		rhs[nc.point1->get_ind()] += val;
		rhs[nc.point2->get_ind()] += val;
	}

	//Dirichlet. Strictly after Neumann
	for (auto& dc: dirichlet_data){
		//put 1 to diagonal and value to rhs
		int ind = dc.point->get_ind();
		double val = (*dc.fun)(dc.point);
		solution_mat.clear_row(ind);
		solution_mat.set(ind, ind, 1.0);
		rhs[ind] = val;
	}

	//solver initialization
	solver.Init(solution_mat);
}


//solve Ax=0
void LaplasProblem::Solve(vector<double>& ans){
	RebuildSolutionMatrix();
	QuickSolve(ans);
}


void LaplasProblem::QuickSolve(vector<double>& ans){
	solver.Solve(rhs, ans);
}

double LaplasProblem::IntegralDfDn(const vector<const GridPoint*>& pnt,
		const vector<double>& f){
	//we assume that pnt are ordered in such a way that
	//each pair <pnt[i], pnt[i+1]> forms an edge
	assert(pnt.size()>1 && pnt[0] != pnt.back());
	assert( [&](){
		auto tree = GGeom::Info::Contour(*grid);
		shared_ptr<HMCont2D::ContourTree::TreeNode> c;
		for (int i=0; i<tree.cont_count(); ++i){
			if (tree.nodes[i]->contains_point(pnt[0])){
				c = tree.nodes[i]; 
				break;
			}
		}
		if (!c) return false;
		auto pp = c->ordered_points(); pp.pop_back();
		auto fnd = std::find(pp.begin(), pp.end(), pnt[0]);
		//choose forward or backward
		std::rotate(pp.begin(), fnd, pp.end());
		if (pnt[1] != pp[1]){
		//try backward
			std::reverse(pp.begin(), pp.end());
			std::rotate(pp.begin(), pp.end()-1, pp.end());
		}
		for (int i=0; i<pnt.size(); ++i){
			if (pnt[i] != pp[i]) return false;
		}
		return true;
	}());
	
	//assemble pure rhs
	vector<double> r(pnt.size(), 0.0);
	for (int i=0; i<pnt.size(); ++i){
		const GridPoint* p1 = pnt[i];
		r[i] = laplas_mat->RowMultVec(f, p1->get_ind());
	}
	//add neumann condition to first and last rhs entries
	//if it exists
	if (neumann_data.size()>0){
		_THROW_NOT_IMP_;
	}
	
	//find |L|*k*dfdn using simple elimination
	vector<double> lk_dfdn(pnt.size()-1, 0.0);
	lk_dfdn[0] = 2*r[0];
	for (int i=1; i<pnt.size()-1; ++i){
		lk_dfdn[i] = 2*r[i] - lk_dfdn[i-1];
	}

	//return sum
	return std::accumulate(lk_dfdn.begin(), lk_dfdn.end(), 0.0);
}

double LaplasProblem::IntegralDfDn(const HMCont2D::Contour& pnt, const vector<double>& f){
	vector<const GridPoint*> pts2; pts2.reserve(pnt.size());
	for(const Point* p: pnt.ordered_points())
		pts2.push_back(
			static_cast<const GridPoint*>(p)
		);
	return IntegralDfDn(pts2, f);
}


