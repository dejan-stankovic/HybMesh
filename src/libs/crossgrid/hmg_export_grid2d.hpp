#ifndef HYBMESH_HMG_EXPORT_GRID2D_HPP
#define HYBMESH_HMG_EXPORT_GRID2D_HPP

#include "grid.h"
#include "hmxmlreader.hpp"

namespace GGeom{ namespace Export{
struct MultipleGridsHMG;
struct GridHMG;

//this class contains only function for exporting
//No write-to-file procedures and auto-write on destruction
struct GridWriter{
	//tp = "ascii", "bin", "floatbin"
	GridWriter(const GridGeom& g,
			HMXML::ReaderA* writer,
			HMXML::Reader* subnode,
			std::string gridname,
			std::string tp);

	void AddVertexData(std::string fieldname, const vector<double>& data, bool binary);
	void AddVertexData(std::string fieldname, const vector<float>& data, bool binary);
	void AddVertexData(std::string fieldname, const vector<char>& data, bool binary);
	void AddVertexData(std::string fieldname, const vector<int>& data, bool binary);

	void AddEdgeData(std::string fieldname, const vector<double>& data, bool binary);
	void AddEdgeData(std::string fieldname, const vector<float>& data, bool binary);
	void AddEdgeData(std::string fieldname, const vector<char>& data, bool binary);
	void AddEdgeData(std::string fieldname, const vector<int>& data, bool binary);

	void AddCellData(std::string fieldname, const vector<double>& data, bool binary);
	void AddCellData(std::string fieldname, const vector<float>& data, bool binary);
	void AddCellData(std::string fieldname, const vector<char>& data, bool binary);
	void AddCellData(std::string fieldname, const vector<int>& data, bool binary);

	void AddVertexData(std::string fieldname, const vector<vector<double>>& data, bool binary);
	void AddVertexData(std::string fieldname, const vector<vector<float>>& data, bool binary);
	void AddVertexData(std::string fieldname, const vector<vector<char>>& data, bool binary);
	void AddVertexData(std::string fieldname, const vector<vector<int>>& data, bool binary);

	void AddEdgeData(std::string fieldname, const vector<vector<double>>& data, bool binary);
	void AddEdgeData(std::string fieldname, const vector<vector<float>>& data, bool binary);
	void AddEdgeData(std::string fieldname, const vector<vector<char>>& data, bool binary);
	void AddEdgeData(std::string fieldname, const vector<vector<int>>& data, bool binary);

	void AddCellData(std::string fieldname, const vector<vector<double>>& data, bool binary);
	void AddCellData(std::string fieldname, const vector<vector<float>>& data, bool binary);
	void AddCellData(std::string fieldname, const vector<vector<char>>& data, bool binary);
	void AddCellData(std::string fieldname, const vector<vector<int>>& data, bool binary);

	void AddCellVertexConnectivity();
	void AddCellEdgeConnectivity();

	template<class A> bool is_binary(){return false;}
private:
	virtual void data_changed(){}

	HMXML::ReaderA* pwriter;
	const GridGeom* grid;
	HMXML::Reader gwriter, vwriter, ewriter, cwriter;
	std::string __tp;
};
template<> bool GridWriter::is_binary<int>(){return __tp == "bin";}
template<> bool GridWriter::is_binary<char>(){return __tp == "bin";}
template<> bool GridWriter::is_binary<double>(){return __tp != "ascii";}
template<> bool GridWriter::is_binary<float>(){return __tp != "ascii";}

struct WOwner{
	WOwner(): writer(HMXML::ReaderA::pcreate("HybMeshData")){}
	WOwner(shared_ptr<HMXML::ReaderA> w): writer(w){}
	shared_ptr<HMXML::ReaderA> writer;
};

//Grid writer with Flush and auto-flush procedures
//with file specification.
//Use this to save the grid to a file quickly.
struct GridHMG: private WOwner, public GridWriter{
	//tp = "ascii", "bin", "floatbin"
	GridHMG(const GridGeom& g, std::string fn,
			std::string gridname, std::string tp);
	GridHMG(const GridGeom& g, std::string fn,
			shared_ptr<HMXML::ReaderA> writer,
			std::string gridname, std::string tp);
	~GridHMG();

	//invoked by destructor or manually
	void Flush();
private:
	void data_changed() override {__flushed=false;}
	friend struct MultipleGridsHMG;
	bool __flushed;
	std::string __filename;
};

struct MultipleGridsHMG{
	MultipleGridsHMG(vector<const GridGeom*> grids, vector<std::string> names, std::string fn, std::string tp);
	~MultipleGridsHMG();
	GridHMG* sub(int i){ return &subs[i]; }
	void Flush();
private:
	vector<GridHMG> subs;
	shared_ptr<HMXML::ReaderA> writer;
	bool __flushed();
};

}}


#endif
