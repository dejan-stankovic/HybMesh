import ctypes as ct
from . import libhmcport, list_to_c, free_cside_array
from hybmeshpack.basic.geom import Point2


def cont2_to_c(cont):
    """ return c pointer to a contours2.AbstractContour2 objects
    """
    # arguments
    c_npnt = ct.c_int(cont.n_points())
    c_pnts = (ct.c_double * (2 * c_npnt.value))()
    c_nedges = ct.c_int(cont.n_edges())
    for i, p in enumerate(cont.points):
        c_pnts[2 * i] = p.x
        c_pnts[2 * i + 1] = p.y

    c_edges = (ct.c_int * (2 * c_nedges.value))()
    for i, [i0, i1, b] in enumerate(cont.edges_points()):
        c_edges[2 * i] = i0
        c_edges[2 * i + 1] = i1

    return libhmcport.create_ecollection_container(c_npnt, c_pnts,
                                                   c_nedges, c_edges)


def cont2_from_c(c_cont, c_bnd=None):
    from hybmeshpack.gdata.contour2 import Contour2
    """ returns contours2.Contour2 from c pointer to ecollection """
    if c_cont == 0:
        return None
    c_eds, c_pts = ct.POINTER(ct.c_int)(), ct.POINTER(ct.c_double)()
    npts, neds = ct.c_int(), ct.c_int()
    libhmcport.ecollection_edges_info(c_cont,
                                      ct.byref(npts), ct.byref(neds),
                                      ct.byref(c_pts), ct.byref(c_eds))
    # from c arrays to python lists
    pts = []
    edcon = []
    for i in range(0, 2 * npts.value, 2):
        pts.append(Point2(c_pts[i], c_pts[i + 1]))
    for i in range(0, 2 * neds.value, 2):
        edcon.append([c_eds[i], c_eds[i + 1]])
    libhmcport.free_ecollection_edges_info(c_pts, c_eds)
    ret = Contour2.create_from_point_set(pts, edcon)
    if c_bnd is not None:
        bmap = {}
        for i, b in enumerate(c_bnd):
            if b != 0:
                bmap[i] = b
        ret.set_edge_bnd(bmap)
    return ret


def contour_from_hmxml(reader, subnode):
    """ returns (python side contour, its name in hmxml file)"""
    creader, c_c = 0, 0
    try:
        name = ct.create_string_buffer(1000)
        creader = libhmcport.creader_create(reader, subnode, name)
        if creader == 0:
            raise Exception("Failed to assemble a contour")
        # read grid
        c_c = libhmcport.creader_getresult(creader)
        c = cont2_from_c(c_c)

        # read boundary types
        libhmcport.creader_read_edge_field.restype = ct.POINTER(ct.c_int)
        bnd = libhmcport.creader_read_edge_field(
            creader, "__boundary_types__", "int")
        edgebt = {}
        if bnd:
            for i in range(c.n_edges()):
                if bnd[i] != 0:
                    edgebt[i] = bnd[i]
            c.set_edge_bnd(edgebt)
            libhmcport.free_int_array(bnd)
        return c, str(name.value)
    except:
        raise
    finally:
        free_cont2(c_c) if c_c != 0 else None
        libhmcport.creader_free(creader) if creader != 0 else None


def free_cont2(cont):
    """ frees pointer to ecollection container"""
    libhmcport.free_ecollection_container(cont)


def clip_domain(cc1, cc2, op, simplify):
    """ cc1, cc2 - c pointers to ecollection containers
    op is {1: union, 2: difference, 3: intersection, 4: xor}
    simplify (bool)
    Returns pointer to resulting container or None
    """
    c_op = ct.c_int(op)
    c_simp = ct.c_int(1 if simplify else 0)
    res = libhmcport.domain_clip(cc1, cc2, c_op, c_simp)
    if (res == 0):
        return None
    else:
        return res


