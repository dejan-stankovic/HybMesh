#include "inscribe_grid.hpp"
#include "infogrid.hpp"
#include "healgrid.hpp"
#include "clipdomain.hpp"
#include "trigrid.hpp"
#include "modgrid.hpp"
#include "assemble2d.hpp"
#include "treverter2d.hpp"
#include "finder2d.hpp"
#include "modcont.hpp"
#include "hmtimer.hpp"
#include "buildcont.hpp"
#include "sizefun.hpp"
#include "wireframegrid.hpp"

using namespace HM2D;
using namespace HM2D::Grid;

HMCallback::FunctionWithCallback<Algos::TSubstractCells> Algos::SubstractCells;
HMCallback::FunctionWithCallback<Algos::TInscribeGrid> Algos::InscribeGrid;
HMCallback::FunctionWithCallback<Algos::TInsertConstraints> Algos::InsertConstraints;
HMCallback::FunctionWithCallback<Algos::TSubstractArea> Algos::SubstractArea;

GridData Algos::TSubstractCells::_run(const GridData& base, const Contour::Tree& cont, SubstractCellsAlgo algo){
	auto invert_extraction = [&](const CellData& extr)->CellData{
		aa::constant_ids_pvec(base.vcells, 0);
		aa::constant_ids_pvec(extr, 1);
		CellData ret;
		std::copy_if(base.vcells.begin(), base.vcells.end(), std::back_inserter(ret),
				[](shared_ptr<Cell> c){ return c->id == 0; });
		return ret;
	};

	CellData goodcells;

	auto cb1 = callback->bottom_line_subrange(95);
	switch (algo){
	case SubstractCellsAlgo::FULLY_INSIDE:
	{
		CellData badcells = ExtractCells.WithCallback(cb1, base, cont, INSIDE);
		goodcells = invert_extraction(badcells);
		break;
	}
	case SubstractCellsAlgo::FULLY_OUTSIDE:
	{
		CellData badcells = ExtractCells.WithCallback(cb1, base, cont, OUTSIDE);
		goodcells = invert_extraction(badcells);
		break;
	}
	case SubstractCellsAlgo::PARTLY_INSIDE:
	{
		goodcells = ExtractCells.WithCallback(cb1, base, cont, OUTSIDE);
		break;
	}
	case SubstractCellsAlgo::PARTLY_OUTSIDE:
	{
		goodcells = ExtractCells.WithCallback(cb1, base, cont, INSIDE);
		break;
	}
	case SubstractCellsAlgo::CROSS:
	{
		goodcells = ExtractCells.WithCallback(cb1, base, cont, BOUND);
		break;
	}
	case SubstractCellsAlgo::NO_CROSS:
	{
		CellData badcells = ExtractCells.WithCallback(cb1, base, cont, BOUND);
		goodcells = invert_extraction(badcells);
		break;
	}
	}

	callback->step_after(5, "Assembling grid");
	GridData ret;
	DeepCopy(goodcells, ret.vcells, 2);
	RestoreFromCells(ret);

	return ret;
}

