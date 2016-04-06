#include "construct_grid3d.hpp"
#include "debug_grid2d.h"
#include "debug_grid3d.hpp"


using namespace HMGrid3D;

namespace cns = Constructor;

HMGrid3D::Grid cns::Cuboid(HMGrid3D::Vertex leftp, double lx, double ly, double lz, int nx, int ny, int nz){
	HMGrid3D::Grid ret;
	//Vertices
	ShpVector<Vertex> vert; vert.reserve(nx*ny*nz);
	double hx = lx/(nx), hy = ly/(ny), hz = lz/(nz);
	vector<double> x(nx+1), y(ny+1), z(nz+1);
	for (int i=0; i<nx+1; ++i) x[i] = leftp.x + i*hx;
	for (int j=0; j<ny+1; ++j) y[j] = leftp.y + j*hy;
	for (int k=0; k<nz+1; ++k) z[k] = leftp.z + k*hz;
	for (int k=0; k<nz+1; ++k){
		for (int j=0; j<ny+1; ++j){
			for (int i=0; i<nx+1; ++i){
				vert.emplace_back(new Vertex(x[i], y[j], z[k]));
			}
		}
	}
	int sx=1, sy=nx+1, sz=(nx+1)*(ny+1), N=(nx+1)*(ny+1)*(nz+1);
	//Edges
	ShpVector<Edge> xedges(N), yedges(N), zedges(N);
	int gi=0;
	for (int k=0; k<nz+1; ++k){
		for (int j=0; j<ny+1; ++j){
			for (int i=0; i<nx+1; ++i){
				auto p1 = vert[gi];
				//x edge
				if (i<nx){
					auto p2 = vert[gi+sx];
					xedges[gi].reset(new Edge(p1, p2));
				}
				//y edge
				if (j<ny){
					auto p2 = vert[gi+sy];
					yedges[gi].reset(new Edge(p1, p2));
				}
				//z edge
				if (k<nz){
					auto p2 = vert[gi+sz];
					zedges[gi].reset(new Edge(p1, p2));
				}
				++gi;
			}
		}
	}
	//Faces
	ShpVector<Face> xfaces(N), yfaces(N), zfaces(N);
	gi=0;
	for (int k=0; k<nz+1; ++k){
		for (int j=0; j<ny+1; ++j){
			for (int i=0; i<nx+1; ++i){
				//z face
				if (i<nx && j<ny){
					auto e1=yedges[gi], e2=xedges[gi+sy], e3=yedges[gi+sx], e4=xedges[gi];
					zfaces[gi].reset(new Face({e1, e2, e3, e4}));
				}
				//y face
				if (i<nx && k<nz){
					auto e1=xedges[gi], e2=zedges[gi+sx], e3=xedges[gi+sz], e4=zedges[gi];
					yfaces[gi].reset(new Face({e1, e2, e3, e4}));
				};
				//x face
				if (j<ny && k<nz){
					auto e1=zedges[gi], e2=yedges[gi+sz], e3=zedges[gi+sy], e4=yedges[gi];
					xfaces[gi].reset(new Face({e1, e2, e3, e4}));
				}
				++gi;
			}
		}
	}
	//Cells
	ret.cells.reserve(nx*ny*nz);
	for (int k=0; k<nz; ++k){
		for (int j=0; j<ny; ++j){
			for (int i=0; i<nx; ++i){
				gi = i*sx + j*sy + k*sz;
				ret.cells.emplace_back(new Cell);
				auto c = ret.cells.back();
				auto xleft = xfaces[gi]; xleft->left = c;
				auto xright = xfaces[gi+sx]; xright->right = c;
				auto yleft =  yfaces[gi]; yleft->left = c;
				auto yright = yfaces[gi+sy]; yright->right = c;
				auto zleft = zfaces[gi]; zleft->left = c;
				auto zright = zfaces[gi+sz]; zright->right = c;
				c->faces = {xleft, xright, yleft, yright, zleft, zright};
			}
		}
	}
	//Boundary Types
	auto setbt = [](shared_ptr<Face> f, int nor, int nol){
		if (f){
			if (!f->right) f->boundary_type = nor;
			if (!f->left) f->boundary_type = nol;
		}
	};
	for (auto f: xfaces) setbt(f, 1, 2);
	for (auto f: yfaces) setbt(f, 3, 4);
	for (auto f: zfaces) setbt(f, 5, 6);

	return ret;
}

