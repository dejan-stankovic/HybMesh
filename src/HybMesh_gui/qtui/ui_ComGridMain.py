# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'ComGridMain.ui'
#
# Created by: PyQt4 UI code generator 4.11.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName(_fromUtf8("MainWindow"))
        MainWindow.resize(804, 600)
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap(_fromUtf8(":/icons/mainwin.png")), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        MainWindow.setWindowIcon(icon)
        MainWindow.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        MainWindow.setAnimated(True)
        MainWindow.setDockNestingEnabled(False)
        MainWindow.setDockOptions(QtGui.QMainWindow.AnimatedDocks)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout_2.setMargin(0)
        self.verticalLayout_2.setObjectName(_fromUtf8("verticalLayout_2"))
        self.f_vtk = QtGui.QFrame(self.centralwidget)
        self.f_vtk.setFrameShape(QtGui.QFrame.StyledPanel)
        self.f_vtk.setFrameShadow(QtGui.QFrame.Sunken)
        self.f_vtk.setLineWidth(4)
        self.f_vtk.setObjectName(_fromUtf8("f_vtk"))
        self.verticalLayout_2.addWidget(self.f_vtk)
        MainWindow.setCentralWidget(self.centralwidget)
        self.menubar = QtGui.QMenuBar(MainWindow)
        self.menubar.setGeometry(QtCore.QRect(0, 0, 804, 20))
        self.menubar.setObjectName(_fromUtf8("menubar"))
        self.menu_File = QtGui.QMenu(self.menubar)
        self.menu_File.setObjectName(_fromUtf8("menu_File"))
        self.menuImport = QtGui.QMenu(self.menu_File)
        self.menuImport.setObjectName(_fromUtf8("menuImport"))
        self.menuExport = QtGui.QMenu(self.menu_File)
        self.menuExport.setObjectName(_fromUtf8("menuExport"))
        self.menu_Edit = QtGui.QMenu(self.menubar)
        self.menu_Edit.setObjectName(_fromUtf8("menu_Edit"))
        self.menu_View = QtGui.QMenu(self.menubar)
        self.menu_View.setObjectName(_fromUtf8("menu_View"))
        self.menu_Geometry = QtGui.QMenu(self.menubar)
        self.menu_Geometry.setObjectName(_fromUtf8("menu_Geometry"))
        self.menuConts = QtGui.QMenu(self.menubar)
        self.menuConts.setObjectName(_fromUtf8("menuConts"))
        self.menuAdd_2 = QtGui.QMenu(self.menuConts)
        self.menuAdd_2.setObjectName(_fromUtf8("menuAdd_2"))
        self.menuGrid = QtGui.QMenu(self.menubar)
        self.menuGrid.setObjectName(_fromUtf8("menuGrid"))
        self.menuAdd = QtGui.QMenu(self.menuGrid)
        self.menuAdd.setObjectName(_fromUtf8("menuAdd"))
        self.menuHelp = QtGui.QMenu(self.menubar)
        self.menuHelp.setObjectName(_fromUtf8("menuHelp"))
        MainWindow.setMenuBar(self.menubar)
        self.statusbar = QtGui.QStatusBar(MainWindow)
        self.statusbar.setObjectName(_fromUtf8("statusbar"))
        MainWindow.setStatusBar(self.statusbar)
        self.dw_grid_manager = QtGui.QDockWidget(MainWindow)
        self.dw_grid_manager.setFeatures(QtGui.QDockWidget.AllDockWidgetFeatures)
        self.dw_grid_manager.setObjectName(_fromUtf8("dw_grid_manager"))
        self.dockWidgetContents = QtGui.QWidget()
        self.dockWidgetContents.setObjectName(_fromUtf8("dockWidgetContents"))
        self.verticalLayout = QtGui.QVBoxLayout(self.dockWidgetContents)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.dw_grid_manager.setWidget(self.dockWidgetContents)
        MainWindow.addDockWidget(QtCore.Qt.DockWidgetArea(2), self.dw_grid_manager)
        self.dw_history = QtGui.QDockWidget(MainWindow)
        self.dw_history.setObjectName(_fromUtf8("dw_history"))
        self.dockWidgetContents_3 = QtGui.QWidget()
        self.dockWidgetContents_3.setObjectName(_fromUtf8("dockWidgetContents_3"))
        self.dw_history.setWidget(self.dockWidgetContents_3)
        MainWindow.addDockWidget(QtCore.Qt.DockWidgetArea(8), self.dw_history)
        self.dw_contour_manager = QtGui.QDockWidget(MainWindow)
        self.dw_contour_manager.setFloating(False)
        self.dw_contour_manager.setFeatures(QtGui.QDockWidget.AllDockWidgetFeatures)
        self.dw_contour_manager.setObjectName(_fromUtf8("dw_contour_manager"))
        self.dockWidgetContents_2 = QtGui.QWidget()
        self.dockWidgetContents_2.setObjectName(_fromUtf8("dockWidgetContents_2"))
        self.dw_contour_manager.setWidget(self.dockWidgetContents_2)
        MainWindow.addDockWidget(QtCore.Qt.DockWidgetArea(2), self.dw_contour_manager)
        self.dw_bnd_manager = QtGui.QDockWidget(MainWindow)
        self.dw_bnd_manager.setAllowedAreas(QtCore.Qt.AllDockWidgetAreas)
        self.dw_bnd_manager.setObjectName(_fromUtf8("dw_bnd_manager"))
        self.dockWidgetContents_4 = QtGui.QWidget()
        self.dockWidgetContents_4.setObjectName(_fromUtf8("dockWidgetContents_4"))
        self.dw_bnd_manager.setWidget(self.dockWidgetContents_4)
        MainWindow.addDockWidget(QtCore.Qt.DockWidgetArea(2), self.dw_bnd_manager)
        self.act_open = QtGui.QAction(MainWindow)
        self.act_open.setObjectName(_fromUtf8("act_open"))
        self.act_save = QtGui.QAction(MainWindow)
        self.act_save.setObjectName(_fromUtf8("act_save"))
        self.act_exit = QtGui.QAction(MainWindow)
        self.act_exit.setObjectName(_fromUtf8("act_exit"))
        self.act_undo = QtGui.QAction(MainWindow)
        self.act_undo.setObjectName(_fromUtf8("act_undo"))
        self.act_redo = QtGui.QAction(MainWindow)
        self.act_redo.setObjectName(_fromUtf8("act_redo"))
        self.act_unf_rect = QtGui.QAction(MainWindow)
        self.act_unf_rect.setObjectName(_fromUtf8("act_unf_rect"))
        self.act_grid_manager = QtGui.QAction(MainWindow)
        self.act_grid_manager.setCheckable(True)
        self.act_grid_manager.setChecked(True)
        self.act_grid_manager.setObjectName(_fromUtf8("act_grid_manager"))
        self.act_unite_grids = QtGui.QAction(MainWindow)
        self.act_unite_grids.setObjectName(_fromUtf8("act_unite_grids"))
        self.act_unf_circ = QtGui.QAction(MainWindow)
        self.act_unf_circ.setObjectName(_fromUtf8("act_unf_circ"))
        self.act_movrot = QtGui.QAction(MainWindow)
        self.act_movrot.setObjectName(_fromUtf8("act_movrot"))
        self.act_unf_ring = QtGui.QAction(MainWindow)
        self.act_unf_ring.setObjectName(_fromUtf8("act_unf_ring"))
        self.act_scale = QtGui.QAction(MainWindow)
        self.act_scale.setObjectName(_fromUtf8("act_scale"))
        self.act_commands_history = QtGui.QAction(MainWindow)
        self.act_commands_history.setCheckable(True)
        self.act_commands_history.setObjectName(_fromUtf8("act_commands_history"))
        self.act_copy_grids = QtGui.QAction(MainWindow)
        self.act_copy_grids.setObjectName(_fromUtf8("act_copy_grids"))
        self.act_remove_grids = QtGui.QAction(MainWindow)
        self.act_remove_grids.setObjectName(_fromUtf8("act_remove_grids"))
        self.act_contour_manager = QtGui.QAction(MainWindow)
        self.act_contour_manager.setCheckable(True)
        self.act_contour_manager.setChecked(True)
        self.act_contour_manager.setObjectName(_fromUtf8("act_contour_manager"))
        self.act_cont_rect = QtGui.QAction(MainWindow)
        self.act_cont_rect.setObjectName(_fromUtf8("act_cont_rect"))
        self.act_bnd_manager = QtGui.QAction(MainWindow)
        self.act_bnd_manager.setCheckable(True)
        self.act_bnd_manager.setObjectName(_fromUtf8("act_bnd_manager"))
        self.act_set_bc = QtGui.QAction(MainWindow)
        self.act_set_bc.setObjectName(_fromUtf8("act_set_bc"))
        self.act_new_bc = QtGui.QAction(MainWindow)
        self.act_new_bc.setObjectName(_fromUtf8("act_new_bc"))
        self.act_ex_cont = QtGui.QAction(MainWindow)
        self.act_ex_cont.setObjectName(_fromUtf8("act_ex_cont"))
        self.act_unite_conts = QtGui.QAction(MainWindow)
        self.act_unite_conts.setObjectName(_fromUtf8("act_unite_conts"))
        self.act_sep_conts = QtGui.QAction(MainWindow)
        self.act_sep_conts.setObjectName(_fromUtf8("act_sep_conts"))
        self.actionContour = QtGui.QAction(MainWindow)
        self.actionContour.setObjectName(_fromUtf8("actionContour"))
        self.act_enter_cont = QtGui.QAction(MainWindow)
        self.act_enter_cont.setObjectName(_fromUtf8("act_enter_cont"))
        self.act_imp_cont = QtGui.QAction(MainWindow)
        self.act_imp_cont.setObjectName(_fromUtf8("act_imp_cont"))
        self.act_imp_grid = QtGui.QAction(MainWindow)
        self.act_imp_grid.setObjectName(_fromUtf8("act_imp_grid"))
        self.act_exp_cont = QtGui.QAction(MainWindow)
        self.act_exp_cont.setObjectName(_fromUtf8("act_exp_cont"))
        self.act_exp_grid = QtGui.QAction(MainWindow)
        self.act_exp_grid.setObjectName(_fromUtf8("act_exp_grid"))
        self.act_clean_hist = QtGui.QAction(MainWindow)
        self.act_clean_hist.setObjectName(_fromUtf8("act_clean_hist"))
        self.act_clean_all = QtGui.QAction(MainWindow)
        self.act_clean_all.setObjectName(_fromUtf8("act_clean_all"))
        self.act_repeat = QtGui.QAction(MainWindow)
        self.act_repeat.setObjectName(_fromUtf8("act_repeat"))
        self.act_about = QtGui.QAction(MainWindow)
        self.act_about.setObjectName(_fromUtf8("act_about"))
        self.menuImport.addAction(self.act_imp_cont)
        self.menuImport.addAction(self.act_imp_grid)
        self.menuExport.addAction(self.act_exp_grid)
        self.menu_File.addAction(self.act_open)
        self.menu_File.addAction(self.act_save)
        self.menu_File.addAction(self.menuImport.menuAction())
        self.menu_File.addAction(self.menuExport.menuAction())
        self.menu_File.addSeparator()
        self.menu_File.addAction(self.act_exit)
        self.menu_Edit.addAction(self.act_undo)
        self.menu_Edit.addAction(self.act_redo)
        self.menu_Edit.addSeparator()
        self.menu_Edit.addAction(self.act_clean_hist)
        self.menu_Edit.addAction(self.act_clean_all)
        self.menu_View.addAction(self.act_grid_manager)
        self.menu_View.addAction(self.act_contour_manager)
        self.menu_View.addAction(self.act_bnd_manager)
        self.menu_View.addAction(self.act_commands_history)
        self.menu_Geometry.addAction(self.act_copy_grids)
        self.menu_Geometry.addAction(self.act_remove_grids)
        self.menu_Geometry.addSeparator()
        self.menu_Geometry.addAction(self.act_movrot)
        self.menu_Geometry.addAction(self.act_scale)
        self.menuAdd_2.addAction(self.act_cont_rect)
        self.menuConts.addAction(self.menuAdd_2.menuAction())
        self.menuConts.addSeparator()
        self.menuConts.addAction(self.act_unite_conts)
        self.menuConts.addAction(self.act_sep_conts)
        self.menuConts.addSeparator()
        self.menuConts.addAction(self.act_set_bc)
        self.menuConts.addAction(self.act_new_bc)
        self.menuAdd.addAction(self.act_unf_rect)
        self.menuAdd.addAction(self.act_unf_circ)
        self.menuAdd.addAction(self.act_unf_ring)
        self.menuGrid.addAction(self.menuAdd.menuAction())
        self.menuGrid.addSeparator()
        self.menuGrid.addAction(self.act_unite_grids)
        self.menuGrid.addAction(self.act_ex_cont)
        self.menuHelp.addAction(self.act_about)
        self.menubar.addAction(self.menu_File.menuAction())
        self.menubar.addAction(self.menu_View.menuAction())
        self.menubar.addAction(self.menu_Edit.menuAction())
        self.menubar.addAction(self.menuConts.menuAction())
        self.menubar.addAction(self.menuGrid.menuAction())
        self.menubar.addAction(self.menu_Geometry.menuAction())
        self.menubar.addAction(self.menuHelp.menuAction())

        self.retranslateUi(MainWindow)
        QtCore.QObject.connect(self.act_exit, QtCore.SIGNAL(_fromUtf8("triggered()")), MainWindow.close)
        QtCore.QObject.connect(self.act_grid_manager, QtCore.SIGNAL(_fromUtf8("toggled(bool)")), self.dw_grid_manager.setVisible)
        QtCore.QObject.connect(self.dw_grid_manager, QtCore.SIGNAL(_fromUtf8("visibilityChanged(bool)")), self.act_grid_manager.setChecked)
        QtCore.QObject.connect(self.act_commands_history, QtCore.SIGNAL(_fromUtf8("toggled(bool)")), self.dw_history.setVisible)
        QtCore.QObject.connect(self.dw_history, QtCore.SIGNAL(_fromUtf8("visibilityChanged(bool)")), self.act_commands_history.setChecked)
        QtCore.QObject.connect(self.act_contour_manager, QtCore.SIGNAL(_fromUtf8("toggled(bool)")), self.dw_contour_manager.setVisible)
        QtCore.QObject.connect(self.dw_contour_manager, QtCore.SIGNAL(_fromUtf8("visibilityChanged(bool)")), self.act_contour_manager.setChecked)
        QtCore.QObject.connect(self.act_bnd_manager, QtCore.SIGNAL(_fromUtf8("toggled(bool)")), self.dw_bnd_manager.setVisible)
        QtCore.QObject.connect(self.dw_bnd_manager, QtCore.SIGNAL(_fromUtf8("visibilityChanged(bool)")), self.act_bnd_manager.setChecked)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(_translate("MainWindow", "MainWindow", None))
        self.menu_File.setTitle(_translate("MainWindow", "&File", None))
        self.menuImport.setTitle(_translate("MainWindow", "&Import", None))
        self.menuExport.setTitle(_translate("MainWindow", "&Export", None))
        self.menu_Edit.setTitle(_translate("MainWindow", "C&ommands", None))
        self.menu_View.setTitle(_translate("MainWindow", "&View", None))
        self.menu_Geometry.setTitle(_translate("MainWindow", "Geo&metry", None))
        self.menuConts.setTitle(_translate("MainWindow", "&Contour", None))
        self.menuAdd_2.setTitle(_translate("MainWindow", "&Add uniform", None))
        self.menuGrid.setTitle(_translate("MainWindow", "&Grid", None))
        self.menuAdd.setTitle(_translate("MainWindow", "&Add uniform", None))
        self.menuHelp.setTitle(_translate("MainWindow", "Help", None))
        self.dw_grid_manager.setWindowTitle(_translate("MainWindow", "Grid Manager", None))
        self.dw_history.setWindowTitle(_translate("MainWindow", "Commands History", None))
        self.dw_contour_manager.setWindowTitle(_translate("MainWindow", "Contours Manager", None))
        self.dw_bnd_manager.setWindowTitle(_translate("MainWindow", "Boundary Manager", None))
        self.act_open.setText(_translate("MainWindow", "&Open ...", None))
        self.act_save.setText(_translate("MainWindow", "&Save ...", None))
        self.act_exit.setText(_translate("MainWindow", "E&xit", None))
        self.act_undo.setText(_translate("MainWindow", "&Undo", None))
        self.act_undo.setShortcut(_translate("MainWindow", "Ctrl+Z", None))
        self.act_redo.setText(_translate("MainWindow", "&Redo", None))
        self.act_redo.setShortcut(_translate("MainWindow", "Ctrl+Y", None))
        self.act_unf_rect.setText(_translate("MainWindow", "&Rectangle", None))
        self.act_grid_manager.setText(_translate("MainWindow", "&Grid Manager", None))
        self.act_unite_grids.setText(_translate("MainWindow", "&Unite grids", None))
        self.act_unf_circ.setText(_translate("MainWindow", "&Cirlce", None))
        self.act_movrot.setText(_translate("MainWindow", "&Move/Rotate", None))
        self.act_unf_ring.setText(_translate("MainWindow", "R&ing", None))
        self.act_scale.setText(_translate("MainWindow", "Sc&ale", None))
        self.act_commands_history.setText(_translate("MainWindow", "Commands &History", None))
        self.act_copy_grids.setText(_translate("MainWindow", "&Copy", None))
        self.act_remove_grids.setText(_translate("MainWindow", "&Remove", None))
        self.act_contour_manager.setText(_translate("MainWindow", "&Contour Manager", None))
        self.act_cont_rect.setText(_translate("MainWindow", "&Rectangle", None))
        self.act_bnd_manager.setText(_translate("MainWindow", "&Boundary Manager", None))
        self.act_set_bc.setText(_translate("MainWindow", "Assign &boundary types", None))
        self.act_new_bc.setText(_translate("MainWindow", "&Create boundary type", None))
        self.act_ex_cont.setText(_translate("MainWindow", "&Exclude conts", None))
        self.act_unite_conts.setText(_translate("MainWindow", "&Unite contours", None))
        self.act_sep_conts.setText(_translate("MainWindow", "&Separate/Simplify", None))
        self.actionContour.setText(_translate("MainWindow", "Contour", None))
        self.act_enter_cont.setText(_translate("MainWindow", "&Enter", None))
        self.act_imp_cont.setText(_translate("MainWindow", "&Contour", None))
        self.act_imp_grid.setText(_translate("MainWindow", "&Grid", None))
        self.act_exp_cont.setText(_translate("MainWindow", "&Contour", None))
        self.act_exp_grid.setText(_translate("MainWindow", "&Grid", None))
        self.act_clean_hist.setText(_translate("MainWindow", "Clean &history", None))
        self.act_clean_all.setText(_translate("MainWindow", "Clean all", None))
        self.act_repeat.setText(_translate("MainWindow", "&Repeat last", None))
        self.act_about.setText(_translate("MainWindow", "About", None))

import ComGridRes_rc