def contour_partition(c_cont, c_bt, c_step, algo, a0,
                      keepbnd, nedges, crosses, keep_pts, sp, ep):
    """ c_step: ct.c_double() * n, where n dependes on algo:
    algo = "const" => n = 1,
    algo = "ref_points" => n = 3*k
    algo = "ref_weights" => n = 2*k
    """
    c_cont = ct.c_void_p(c_cont)
    # algo treatment
    if algo == "const":
        c_algo = ct.c_int(0)
        c_n = ct.c_int(0)
    elif algo == "ref_points":
        c_algo = ct.c_int(1)
        c_n = ct.c_int(len(c_step) / 3)
    elif algo == "ref_weights":
        c_algo = ct.c_int(2)
        c_n = ct.c_int(len(c_step) / 2)
    elif algo == "ref_lengths":
        c_algo = ct.c_int(3)
        c_n = ct.c_int(len(c_step) / 2)
    else:
        raise ValueError("invalid partition algo")
    # start/end point
    if sp is not None:
        c_sp = list_to_c([sp.x, sp.y], "float")
    else:
        c_sp = ct.c_void_p(0)
    if ep is not None:
        c_ep = list_to_c([ep.x, ep.y], "float")
    else:
        c_ep = ct.c_void_p(0)

    # prepare arrays for boundary output
    c_bnd = ct.POINTER(ct.c_int)()
    n_bnd = ct.c_int(0)
    c_ne = ct.c_int(-1 if nedges is None else nedges)
    num_crosses = ct.c_int(len(crosses))
    c_crosses = list_to_c(crosses, "void*")
    num_keep_pts = ct.c_int(len(keep_pts))
    kp = []
    for p in keep_pts:
        kp.extend([p.x, p.y])
    c_keep_pts = list_to_c(kp, "float")

    ret = libhmcport.contour_partition(c_cont, c_bt, c_algo,
                                       c_n, c_step,
                                       ct.c_double(a0),
                                       ct.c_int(1 if keepbnd else 0), c_ne,
                                       num_crosses, c_crosses,
                                       num_keep_pts, c_keep_pts,
                                       c_sp, c_ep,
                                       ct.byref(n_bnd), ct.byref(c_bnd)
                                       )
    # copy c-side allocated bnd array to python side allocated
    c_bndret = (ct.c_int * (n_bnd.value))()
    for i in range(len(c_bndret)):
        c_bndret[i] = c_bnd[i]
    # clear c-side allocated array
    libhmcport.free_int_array(c_bnd)

    if ret == 0:
        raise Exception("contour partition failed")
    return ret, c_bndret


def spline(c_pnts, c_bt, nedges):
    # algo treatment
    c_bnd = ct.POINTER(ct.c_int)()
    n_bnd = ct.c_int(0)
    c_ne = ct.c_int(nedges)
    npnt = len(c_pnts) / 2
    nbt = len(c_bt)

    ret = libhmcport.spline(npnt, c_pnts, nbt, c_bt, c_ne,
                            ct.byref(n_bnd), ct.byref(c_bnd))
    # copy c-side allocated bnd array to python side allocated
    c_bndret = (ct.c_int * (n_bnd.value))()
    for i in range(len(c_bndret)):
        c_bndret[i] = c_bnd[i]
    # clear c-side allocated array
    libhmcport.free_int_array(c_bnd)

    if ret == 0 or ret is None:
        raise Exception("spline builder failed")
    return ret, c_bndret


def matched_partition(c_cont, c_src, c_pts, step, dist, pw, a0):
    c_step = ct.c_double(step)
    c_a0 = ct.c_double(a0)
    c_dist = ct.c_double(dist)
    c_nc = ct.c_int(len(c_src))
    c_src2 = list_to_c(c_src, 'void*')
    c_pw = ct.c_double(pw)
    c_np = ct.c_int(len(c_pts) / 3) if c_pts is not None else ct.c_int(0)

    ret = libhmcport.matched_partition(c_cont, c_nc, c_src2, c_np, c_pts,
                                       c_step, c_dist, c_pw, c_a0)
    if ret == 0:
        raise Exception("contour partition failed")
    return ret


def segment_part(start, end, hstart, hend, hinternal):
    """ return list of double containing at least two entries """
    c_start = ct.c_double(start)
    c_end = ct.c_double(end)
    c_h0 = ct.c_double(hstart)
    c_h1 = ct.c_double(hend)
    c_in = ct.c_int(len(hinternal) / 2)
    c_ih = list_to_c(hinternal, float)
    c_resn = ct.c_int(0)
    c_resh = ct.POINTER(ct.c_double)()

    isok = libhmcport.segment_part(c_start, c_end, c_h0, c_h1, c_in, c_ih,
                                   ct.byref(c_resn), ct.byref(c_resh))
    if isok == 0 or c_resn.value < 2:
        raise Exception("segment partition failed")

    ret = []
    for i in range(c_resn.value):
        ret.append(c_resh[i])

    libhmcport.free_double_array(c_resh)

    return ret


def extract_contour(c_src, plist, project_to):
    pnts = []
    for p in plist:
        pnts.extend([p.x, p.y])
    c_pnts = list_to_c(pnts, float)
    c_npnts = ct.c_int(len(plist))
    ret_conts = ct.POINTER(ct.c_void_p)()
    ok = libhmcport.extract_contour(c_src, c_npnts, c_pnts, project_to,
                                    ct.byref(ret_conts))
    if ok != 0:
        ret = []
        for i in range(len(plist) - 1):
            ret.append(ret_conts[i])
        return ret
    else:
        raise Exception("contour extraction failed")


def unite_contours(conts):
    "-> (c_return, new-edge -> old-edge linking list)"
    c_nconts = ct.c_int(len(conts))
    c_conts = list_to_c(conts, 'void*')
    ret_cont = ct.c_void_p(0)
    ret_nlinks = ct.c_int(0)
    ret_links = ct.POINTER(ct.c_int)()
    a = libhmcport.unite_contours(
        c_nconts, c_conts,
        ct.byref(ret_cont),
        ct.byref(ret_nlinks), ct.byref(ret_links))

    if a == 0:
        raise Exception("unite_contours failed")

    links = []
    for i in range(ret_nlinks.value):
        links.append(ret_links[i])
    free_cside_array(ret_links, 'int')
    return ret_cont, links