namespace{

void id1_to_significant_prims(Contour::Tree& triarea, const EdgeData& keep_edges,
		const VertexData& keep_vert, double angle0){
	auto ae = triarea.alledges();
	auto av = HM2D::AllVertices(ae);

	//Significant edges
	EdgeData sig_edges;
	auto bb = HM2D::BBox(HM2D::AllVertices(keep_edges));
	Finder::RasterizeEdges rast(keep_edges, bb, bb.maxlen()/30);
	aa::constant_ids_pvec(av, 0);
	double ksi;
	for (auto v: av){
		for (int isusp: rast.bbfinder().suspects(*v)){
			auto esusp = keep_edges[isusp];
			if (isOnSection(*v, *esusp->pfirst(), *esusp->plast(), ksi)){
				v->id = 1;
				break;
			}
		}
	}
	for (auto e: ae) if (e->pfirst()->id == 1 && e->plast()->id == 1){
		Point pmid = Point::Weigh(*e->pfirst(), *e->plast(), 0.5);
		for (int isusp: rast.bbfinder().suspects(pmid)){
			auto esusp = keep_edges[isusp];
			if (isOnSection(pmid, *esusp->pfirst(), *esusp->plast(), ksi)){
				sig_edges.push_back(e);
				break;
			}
		}
	}

	//Significant vertices
	VertexData sig_vertices;
	if (angle0 >= 0){
		EdgeData nonsig_edges = ae;
		aa::constant_ids_pvec(nonsig_edges, 0);
		aa::constant_ids_pvec(sig_edges, 1);
		aa::keep_by_id(nonsig_edges, 0);
		for (auto c: Contour::Assembler::SimpleContours(nonsig_edges)){
			for (auto v: AllVertices(ECol::Algos::Simplified(c, angle0, false, keep_vert))){
				sig_vertices.push_back(v);
			}
		}
	} else sig_vertices = av;

	//do segmentation using id=1 to keep needed primitives
	aa::constant_ids_pvec(av, 0);
	aa::constant_ids_pvec(ae, 0);
	aa::constant_ids_pvec(sig_vertices, 1);
	aa::constant_ids_pvec(sig_edges, 1);
}

VertexData add_tree_crosses(HM2D::Contour::Tree& tree){
	VertexData ret;
	int n = tree.nodes.size();
	for (int i=0; i<n; ++i)
	for (int j=i+1; j<n; ++j){
		if (tree.nodes[i]->isbound() && tree.nodes[j]->isbound()) continue;
		auto& c1 = tree.nodes[i]->contour;
		auto& c2 = tree.nodes[j]->contour;
		auto crosses = HM2D::Contour::Finder::CrossAll(c1, c2);
		for (auto& c: crosses){
			auto res1 = HM2D::Contour::Algos::GuaranteePoint(c1, std::get<1>(c));
			auto res2 = HM2D::Contour::Algos::GuaranteePoint(c2, std::get<1>(c));
			ret.push_back(std::get<1>(res1));
			ret.push_back(std::get<1>(res2));
		}
	}
	return ret;
}

}

GridData Algos::TInscribeGrid::_run(const GridData& base, const Contour::Tree& cont2,
		OptInscribe opt){
	Contour::Tree cont = cont2; cont.remove_detached();
	//1) Throw away cells which are outside cont
	auto cb1 = callback->bottom_line_subrange(20);
	auto g1algo = opt.inside ? Algos::SubstractCellsAlgo::PARTLY_OUTSIDE
	                         : Algos::SubstractCellsAlgo::PARTLY_INSIDE;
	GridData g1 = SubstractCells.WithCallback(cb1, base, cont, g1algo);

	//2) offsetting
	//delta >> geps to avoid geometry errors
	callback->step_after(10, "Offset");
	opt.buffer_size = std::max(1e3*geps, opt.buffer_size);
	Contour::Tree bzone;
	for (auto& c: cont.bound_contours()){
		Contour::R::Clockwise rc(c->contour, false);
		double delta = opt.buffer_size;
		if (opt.inside) delta*=-1;
		if (c->isinner()) delta*=-1;
		Contour::Tree t1 = Contour::Algos::Offset(
			c->contour,
			delta,
			Contour::Algos::OffsetTp::RC_CLOSED_POLY);
		t1.add_contour(c->contour);
		bzone = Contour::Clip::Union(bzone, t1);
	}

	//3) Throw away cells within the buffer
	auto cb3 = callback->bottom_line_subrange(20);
	GridData g2 = SubstractCells.WithCallback(cb3, g1, bzone, Algos::SubstractCellsAlgo::PARTLY_INSIDE);
	if (opt.fillalgo == 99) return g2;

	//4) Assemble triangulation area
	callback->step_after(10, "Triangulation area");
	Contour::Tree triarea = Contour::Tree::DeepCopy(cont);
	for (auto c: Contour::Assembler::GridBoundary(g2)){
		triarea.add_contour(c);
	}
	if (!opt.inside){
		triarea = Contour::Clip::Difference(Contour::Tree::GridBoundary(base), triarea);
	}

	//5) 1D segmentation of triangulation area
	EdgeData keep_edges = ECol::Assembler::GridBoundary(g2);
	if (opt.keep_cont){
		for (auto e: cont.alledges()) keep_edges.push_back(e);
	}
	//sources
	id1_to_significant_prims(triarea, keep_edges, {}, opt.angle0);
	ApplySizeFunction(triarea);
	
	//6) Triangulation
	auto cb6 = callback->bottom_line_subrange(20);
	GridData g3;
	if (opt.fillalgo == 0) g3 = Mesher::UnstructuredTriangle.WithCallback(cb6, triarea);
	else if (opt.fillalgo == 1) g3 = Mesher::UnstructuredTriangleRecomb.WithCallback(cb6, triarea);

	//7) Merging
	callback->step_after(10, "Merging");
	Grid::Algos::MergeTo(g2, g3);
	
	return g3;
}

