#ifndef HMCONT2D_CONT_ASSEMBLER_HPP
#define HMCONT2D_CONT_ASSEMBLER_HPP

#include "contour.hpp"
//assemble routines (unlike construction routines) tries to
//use shallow copies of input data when possible

namespace HM2D{ namespace Contour{ namespace Assembler{

//returns all possible contours (including 1 edge contours) from collection
//direction is arbitrary
//Resulting contours will not intersect each other if input edges don't 
vector<EdgeData> AllContours(const EdgeData& input);

//all contours start and end in points which have !=2 edges connections
vector<EdgeData> SimpleContours(const EdgeData& input);

//Assemble single contour from shattered edges starting from given points of collection edges
//resulting direction is arbitrary.
//Use ShrinkContour if col is known to be a contour.
EdgeData Contour1(const EdgeData& col, const Point* pnt_start, const Point* pnt_end=0);

//from consequent list of points
EdgeData Contour1(const VertexData& pnt, bool force_closed=false);

//assemble contour from another contour.
EdgeData ShrinkContour(const EdgeData& con, const Point* pnt_start, const Point* pnt_end);


// ================== Ecollection modificators
//splits the edge->vertices graph
//!!!resulting edges do not form assembled contours
vector<EdgeData> QuickSeparate(const EdgeData& ecol);

}}}

#endif

