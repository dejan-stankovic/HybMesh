#include <string.h>
#include "cport_hmxml.h"
#include "c2cpp_helper.hpp"
#include "hmxmlreader.hpp"
#include "import2d_hm.hpp"
#include "import3d_hm.hpp"

int hmxml_open_doc(const char* fname, void** doc, void** root){
	try{
		HMXML::ReaderA* doc_ = new HMXML::ReaderA(fname, "</HybMeshData>");
		HMXML::Reader* root_ = new HMXML::Reader(*doc_, doc_->_nd);
		*doc = doc_;
		*root = root_;
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int hmxml_new_doc(void** doc, void** root){
	try{
		HMXML::ReaderA* doc_ = HMXML::ReaderA::pcreate("HybMeshData");
		HMXML::Reader* root_ = new HMXML::Reader(*doc_, doc_->_nd);
		*doc = doc_;
		*root = root_;
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int hmxml_free_node(void* node){
	try{
		if (node != nullptr) delete static_cast<HMXML::Reader*>(node);
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int hmxml_free_doc(void* doc){
	try{
		if (doc != nullptr) delete static_cast<HMXML::ReaderA*>(doc);
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int hmxml_query(void* node, const char* query, int* rnum, void*** ret){
	try{
		HMXML::Reader* wr = static_cast<HMXML::Reader*>(node);
		vector<HMXML::Reader> fnd = wr->findall_by_path(query);
		c2cpp::to_ppp(fnd, rnum, ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int hmxml_write(void* doc, const char* filename){
	try{
		static_cast<HMXML::ReaderA*>(doc)->write(filename);
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int hmxml_purged_string(void* doc, char** ret){
	try{
		HMXML::ReaderA* wr = static_cast<HMXML::ReaderA*>(doc);
		HMXML::Reader copy = HMXML::Reader::copy_to_root(*wr);
		for (auto&n: copy.findall_by_path(".//GRID2D")) n.unlink_node();
		for (auto&n: copy.findall_by_path(".//GRID3D")) n.unlink_node();
		for (auto&n: copy.findall_by_path(".//CONTOUR2D")) n.unlink_node();
		for (auto&n: copy.findall_by_path(".//SURFACE3D")) n.unlink_node();
		c2cpp::to_char_string(copy.tostring(), ret);
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int read_contour2(void* doc, void* node, void** obj, char* name){
	try{
		auto wr = static_cast<HMXML::ReaderA*>(doc);
		auto sn = static_cast<HMXML::Reader*>(node);
		std::string nm = sn->attribute(".", "name");
		HM2D::Import::EColReader reader(wr, sn);
		*obj = reader.result.release();
		strcpy(name, nm.c_str());
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int read_grid2(void* doc, void* node, void** obj, char* name){
	try{
		auto wr = static_cast<HMXML::ReaderA*>(doc);
		auto sn = static_cast<HMXML::Reader*>(node);
		std::string nm = sn->attribute(".", "name");
		HM2D::Import::GridReader reader(wr, sn);
		*obj = reader.result.release();
		strcpy(name, nm.c_str());
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int read_surface3(void* doc, void* node, void** obj, char* name){
	try{
		auto wr = static_cast<HMXML::ReaderA*>(doc);
		auto sn = static_cast<HMXML::Reader*>(node);
		std::string nm = sn->attribute(".", "name");
		auto ret = HM3D::Import::ReadHMC(wr, sn);
		c2cpp::to_pp(ret->result->surface, obj);
		strcpy(name, nm.c_str());
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}

int read_grid3(void* doc, void* node, void** obj, char* name, hmcport_callback cb){
	try{
		auto wr = static_cast<HMXML::ReaderA*>(doc);
		auto sn = static_cast<HMXML::Reader*>(node);
		std::string nm = sn->attribute(".", "name");
		auto ret = HM3D::Import::ReadHMG.WithCallback(cb, wr, sn);
		c2cpp::to_pp(ret->result->grid, obj);
		strcpy(name, nm.c_str());
		return HMSUCCESS;
	} catch (std::exception& e){
		add_error_message(e.what());
		return HMERROR;
	}
}
