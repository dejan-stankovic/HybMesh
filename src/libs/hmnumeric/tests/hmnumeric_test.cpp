#include "hmfem.hpp"
#include "femassembly.hpp"
#include "hmtesting.hpp"
#include "assemble2d.hpp"
#include "buildcont.hpp"
#include "export2d_vtk.hpp"
#include "partcont.hpp"
#include "buildgrid.hpp"
#include "modgrid.hpp"
#include "buildcont.hpp"
#include "trigrid.hpp"
#include "finder2d.hpp"
#include "infogrid.hpp"
#include "laplace_bem2d.hpp"
#include "hmtimer.hpp"
#include "treverter2d.hpp"
#include "densemat.hpp"
using HMTesting::add_check;

double maxskew(const HM2D::GridData& g){
	auto s = HM2D::Grid::Skewness(g);
	return *max_element(s.begin(), s.end());
}

void test01(){
	std::cout<<"01. fem in square"<<std::endl;
	Point p1{0.0, 0.0}, p2{2.0, 0.0}, p3{2.0, 1.0}, p4{0.0, 1.0};

	//-> Dfdn(bottom), Dfdx(area), DfDy(area)
	auto dotest = [&](shared_ptr<HM2D::GridData> g)->std::array<double, 3>{
		auto cont = HM2D::Contour::Assembler::GridBoundary1(*g);
		auto _av = HM2D::AllVertices(cont);
		auto cp1 = _av[std::get<0>(HM2D::Finder::ClosestPoint(_av, p1))].get();
		auto cp2 = _av[std::get<0>(HM2D::Finder::ClosestPoint(_av, p2))].get();
		auto cp3 = _av[std::get<0>(HM2D::Finder::ClosestPoint(_av, p3))].get();
		auto cp4 = _av[std::get<0>(HM2D::Finder::ClosestPoint(_av, p4))].get();

		//HM2D::Vertex* gp1 = static_cast<HM2D::Vertex*>(HMCont2D::ECollection::Finder::ClosestPoint(cont, p1));
		//HM2D::Vertex* gp2 = static_cast<HM2D::Vertex*>(HMCont2D::ECollection::Finder::ClosestPoint(cont, p2));
		//HM2D::Vertex* gp3 = static_cast<HM2D::Vertex*>(HMCont2D::ECollection::Finder::ClosestPoint(cont, p3));
		//HM2D::Vertex* gp4 = static_cast<HM2D::Vertex*>(HMCont2D::ECollection::Finder::ClosestPoint(cont, p4));
		auto contbot = HM2D::Contour::Assembler::ShrinkContour(cont, cp1, cp2);
		auto conttop = HM2D::Contour::Assembler::ShrinkContour(cont, cp3, cp4);

		auto p = HMFem::LaplaceProblem(*g);
		p.SetDirichlet(contbot, [](const HM2D::Vertex* p){ return 1.0; });
		p.SetDirichlet(conttop, [](const HM2D::Vertex* p){ return 6.0; });


		auto v = vector<double>(g->vvert.size(), 0.0);
		p.Solve(v);

		double I1 = p.IntegralDfDn(contbot, v);

		auto mass = HMFem::Assemble::LumpMass(*g);

		vector<double> dx(g->vvert.size());
		HMFem::Assemble::DDx(*g)->MultVec(v, dx);
		for (int i=0; i<dx.size(); ++i) dx[i]/=mass[i];
		double I2 = std::inner_product(dx.begin(), dx.end(), mass.begin(), 0.0);

		vector<double> dy(g->vvert.size());
		HMFem::Assemble::DDy(*g)->MultVec(v, dy);
		for (int i=0; i<dy.size(); ++i) dy[i]/=mass[i];
		double I3 = std::inner_product(dy.begin(), dy.end(), mass.begin(), 0.0);

		return {I1, I2, I3};
	};

	//1) structured grid
	auto g4 = HM2D::Grid::Constructor::RectGrid01(21, 21);
	HM2D::Grid::Algos::CutCellDims(g4, 3);
	for (auto v: g4.vvert) v->x*=2;
	auto g1 = std::make_shared<HM2D::GridData>(g4);
	auto I1 = dotest(g1);
	add_check(fabs(I1[0]+10)<1e-6, "structured triangle grid, DfDn");
	add_check(fabs(I1[1])<1e-6, "structured triangle grid, DfDx");
	add_check(fabs(I1[2]-10)<1e-6, "structured triangle grid, DfDy");

	//2) unstructured grid
	double h = 0.05;
	auto cont = HM2D::Contour::Constructor::FromPoints({p1, p2, p3, p4}, true);
	auto tree = HM2D::Mesher::PrepareSource(cont, h);
	auto g2 = std::make_shared<HM2D::GridData>(HM2D::Mesher::UnstructuredTriangle(tree));
	auto I2 = dotest(g2);
	add_check(fabs(I2[0]+10)<1e-6, "unstructured triangle grid, DfDn");
	add_check(fabs(I2[1])<1e-6, "unstructured triangle grid, DfDx");
	add_check(fabs(I2[2]-10)<1e-6, "unstructured triangle grid, DfDy");
}

