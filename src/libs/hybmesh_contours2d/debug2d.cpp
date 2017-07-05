#ifndef NDEBUG

#include "debug2d.hpp"
#include "buildcont.hpp"
#include <fstream>
#include "export2d_vtk.hpp"
#include "modcont.hpp"

using namespace HM2D;

void Debug::info_cell(const GridData& g, int i){
	aa::RestoreIds<CellData> r1(g.vcells);
	aa::RestoreIds<EdgeData> r2(g.vedges);
	aa::RestoreIds<VertexData> r3(g.vvert);
	numer_all(g);
	auto c = g.vcells[i];
	std::cout<<"Cell "<<i<<" edge list:"<<std::endl;
	for (int j=0; j<c->edges.size(); ++j){
		std::cout<<j<<")\t";
		std::cout<<c->edges[j]->id<<" - ";
		std::cout<<c->edges[j].get()<<"; cell neighbours: ";
		if (c->edges[j]->has_left_cell())
			std::cout<<c->edges[j]->left.lock()->id<<" - ";
		else
			std::cout<<"no - ";
		if (c->edges[j]->has_right_cell())
			std::cout<<c->edges[j]->right.lock()->id;
		else
			std::cout<<"no";
		std::cout<<"; vertex neighbours: ";
		std::cout<<c->edges[j]->vertices[0]->id<<" - ";
		std::cout<<c->edges[j]->vertices[1]->id<<" ";
		if (HM2D::Contour::CorrectlyDirectedEdge(c->edges, j)){
			std::cout<<"  direct;"<<std::endl;
		} else {
			std::cout<<"  reversed;"<<std::endl;
		}
	}
	std::cout<<"Cell "<<i<<" vertex list:"<<std::endl;
	int j=0;
	for (auto& v: HM2D::Contour::OrderedPoints1(c->edges)){
		std::cout<<j<<")\t"<<v->id<<" - "<<v.get()<<std::endl;
		++j;
	}
}

void Debug::numer_all(const GridData& g){
	aa::enumerate_ids_pvec(g.vcells);
	aa::enumerate_ids_pvec(g.vedges);
	aa::enumerate_ids_pvec(g.vvert);
}

void Debug::info_contour(const EdgeData& c){
	Cout()<<"+++ EdgeData at "<<&c<<". "<<c.size()<<" edges. ";
	if (Contour::IsContour(c)){
		if (Contour::IsClosed(c)) Cout()<<"Closed contour. "<<"Area = "<<Contour::Area(c)<<std::endl;
		else Cout()<<"Open contour"<<std::endl;
	} else {
		Cout()<<"Shattered edges."<<std::endl;
	}
	Cout()<<"+++ Length = "<<Contour::Length(c)<<std::endl;
	int i = 0;
	for (auto e: c){
		Print("%i: Points %p -- %p, (%10.6f, %10.6f) -- (%10.6f, %10.6f) --> Edge %p\n",
			i,
			e->first().get(), e->last().get(), e->first()->x, e->first()->y,
			e->last()->x, e->last()->y, e.get());
		++i;
	}
	Cout()<<"+++++++++++++++++++++++++++++++++++++"<<std::endl;
}

void Debug::info_tree(const Contour::Tree& c){
	Print("+++ ContourTree at %p. %i Contours. Area = %f.\n", &c, c.nodes.size(),
			c.area());
	tabs += 1;
	for (auto r: c.roots()){
		info_tree_node(*r);
	}
	tabs -= 1;
	Cout()<<"+++++++++++++++++++++++++++++++++++++"<<std::endl;
}


void Debug::info_tree_node(const Contour::Tree::TNode& c){
	Print("+++ TreeNode at %p. Parent = %p. %i Children\n", &c, c.parent.lock().get(), c.children.size());
	info_contour(c.contour);
	tabs += 1;
	for (auto c2: c.children){
		info_tree_node(*c2.lock());
	}
	tabs -= 1;
	Cout()<<"+++++++++++++++++++++++++++++++++++++"<<std::endl;
}

#include <time.h>
std::string random_id(int len){
	static bool k = true;
	if (k){
		srand (time(NULL));
		k = false;
	}
	std::string ret;
	for (int i=0; i<len; ++i) ret += char(97 + rand()%26);
	return ret;
}

void Debug::geogebra_contour(const EdgeData& c){
	auto op = Contour::OrderedPoints(c);
	std::string id = random_id(3);
	for (int i=0; i<op.size(); ++i){
		if (i == op.size() - 1 && Contour::IsClosed(c)) break;
		printf("P%s_{%i} = (%10.8f, %10.8f)\n", id.c_str(), i+1, op[i]->x, op[i]->y);
	}
	if (op.size() == 2){
		std::cout<<"L"+id<<" = Segment[";
		std::cout<<"P"+id+"_{1}, ";
		std::cout<<"P"+id+"_{2}]"<<std::endl;
	} else if (!Contour::IsClosed(c)){
		std::cout<<"L"+id<<" = Polyline[";
		for (int i=0; i<op.size(); ++i){
			if (i>0) std::cout<<", ";
			std::cout<<"P"+id+"_{"<<i+1<<"}";
		}
		std::cout<<"]"<<std::endl;
	} else {
		std::cout<<"L"+id<<" = Polygon[";
		for (int i=0; i<op.size()-1; ++i){
			if (i>0) std::cout<<", ";
			std::cout<<"P"+id+"_{"<<i+1<<"}";
		}
		std::cout<<"]"<<std::endl;
	}
}


