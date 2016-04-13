#ifndef HYBMESH_HMCPORT_GRID2D_H
#define HYBMESH_HMCPORT_GRID2D_H
#include "hmcport.h"

extern "C"{

//boundary struct
struct Grid2DBoundaryStruct{
	int n;
	int* edge_start_nodes;
	int* edge_end_nodes;
	int* btypes;
};
Grid2DBoundaryStruct* set_grid2_boundary_types(int n, int* n1, int* n2, int* bt);
void free_grid2_boundary_types(Grid2DBoundaryStruct*);

//data_periodic is a 
int export_msh_grid(const Grid* grid, const char* fname,
		const Grid2DBoundaryStruct* bstr,
		const BoundaryNamesStruct* bnames,
		int n_periodic,
		int* data_periodic);

//builds rectangular grid on basis of four open HMCont2D::Contour objects
//algo is:
//  0) linear connection
//  1) laplas connection
//  2) conformal connection
//if input are not positioned correctly their vertices coordinates will be changed
//returns pointer to GridGeom or NULL if failed
void* custom_rectangular_grid(int algo, void* left, void* bot, void* right, void* top);

}
#endif