GridData Algos::TInsertConstraints::_run(const GridData& base,
		const vector<EdgeData>& cont,
		const vector<std::pair<Point, double>>& pnt,
		Algos::OptInsertConstraints opt){
	//offsetting from polylines
	callback->step_after(10, "Offset");
	opt.buffer_size = std::max(1e3*geps, opt.buffer_size);
	Contour::Tree bzone;
	for (auto& c: cont){
		Contour::R::Clockwise rc(c, false);
		Contour::Tree t1 = Contour::Algos::Offset(
			c, opt.buffer_size,
			Contour::Algos::OffsetTp::RC_OPEN_ROUND);
		bzone = Contour::Clip::Union(t1, bzone);
	}
	//offsetting from points
	for (auto& p: pnt){
		auto crc = Contour::Constructor::Circle(32, opt.buffer_size, p.first);
		Contour::Tree t1; t1.add_contour(crc);
		bzone = Contour::Clip::Union(t1, bzone);
	}

	//remove cells under the offset
	auto cb1 = callback->bottom_line_subrange(20);
	CellData needed_cells = ExtractCells.WithCallback(cb1, base, bzone, OUTSIDE);
	aa::constant_ids_pvec(base.vcells, 0);
	aa::constant_ids_pvec(needed_cells, 1);
	CellData not_needed_cells;
	not_needed_cells.reserve(base.vcells.size() - needed_cells.size());
	for (auto& c: base.vcells) if (c->id==0) not_needed_cells.push_back(c);
	GridData g2, g3;
	DeepCopy(needed_cells, g2.vcells);
	RestoreFromCells(g2);
	if (opt.fillalgo == 99) return g2;
	DeepCopy(not_needed_cells, g3.vcells);
	RestoreFromCells(g3);

	//triangulation area
	callback->step_after(35, "Assemble buffer area", 3, 1);
	Contour::Tree triarea = Contour::Tree::GridBoundary(g3);
	for (auto& c: cont) triarea.add_detached_contour(c);
	//paste crosses
	VertexData cr_points = add_tree_crosses(triarea);
	//id=1 to edges and vertices which should be kept
	if (opt.keep_cont) aa::constant_ids_pvec(triarea.alledges(), 1);
	else { id1_to_significant_prims(triarea, ECol::Assembler::GridBoundary(g2), cr_points, opt.angle0); }
	for (auto& v: cr_points) v->id = 1;
	//build point constraints and check if we will need explicit size function in future.
	bool force_sizefun = false;
	vector<std::pair<Point, double>> src_tri;
	for (auto& p: pnt) {
		if (p.second > 0) src_tri.push_back(p);
		else { force_sizefun = true; }
	}
	//generate 1D mesh
	callback->subprocess_step_after(2);
	auto sfun = ApplySizeFunction(triarea, src_tri, force_sizefun);

	//calculate point constraint sizes if they were not given using sfun.
	CoordinateMap2D<double> pnt2;
	for (auto& p: pnt){
		double v = p.second;
		if (v <= 0) v = sfun->sz(p.first);
		pnt2.add(p.first, v);
	}

	//Triangulation
	auto cb2 = callback->bottom_line_subrange(30);
	GridData g4;
	if (opt.fillalgo == 0) g4 = Mesher::UnstructuredTriangle.WithCallback(cb2, triarea, pnt2);
	else if (opt.fillalgo == 1) g4 = Mesher::UnstructuredTriangleRecomb.WithCallback(cb2, triarea, pnt2);
	
	//Merge
	callback->step_after(5, "Merge");
	Grid::Algos::MergeTo(g2, g4);

	return  g4;
}