HMGrid3D::Grid cns::SweepGrid2D(const GridGeom& g, const vector<double>& zcoords){
	return SweepGrid2D(g, zcoords,
			[](int){ return 1; },
			[](int){ return 2; },
			[](int){ return 3; });
}

HMGrid3D::Grid cns::SweepGrid2D(const GridGeom& g, const vector<double>& zcoords,
		std::function<int(int)> bottom_bt,
		std::function<int(int)> top_bt,
		std::function<int(int)> side_bt){

	HMGrid3D::Grid ret;

	//Needed Data
	auto e2d = g.get_edges();
	vector<::Edge> e2dvector(e2d.begin(), e2d.end());
	int n2p = g.n_points(), n2c = g.n_cells(), n2e = e2d.size(), nz = zcoords.size();
	vector<vector<int>> cell_edges2d(n2c);
	for (int i=0; i<n2c; ++i){
		auto c2d = g.get_cell(i);
		for (int j=0; j<c2d->dim(); ++j){
			int p1 = c2d->get_point(j)->get_ind();
			int p2 = c2d->get_point(j+1)->get_ind();
			::Edge e(p1, p2);
			int fnd = std::find(e2dvector.begin(), e2dvector.end(), e) - e2dvector.begin();
			cell_edges2d[i].push_back(fnd);
		}
	}

	//Vertices
	ShpVector<Vertex> vert; vert.reserve(n2p*nz);
	{
		for (int i=0; i<nz; ++i){
			double z = zcoords[i];
			for (int j=0; j<n2p; ++j){
				auto p = g.get_point(j);
				vert.emplace_back(new Vertex(p->x, p->y, z));
			}
		}
	}

	//Edges
	ShpVector<Edge> xyedges(n2e*nz), zedges((nz-1)*n2p);
	{
		//xy edges
		int j=0;
		for (int i=0; i<nz; ++i){
			for (auto& e: e2dvector){
				int i1 = i*n2p + e.p1;
				int i2 = i*n2p + e.p2;
				xyedges[j++].reset(new Edge(vert[i1], vert[i2]));
			}
		}
		//z edges
		int k=0;
		for (int i=0; i<nz-1; ++i){
			for (int j=0; j<n2p; ++j){
				int i1 = i*n2p + j;
				int i2 = (i+1)*n2p + j;
				zedges[k++].reset(new Edge(vert[i1], vert[i2]));
			}
		}
	}

	//Faces
	ShpVector<Face> xyfaces; xyfaces.reserve(n2c*nz);
	ShpVector<Face> zfaces; zfaces.reserve(n2e*(nz-1));
	{
		//xyfaces
		for (int i=0; i<nz; ++i){
			for (int j=0; j<n2c; ++j){
				Face* f = new Face();
				for (auto k: cell_edges2d[j]){
					f->edges.push_back(xyedges[i*n2e+k]);
				}
				xyfaces.emplace_back(f);
			}
		}
		//zfaces
		for (int i=0; i<nz-1; ++i){
			for (int j=0; j<n2e; ++j){
				int i12d = e2dvector[j].p1;
				int i22d = e2dvector[j].p2;
				Face* f = new Face();
				auto e1 = xyedges[j + i*n2e];
				auto e2 = zedges[i22d + i*n2p];
				auto e3 = xyedges[j+ (i+1)*n2e];
				auto e4 = zedges[i12d + i*n2p];
				f->edges = {e1, e2, e3, e4};
				zfaces.emplace_back(f);
			}
		}
	}

	//Cells
	{
		ret.cells.reserve(n2c*(nz-1));
		for (int i=0; i<nz-1; ++i){
			for (int j=0; j<n2c; ++j){
				ret.cells.emplace_back(new Cell());
				auto& c = ret.cells.back();
				int ifc = i*n2c + j;
				auto bot = xyfaces[ifc]; bot->right = c;
				auto top = xyfaces[ifc + n2c]; top->left = c;
				c->faces = {bot, top};
				for (int k: cell_edges2d[j]){
					auto f1 = zfaces[k + n2e*i];
					c->faces.push_back(f1);
					bool isleft = (e2dvector[k].cell_left == j);
					if (isleft) f1->left = c;
					else f1->right = c;
				}
			}
		}
	}

	//Boundary Types
	{
		//top, bottom
		int j = n2c*(nz-1);
		for (int i=0; i<n2c; ++i){
			xyfaces[i]->boundary_type = bottom_bt(i);
			xyfaces[j++]->boundary_type = top_bt(i);
		}
		//sides
		for (int i=0; i<n2e; ++i) if (e2dvector[i].is_boundary()){
			int bt = side_bt(i);
			for (int k=0; k<nz-1; ++k){
				zfaces[i + k*n2e]->boundary_type = bt;
			}
		}
	}

	return ret;
}

