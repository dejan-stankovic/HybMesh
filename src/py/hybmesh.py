#!/usr/bin/env python
"""
Console interface for HybMesh. Possible arguments:
-v -- print version and exit
-u -- check for updates and exit
-sx fn.py [-silent] [-verbosity n]
    Execute hybmesh python script fn.py
    -silent: no console callback during execution
    -verbosity n: choose console output mode
        0: silent mode (same as -silent)
        1: +execution errors descriptions
        2: +commands start/end reports
        3: +progress bar [default]
"""
# -x fn.hmp [-sgrid gname fmt fn] [-sproj fn] [-silent]
#     Execute command flow from 'hmp' file and saves resulting data.
#     Options are
#     -sgrid gname fmt fn: export resulting grid with called gname to
#     file fn using format fmt. Can be called multiple times.
#         Formats:
#             vtk - vtk format
#             hmg - native HybMesh format
#             msh - fluent mesh format

#     -sproj fn.hmp: save project file after execution to file fn.hmp
#     -silent: no console callback during execution

#     Example:
#     > hybmesh -x state1.hmp -sgrid Grid1 vtk grid1.vtk -sproj final.hmp
#         1) loads data from stat1.hmp
#         2) executes command flow
#         3) exports Grid1 to vtk format
#         4) saves finalized project to final.hmp
import sys
from hybmeshpack import progdata, imex, basic


def main():
    if len(sys.argv) == 1 or sys.argv[1] in ['help', 'h', '-help', '-h']:
        print __doc__
        sys.exit()

    if len(sys.argv) == 2:
        if sys.argv[1] == '-v':
            print progdata.HybMeshVersion.current()
            sys.exit()
        if sys.argv[1] == '-u':
            r = progdata.check_for_updates()
            print 'Current version: %s' % r[0]
            if r[1] is None:
                print 'Failed to check for latest version'
            elif r[2] == -1:
                print 'New version %s is available at %s' % \
                    (r[1], progdata.project_url())
            else:
                print 'No updates are available'
            sys.exit()

    # execution
    if len(sys.argv) >= 3 and '-sx' in sys.argv:
        fn = sys.argv[sys.argv.index('-sx') + 1]
        from hybmeshpack import hmscript
        verb = 3
        if '-silent' in sys.argv:
            verb = 0
        elif '-verbosity' in sys.argv:
            try:
                iv = sys.argv.index('-verbosity')
                verb = int(sys.argv[iv + 1])
                if verb < 0 or verb > 3:
                    raise
            except:
                sys.exit('Invalid verbosity level. See -help.')
        if verb == 0:
            hmscript.flow.set_interface(hmscript.ConsoleInterface0())
        elif verb == 1:
            hmscript.flow.set_interface(hmscript.ConsoleInterface1())
        elif verb == 2:
            hmscript.flow.set_interface(hmscript.ConsoleInterface2())
        elif verb == 3:
            hmscript.flow.set_interface(hmscript.ConsoleInterface3())
        execfile(fn)
        if verb > 1:
            print "DONE"
        sys.exit()
    elif len(sys.argv) >= 3 and '-x' in sys.argv:
        fn = sys.argv[sys.argv.index('-x') + 1]
        # load flow and state
        f = imex.read_flow_and_framework_from_file(fn)

        if '-silent' not in sys.argv:
            f.set_interface(basic.interf.ConsoleInterface())

        # run till end
        f.exec_all()

        # save current state
        if '-sproj' in sys.argv:
            fn = sys.argv[sys.argv.index('-sproj') + 1]
            imex.write_flow_and_framework_to_file(f, fn)

        # export grid
        for i, op in enumerate(sys.argv):
            if op == '-sgrid':
                gname = sys.argv[i + 1]
                fmt = sys.argv[i + 2]
                fn = sys.argv[i + 3]
                try:
                    imex.export_grid(fmt, fn, flow=f, name=gname)
                    print '%s saved to %s' % (gname, fn)
                except Exception as e:
                    print 'Exporting Failure: %s' % str(e)

        print "DONE"
        sys.exit()

    sys.exit('Invalid option string. See -help.')


if __name__ == '__main__':
    main()