void Debug::geogebra_tree(const Contour::Tree& c){
	for (auto n: c.nodes) geogebra_contour(n->contour);
}
void Debug::geogebra_ecollection(const EdgeData& ecol){
	auto etree = HM2D::Contour::Tree::Assemble(ecol);
	geogebra_tree(etree);
}


VertexData Debug::allvertices(const EdgeData& c){
	VertexData ret;
	for (auto e: c){
		ret.push_back(e->first());
		ret.push_back(e->last());
	}
	return aa::no_duplicates(ret);
}
EdgeData Debug::alledges(const CellData& cells){
	EdgeData ret;
	for (auto c: cells)
	for (auto e: c->edges) ret.push_back(e);
	return aa::no_duplicates(ret);
}

void Debug::save_edges_vtk(const EdgeData& c){
	auto av = allvertices(c);
	aa::RestoreIds<EdgeData> r1(c);
	aa::RestoreIds<VertexData> r2(av);

	HM2D::Export::ContourVTK(c, "_dbgout.vtk");
}

void Debug::save_edges_vtk(const EdgeData& c, const std::map<Point*, double>& data){
	save_edges_vtk(c);
	auto av = allvertices(c);
	vector<float> values(av.size(), -1);
	for (int i=0; i<values.size(); ++i){
		auto fnd = data.find(av[i].get());
		if (fnd == data.end()) continue;
		values[i] = fnd->second;
	}
	std::ofstream fs("_dbgout.vtk", std::ios::app);
	fs<<"POINT_DATA "<<av.size()<<std::endl;
	fs<<"SCALARS mapped_values float 1"<<std::endl;
	fs<<"LOOKUP_TABLE default"<<std::endl;
	for (auto x: values) fs<<x<<std::endl;
}

void Debug::save_vertices_vtk(const VertexData& c){
	aa::RestoreIds<VertexData> r1(c);

	HM2D::Export::VerticesVTK(c, "_dbgout.vtk");
}
void Debug::save_vertices_vtk(const vector<Point>& c){
	VertexData vd(c.size());
	for (int i=0; i<c.size(); ++i){
		vd[i] = shared_ptr<Vertex>(new Vertex(c[i]));
	}
	return Debug::save_vertices_vtk(vd);
}
void Debug::save_vertices_vtk(const EdgeData& c){
	auto av = allvertices(c);
	save_vertices_vtk(av);
}

void Debug::save_grid_vtk(const GridData& c){
	aa::RestoreIds<VertexData> r1(c.vvert);
	aa::RestoreIds<EdgeData> r2(c.vedges);
	aa::RestoreIds<CellData> r3(c.vcells);

	HM2D::Export::GridVTK(c, "_dbgout.vtk");
}
void Debug::save_cells_vtk(const CellData& data){
	std::map<int, weak_ptr<Cell>> oldleft;
	std::map<int, weak_ptr<Cell>> oldright;

	GridData grid;
	grid.vcells.reserve(data.size());
	std::copy_if(data.begin(), data.end(),
			std::back_inserter(grid.vcells),
			[](shared_ptr<Cell> c){ return c!=nullptr; });
	grid.vedges = alledges(grid.vcells);
	grid.vvert = allvertices(grid.vedges);

	aa::RestoreIds<VertexData> r1(grid.vvert);
	aa::RestoreIds<EdgeData> r2(grid.vedges);
	aa::RestoreIds<CellData> r3(grid.vcells);

	for (auto e: grid.vedges){
		if (e->has_left_cell()) e->left.lock()->id = 1;
		if (e->has_right_cell()) e->right.lock()->id = 1;
	}
	aa::constant_ids_pvec(data, 0);
	for (int i=0; i<grid.vedges.size(); ++i){
		auto& e = grid.vedges[i];
		if (e->has_left_cell() && e->left.lock()->id == 1){
			oldleft.emplace(i, e->left);
			e->left.reset();
		}
		if (e->has_right_cell() && e->right.lock()->id == 1){
			oldright.emplace(i, e->right);
			e->right.reset();
		}
	}

	save_grid_vtk(grid);

	for (auto& it: oldright){
		grid.vedges[it.first]->right = it.second;
	}
	for (auto& it: oldleft){
		grid.vedges[it.first]->left = it.second;
	}

}

void Debug::save_cells_vtk_id(const CellData& data){
	save_cells_vtk(data);
	std::ofstream fs("_dbgout.vtk", std::ios::app);
	fs<<"CELL_DATA "<<data.size()<<std::endl;
	fs<<"SCALARS id int 1"<<std::endl;
	fs<<"LOOKUP_TABLE default"<<std::endl;
	for (int i=0; i<data.size(); ++i) fs<<data[i]->id<<std::endl;
}
void Debug::save_edges_vtk_id(const EdgeData& data){
	save_edges_vtk(data);
	std::ofstream fs("_dbgout.vtk", std::ios::app);
	fs<<"CELL_DATA "<<data.size()<<std::endl;
	fs<<"SCALARS id int 1"<<std::endl;
	fs<<"LOOKUP_TABLE default"<<std::endl;
	for (int i=0; i<data.size(); ++i) fs<<data[i]->id<<std::endl;
}
void Debug::save_vertices_vtk_id(const VertexData& data){
	save_vertices_vtk(data);
	std::ofstream fs("_dbgout.vtk", std::ios::app);
	fs<<"CELL_DATA "<<data.size()<<std::endl;
	fs<<"SCALARS id int 1"<<std::endl;
	fs<<"LOOKUP_TABLE default"<<std::endl;
	for (int i=0; i<data.size(); ++i) fs<<data[i]->id<<std::endl;
}
void Debug::save_vertices_vtk_id(const EdgeData& data){
	auto av = allvertices(data);
	save_vertices_vtk_id(av);
}

#endif