GridData Algos::TInsertConstraints::_run(const GridData& base,
		const vector<EdgeData>& cont,
		Algos::OptInsertConstraints opt){
	return _run(base, cont, vector<std::pair<Point, double>>(), opt);
}

GridData Algos::TInsertConstraints::_run(const GridData& base,
		const vector<std::pair<Point, double>>& pnt,
		Algos::OptInsertConstraints opt){
	return _run(base, {}, pnt, opt);
}

namespace{

vector<Contour::Tree> split_tree(const Contour::Tree& area, const GridData& g, bool is_inner){
	Contour::Tree rt = Contour::Tree::DeepCopy(area, 1);
	rt.remove_detached();
	//force is_inner=false by addition new contour to c
	if (is_inner == true){
		//auto bbox = HM2D::BBox(g.vvert, 1.);
		auto bbox = HM2D::BBox(area.alledges(), 1.);
		for (auto& v: g.vvert) bbox.widen(*v);
		EdgeData bcont = HM2D::Contour::Constructor::FromPoints(
			{bbox.xmin,bbox.ymin, bbox.xmax,bbox.ymin,
			 bbox.xmax,bbox.ymax, bbox.xmin,bbox.ymax}, true);
		rt.add_detached_contour(bcont);
		for (auto& n: rt.roots()){
			n->parent = rt.nodes.back();
			rt.nodes.back()->children.push_back(n);
		}
		for (auto& n: rt.nodes) n->level+=1;
	}
	//crop
	vector<Contour::Tree> ret = Contour::Tree::CropLevel01(rt);

	return ret;
}

}
//##########################################################
GridData Algos::TSubstractArea::_run(const GridData& ginp, const Contour::Tree& area, bool is_inner){
	//temporary remove those which will not be affected by substraction
	CellData not_needed = ExtractCells(ginp, area, is_inner ? INSIDE : OUTSIDE);
	CellData needed = ExtractCells(ginp, area, is_inner ? OUTSIDE : INSIDE);
	aa::constant_ids_pvec(ginp.vcells, 0);
	aa::constant_ids_pvec(needed, 1);
	aa::constant_ids_pvec(not_needed, 1);
	CellData ambigious = aa::copy_by_id(ginp.vcells, 0);

	GridData gamb;
	HM2D::DeepCopy(ambigious, gamb.vcells, 2);
	RestoreFromCells(gamb);
	GridData gneeded;
	HM2D::DeepCopy(needed, gneeded.vcells, 2);
	RestoreFromCells(gneeded);


	callback->step_after(5, "Domains processing");
	Contour::Tree gcont = Contour::Tree::GridBoundary(gamb);
	vector<Contour::Tree> tarea = split_tree(area, gamb, is_inner);
	
	vector<GridData> gg;
	callback->step_after(85, "Substraction", tarea.size()*7);
	for (int i=0; i<tarea.size(); ++i){
		//building a graph
		callback->subprocess_step_after(1);
		Impl::PtsGraph graph(gamb);

		//add contour edges
		callback->subprocess_step_after(1);
		for (auto& n: tarea[i].nodes){
			graph.add_edges(n->contour);
		}

		//exclude edges
		callback->subprocess_step_after(1);
		graph.exclude_area(gcont, OUTSIDE);

		callback->subprocess_step_after(1);
		graph.exclude_area(tarea[i], OUTSIDE);

		callback->subprocess_step_after(1);
		gg.push_back(graph.togrid());
	
		//remove elements which can present in the grid due
		//to outer contour exclusion
		callback->subprocess_step_after(1);
		if (tarea[i].nodes.size()>1) Algos::RemoveCells(gg.back(), tarea[i], OUTSIDE);

		callback->subprocess_step_after(1);
		if (gcont.nodes.size()>1) Algos::RemoveCells(gg.back(), gcont, OUTSIDE);
	}
	callback->subprocess_fin();

	//assembling the result
	callback->step_after(5, "Assembling the result");
	for (int i=1; i<gg.size(); ++i){
		Algos::ShallowAdd(gg[i], gg[0]);
	}
	Algos::MergeTo(gneeded, gg[0]);

	callback->step_after(5, "Assign boundary types");
	//place area boundary before grid boundary to
	//guarantee higher priority of area edges.
	EdgeData cd = area.alledges_bound();
	EdgeData gd = HM2D::ECol::Assembler::GridBoundary(ginp);
	cd.insert(cd.end(), gd.begin(), gd.end());
	EdgeData rd = HM2D::ECol::Assembler::GridBoundary(gg[0]);
	HM2D::ECol::Algos::AssignBTypes(cd, rd);

	return gg[0];
}