void test02(){
	std::cout<<"02. Auxiliary triangulation"<<std::endl;
	{
		//Rectangle checks
		auto rcont = HM2D::Contour::Constructor::FromPoints({0,0, 10,0, 10,10, 0,10}, true);
		auto ans1 = HMFem::AuxGrid3(rcont, 200, 200000);
		add_check(abs((int)ans1.vvert.size()-200)<0.1*200, "coarse grid in rectangle");

		auto ans3 = HMFem::AuxGrid3(rcont, 10000, 200000);
		add_check(abs((int)ans3.vvert.size()-10000)<0.1*10000, "fine grid in rectangle");
	}
	{
		//ring checks
		auto rgrid = HM2D::Grid::Constructor::Ring(Point(0, 0), 1, 0.7, 16, 1);
		auto rcont = HM2D::Contour::Tree::GridBoundary(rgrid);
		HM2D::GridData ans1 = HMFem::AuxGrid3(rcont, 5000, 1000000);
		add_check(abs((int)ans1.vvert.size()-5000)<0.1*5000, "grid in the ring area");
	}
	{
		//grid with coarsening needed
		auto rcont = HM2D::Contour::Constructor::Circle(400, 1.0, Point(0, 0));
		HM2D::GridData ans1 = HMFem::AuxGrid3(rcont, 1000, 1000000);
		add_check(abs((int)ans1.vvert.size()-1000)<0.5*1000, "grid in fine circle contour");
	}
	{
		auto rcont2 = HM2D::Contour::Constructor::FromPoints({
			0.0,0.0, 3.0,0.0,   3.0,1.0,   2.5,1.0, 2.47,1.04, 2.43,1,
			1.6,1.0, 1.57,1.04, 1.55,0.99, 0.0,1.0}, true);
		std::map<double, double> w;
		w[0] = 0.02; w[0.5] = 0.4;
		HM2D::Export::ContourVTK(rcont2, "c1.vtk");
		auto rcont = HM2D::Contour::Algos::WeightedPartition(w, rcont2,
				HM2D::Contour::Algos::PartitionTp::KEEP_ALL);
		HM2D::Export::ContourVTK(rcont, "c1.vtk");
		HM2D::GridData ans1 = HMFem::AuxGrid3(rcont, 500, 1000000);
		HM2D::Export::GridVTK(ans1, "g1.vtk");
		add_check(abs((int)ans1.vvert.size()-500)<500, "grid with fine regions in its source tree");
	}
}