HMGrid3D::Grid Constructor::Copy::ShallowVertices(const HMGrid3D::Grid& b){
	auto bcells = b.allcells();
	auto bfaces = b.allfaces();
	auto bedges = b.alledges();

	std::vector<shared_ptr<Cell>> cells_new; cells_new.reserve(bcells.size());
	std::vector<shared_ptr<Face>> faces_new; faces_new.reserve(bfaces.size());
	std::vector<shared_ptr<Edge>> edges_new; edges_new.reserve(bedges.size());

	for (int i=0; i<bcells.size(); ++i) cells_new.emplace_back(new Cell());
	for (int i=0; i<bfaces.size(); ++i) faces_new.emplace_back(new Face());
	for (int i=0; i<bedges.size(); ++i) edges_new.emplace_back(new Edge());

	//edges
	for (int i=0; i<bedges.size(); ++i) edges_new[i]->vertices = bedges[i]->vertices;

	//faces
	auto _eindexer = aa::shp_container_indexer(bedges);
	_eindexer.convert();
	for (int i=0; i<bfaces.size(); ++i){
		faces_new[i]->edges.reserve(bfaces[i]->n_edges());
		for (auto e: bfaces[i]->edges){
			int ind = _eindexer.index(e);
			faces_new[i]->edges.push_back(edges_new[ind]);
		}
		faces_new[i]->boundary_type = bfaces[i]->boundary_type;
	}
	_eindexer.restore();

	//cells
	auto _findexer = aa::shp_container_indexer(bfaces);
	_findexer.convert();
	for (int i=0; i<bcells.size(); ++i){
		cells_new[i]->faces.reserve(bcells[i]->n_faces());
		for (auto f: bcells[i]->faces){
			int ind = _findexer.index(f);
			cells_new[i]->faces.push_back(faces_new[ind]);
		}
	}
	_findexer.restore();

	//faces->cells connectivity
	for (int i=0; i<bcells.size(); ++i){
		auto oldcell = bcells[i];
		for (int j=0; j<bcells[i]->n_faces(); ++j){
			auto oldface = oldcell->faces[j];
			bool is_left = oldface->left == oldcell;
			if (is_left) cells_new[i]->faces[j]->left = cells_new[i];
			else cells_new[i]->faces[j]->right = cells_new[i];
		}
	}

	HMGrid3D::Grid ret;
	ret.cells = cells_new;

	return ret;
}