/*
GridData Algos::TSubstractArea::_run(const GridData& g1, const Contour::Tree& area, bool is_inner){
	callback->step_after(5, "Domains processing");
	Contour::Tree gcont = Contour::Tree::GridBoundary(g1);
	vector<Contour::Tree> tarea = split_tree(area, g1, is_inner);
	
	vector<GridData> gg;
	callback->step_after(85, "Substraction", tarea.size()*7);
	for (int i=0; i<tarea.size(); ++i){
		//building a graph
		callback->subprocess_step_after(1);
		Impl::PtsGraph graph(g1);

		//add contour edges
		callback->subprocess_step_after(1);
		for (auto& n: tarea[i].nodes){
			graph.add_edges(n->contour);
		}

		//exclude edges
		callback->subprocess_step_after(1);
		graph.exclude_area(gcont, OUTSIDE);

		callback->subprocess_step_after(1);
		graph.exclude_area(tarea[i], OUTSIDE);

		callback->subprocess_step_after(1);
		gg.push_back(graph.togrid());
	
		//remove elements which can present in the grid due
		//to outer contour exclusion
		callback->subprocess_step_after(1);
		if (tarea[i].nodes.size()>1) Algos::RemoveCells(gg.back(), tarea[i], OUTSIDE);

		callback->subprocess_step_after(1);
		Algos::RemoveCells(gg.back(), gcont, OUTSIDE);
	}
	callback->subprocess_fin();

	//assembling the result
	callback->step_after(5, "Assembling the result");
	for (int i=1; i<gg.size(); ++i){
		Algos::ShallowAdd(gg[i], gg[0]);
	}

	callback->step_after(5, "Assign boundary types");
	//place area boundary before grid boundary to
	//guarantee higher priority of area edges.
	EdgeData cd = area.alledges_bound();
	EdgeData gd = HM2D::ECol::Assembler::GridBoundary(g1);
	cd.insert(cd.end(), gd.begin(), gd.end());
	EdgeData rd = HM2D::ECol::Assembler::GridBoundary(gg[0]);
	HM2D::ECol::Algos::AssignBTypes(cd, rd);
	return gg[0];
}
*/