void test03(){
	std::cout<<"03. Auxiliary triangulation with contour constrains"<<std::endl;
	{
		auto sqrcont = HM2D::Contour::Constructor::Circle(32, 1.0, Point(0, 0));
		auto constr = HM2D::Contour::Constructor::FromPoints({
				-0.7,0, -0.69,0, -0.68,0, -0.67,0, -0.3,0, 0.1,0.4, 0.6,-0.3});
		HM2D::Contour::Tree tree;
		tree.add_contour(sqrcont);
		HM2D::GridData ans1 = HMFem::AuxGrid3(tree, vector<HM2D::EdgeData> {constr}, 1000, 1000000);
		add_check([&](){
			auto fpoint = [&](double x, double y){
				Point pp(x, y);
				for (int i=0; i<ans1.vvert.size(); ++i)
					if (*ans1.vvert[i] == pp) return true;
				return false;
			};
			return (fpoint(-0.7, 0) && fpoint(-0.69, 0) && fpoint(-0.68,0) && fpoint(-0.67,0)
				&& fpoint(-0.3, 0) && ans1.vvert.size() < 1200);
		}(), "non-touching constraint");
		add_check(maxskew(ans1)<0.95, "non-touching, skewness check");
		HM2D::Export::GridVTK(ans1, "g1.vtk");
	}
	{
		auto sqrcont = HM2D::Contour::Constructor::FromPoints({0,0, 1,0, 1,1, 0,1}, true);
		std::map<double, double> w;
		w[0.133] = 0.01; w[0.5] = 0.1;
		auto sqrcont2 = HM2D::Contour::Algos::WeightedPartition(w, sqrcont);
		auto lineconst = HM2D::Contour::Constructor::FromPoints({0.5,0, 0.2,0.5});
		HM2D::Contour::Tree tree;
		tree.add_contour(sqrcont2);
		HM2D::GridData ans1 = HMFem::AuxGrid3(tree,
				vector<HM2D::EdgeData> {lineconst}, 500, 100000);
		add_check(ans1.vvert.size() < 700 && ans1.vcells.size() < 1200 &&
			[&](){
				Point p1(0.5, 0), p2(0.2, 0.5);
				double ksieta[2];
				for (auto e: ans1.vedges){
					Point p3 = *e->first();
					Point p4 = *e->last();
					if (SectCross(p1, p2, p3, p4, ksieta)){
						if (ksieta[1]>geps && ksieta[1]<1-geps) return false;
					}
				}
				return true;
			}()
			, "touching coarse constraint");
		add_check(maxskew(ans1)<0.95, "coarse, touching, skewness check");
	}
	{
		auto sqrcont = HM2D::Contour::Constructor::FromPoints({0,0, 1,0, 1,1, 0,1}, true);
		std::map<double, double> w;
		w[0.133] = 0.01; w[0.5] = 0.2;
		auto sqrcont2 = HM2D::Contour::Algos::WeightedPartition(w, sqrcont);
		auto lineconst = HM2D::Contour::Constructor::FromPoints({0.5,0, 0.2,0.5, 1,0.2});
		auto lineconst2 = HM2D::Contour::Algos::Partition(0.01, lineconst, HM2D::Contour::Algos::PartitionTp::KEEP_ALL);
		HM2D::Contour::Tree tree;
		tree.add_contour(sqrcont2);
		HM2D::GridData ans1 = HMFem::AuxGrid3(tree,
				vector<HM2D::EdgeData> {lineconst2}, 500, 100000);
		add_check(ans1.vvert.size() < 800, "touching fine constraint");
		add_check(maxskew(ans1)<0.95, "fine, touching, skewness check");
		HM2D::Export::GridVTK(ans1, "g1.vtk");
	}
};

void test04(){
	std::cout<<"03. Bem with constant elements"<<std::endl;
	{
		HMMath::DenseMat mat(3);
		mat.val(0, 0) = 7; mat.val(0, 1) = 2; mat.val(0, 2) = 3;
		mat.val(1, 0) = 3; mat.val(1, 1) = 4; mat.val(1, 2) = 1;
		mat.val(2, 0) = 1; mat.val(2, 1) = 3; mat.val(2, 2) = 9;
		std::vector<double> rhs {0, 1, 2}, ans {0, 0, 0};
		HMMath::DenseSolver slv(mat);
		slv.Solve(rhs, ans);
		add_check(fabs(mat.RowMultVec(ans, 0)-rhs[0])<1e-12 &&
			fabs(mat.RowMultVec(ans, 1)-rhs[1])<1e-12 &&
			fabs(mat.RowMultVec(ans, 2)-rhs[2])<1e-12, "dense matrix solver");
	}
	{
		double step = 0.05;
		auto c1 = HM2D::Contour::Constructor::Rectangle(Point(0,0), Point(2, 1));
		auto c2 = HM2D::Contour::Constructor::Circle(16, 0.2, Point(1.3, 0.6));
		HM2D::Contour::R::ReallyDirect::Permanent(c1);
		HM2D::Contour::R::ReallyRevert::Permanent(c2);

		c1 = HM2D::Contour::Algos::Partition(step, c1, HM2D::Contour::Algos::PartitionTp::KEEP_SHAPE);
		c2 = HM2D::Contour::Algos::Partition(step, c2, HM2D::Contour::Algos::PartitionTp::KEEP_SHAPE);

		for (auto e: c1) e->boundary_type = 1;
		for (auto e: c2) e->boundary_type = 2;

		HM2D::Contour::Tree t1;
		t1.add_contour(c1);
		t1.add_contour(c2);

		HM2D::GridData g3 = HM2D::Mesher::UnstructuredTriangle(t1);

		HMBem::LaplaceCE2D bemslv(t1);
		int it = 0;
		for (auto e: t1.alledges()){
			double dv = e->boundary_type == 1 ? 1 : 2;
			bemslv.dirichlet_value(it++, dv);
		}
		bemslv.Solve();

		double integral = 0.0;
		for (auto& c: g3.vcells){
			Point cnt(0.0, 0.0);
			for (auto p: HM2D::AllVertices(c->edges)) cnt += *p;
			cnt /= 3;
			double v = bemslv.internal_value_at(cnt.x, cnt.y);
			integral += HM2D::Contour::Area(c->edges)*v;
		}
		add_check(fabs(integral - 2.28922)<1e-2, "bem for dirichlet poisson problem");
	}
};

int main(){
	test01();
	test02();
	test03();
	test04();


	HMTesting::check_final_report();
	std::cout<<"DONE"<<std::endl;
}
