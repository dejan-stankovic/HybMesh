from HybMeshPyPack import com
from HybMeshPyPack.basic.geom import Point2
from HybMeshPyPack.hmscript import flow


# Prototype grids
def add_unf_rect_grid(p0, p1, nx, ny):
    """Builds rectangular grid

    Args:
       p0, p1 (list-of-float): bottom left, top right points as [x, y] list

       nx, ny (int): partition in x, y direction

    Returns:
       created grid identifier

    """
    c = com.gridcom.AddUnfRectGrid({"p0": Point2(*p0),
                                    "p1": Point2(*p1),
                                    "nx": nx,
                                    "ny": ny})
    flow.exec_command(c)
    return c._get_added_names()[0][0]


def add_unf_circ_grid(p0, rad, na, nr, coef=1, is_trian=True):
    """Builds circular grid

    Args:
       p0 (list-of-float): center coordinate as [x, y]

       rad (float): radius

       na, nr (int): partitions of arc and radius respepctively

    Kwargs:
       coef (float): refinement coefficient::

         coef = 1: equidistant radius division
         coef > 1: refinement towards center of circle
         0<coef<1:  refinement towards outer arc

       is_trian (bool): True if center cell should be triangulated

    Returns:
       created grid identifier

    """
    c = com.gridcom.AddUnfCircGrid({
        "p0": Point2(*p0),
        "rad": rad,
        "na": na, "nr": nr,
        "coef": coef,
        "is_trian": is_trian})
    flow.exec_command(c)
    return c._get_added_names()[0][0]


def add_unf_ring_grid(p0, radinner, radouter,
                      na, nr, coef=1.0):
    """Builds ring grid

    Args:
       p0 (list-of-float): center coordinates as [x, y]

       radinner, radouter (float): inner and outer radii

       na, nr (int): arc and radius partition respectiverly

    Kwargs:
       coef (float): refinement coefficient::

         coef = 1: equidistant radius division
         coef > 1: refinement towards center of circle
         0<coef<1:  refinement towards outer arc

    Returns:
       created grid identifier

    """
    c = com.gridcom.AddUnfRingGrid({
        "p0": Point2(*p0),
        "radinner": radinner, "radouter": radouter,
        "na": na, "nr": nr,
        "coef": coef})
    flow.exec_command(c)
    return c._get_added_names()[0][0]