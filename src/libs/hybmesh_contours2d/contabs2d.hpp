#ifndef HYBMESH_CONTABS2D_HPP
#define HYBMESH_CONTABS2D_HPP

#include "primitives2d.hpp"

namespace HM2D{ namespace Connectivity {

//Vertex->Edges
struct VertexEdgeR{
	shared_ptr<Vertex> v;
	vector<int> eind;
	size_t size() const { return eind.size(); }
};
vector<VertexEdgeR> VertexEdge(const EdgeData& data);

vector<vector<int>> EdgeEdge(const EdgeData& data);

struct VertexCellR{
	shared_ptr<Vertex> v;
	vector<int> cind;
	size_t size() const { return cind.size(); }
};
vector<VertexCellR> VertexCell(const CellData& data);

vector<vector<int>> CellCell(const CellData& data);


}}
#endif