def simplify_contour(cont, btypes, angle):
    """ cont - c-side contour
        btypes - boundary types of cont (python side list)
        angle - minimum allowed angle
        -> (c_return, btypes of c_return) or raise
    """
    c_a = ct.c_double(angle)
    c_btypes = list_to_c(btypes, 'int')
    ret_nbtypes = ct.c_int(0)
    ret_btypes = ct.POINTER(ct.c_int)()
    ret_cont = ct.c_void_p()
    a = libhmcport.simplify_contour(
        cont, c_a, c_btypes,
        ct.byref(ret_cont),
        ct.byref(ret_nbtypes), ct.byref(ret_btypes))
    if a == 0:
        raise Exception("simplify contour failed")
    bt = [ret_btypes[i] for i in range(ret_nbtypes.value)]
    free_cside_array(ret_btypes, 'int')
    return ret_cont, bt


def full_separate_contour(cont, btypes):
    """ cont - c-side contour
        btypes - python list of boundary types of cont
        -> ([ret], [btypes of ret])  or raise
    """
    c_btypes = list_to_c(btypes, 'int')
    ret_nconts = ct.c_int(0)
    ret_conts = ct.POINTER(ct.c_void_p)()
    ret_nbtypes = ct.c_int(0)
    ret_btypes = ct.POINTER(ct.c_int)()
    ans = libhmcport.separate_contour(
        cont, c_btypes,
        ct.byref(ret_nconts), ct.byref(ret_conts),
        ct.byref(ret_nbtypes), ct.byref(ret_btypes))
    if ans == 0:
        raise Exception("full_separate_contour failed")
    ret, btypes = [], []
    for i in range(ret_nconts.value):
        ret.append(ret_conts[i])
    for i in range(ret_nbtypes.value):
        btypes.append(ret_btypes[i])
    free_cside_array(ret_btypes, 'int')
    free_cside_array(ret_conts, 'void*')
    return ret, btypes


def quick_separate_contour(cont, btypes):
    """ cont - c-side contour
        btypes - python list of boundary types of cont
        -> ([ret], [btypes of ret])  or raise
    """
    c_btypes = list_to_c(btypes, 'int')
    ret_nconts = ct.c_int(0)
    ret_conts = ct.POINTER(ct.c_void_p)()
    ret_nbtypes = ct.c_int(0)
    ret_btypes = ct.POINTER(ct.c_int)()
    ans = libhmcport.quick_separate_contour(
        cont, c_btypes,
        ct.byref(ret_nconts), ct.byref(ret_conts),
        ct.byref(ret_nbtypes), ct.byref(ret_btypes))
    if ans == 0:
        raise Exception("quick_separate_contour failed")
    ret, btypes = [], []
    for i in range(ret_nconts.value):
        ret.append(ret_conts[i])
    for i in range(ret_nbtypes.value):
        btypes.append(ret_btypes[i])
    free_cside_array(ret_btypes, 'int')
    free_cside_array(ret_conts, 'void*')
    return ret, btypes


def connect_subcontours(conts, fix, close, shiftnext):
    c_nconts = ct.c_int(len(conts))
    c_conts = list_to_c(conts, "void*")
    c_nfix = ct.c_int(len(fix))
    c_fix = list_to_c(fix, "int")
    c_close = close
    cret = ct.c_void_p()
    c_shiftnext = ct.c_int(1 if shiftnext else 0)
    ok = libhmcport.connect_subcontours(c_nconts, c_conts, c_nfix, c_fix,
                                        c_close, c_shiftnext,
                                        ct.byref(cret))
    if ok == 1:
        return cret
    else:
        raise Exception("connect subcontours failed")


def cwriter_create(contname, c_c, c_writer, c_sub, fmt):
    ret = libhmcport.cwriter_create(contname, c_c, c_writer, c_sub, fmt)
    if ret == 0:
        raise Exception("Error writing contour to hmc")
    else:
        return ret


def cwriter_add_edge_field(c_cwriter, fname, fieldtype, c_field):
    ret = 0
    if fieldtype == "int":
        ret = libhmcport.cwriter_add_edge_field(
            c_cwriter, fname, ct.cast(c_field, ct.c_void_p),
            ct.c_int(len(c_field)), "int")
    elif fieldtype == "char":
        ret = libhmcport.cwriter_add_edge_field(
            c_cwriter, fname, ct.cast(c_field, ct.c_void_p),
            ct.c_int(len(c_field)), "char")
    elif fieldtype == "double":
        ret = libhmcport.cwriter_add_edge_field(
            c_cwriter, fname, ct.cast(c_field, ct.c_void_p),
            ct.c_int(len(c_field)), "double")
    elif fieldtype == "float":
        ret = libhmcport.cwriter_add_edge_field(
            c_cwriter, fname, ct.cast(c_field, ct.c_void_p),
            ct.c_int(len(c_field)), "float")
    if ret == 0:
        raise Exception("Error writing edge field to a contour")
    else:
        return ret


def free_cwriter(c_cwriter):
    libhmcport.cwriter_free(c_cwriter)
