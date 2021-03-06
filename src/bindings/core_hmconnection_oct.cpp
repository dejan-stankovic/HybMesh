#include "octave/oct.h"
#include <iostream>
#include "client.h"
#include <stdexcept>
#include <string>

static void check_err(int err, const char* msg){
	if (err == 0){
		throw std::runtime_error(msg);
	}
}

static octave_value_list oct_require_connection(const octave_value_list& args){
	int id;
	charNDArray server_path_ar = args(1).char_array_value();
	char* server_path = server_path_ar.fortran_vec();
	std::string server_path2(server_path, server_path+server_path_ar.numel());
	int err = HybmeshClientToServer_new(server_path2.c_str(), &id);
	check_err(err, "failed to open a hybmesh subprocess");
	NDArray ret(dim_vector(1, 1));
	ret(0) = (double)id;
	return octave_value(ret);
}

static octave_value_list oct_get_signal(const octave_value_list& args){
	int id = args(1).array_value()(0);
	char sig;
	int err = HybmeshClientToServer_get_signal(id, &sig);
	check_err(err, "failed to read a signal from the pipe");
	charNDArray ret(dim_vector(1, 1));
	ret(0) = sig;
	return octave_value(ret);
}

static octave_value_list oct_send_signal(const octave_value_list& args){
	int id = args(1).array_value()(0);
	char sig = args(2).char_array_value()(0);
	int err = HybmeshClientToServer_send_signal(id, sig);
	check_err(err, "failed to send a signal to the pipe");
	return octave_value_list();
}

static octave_value_list oct_get_data(const octave_value_list& args){
	int id = args(1).array_value()(0);
	int sz;

	int err1 = HybmeshClientToServer_get_data1(id, &sz);
	check_err(err1, "failed to read data from the pipe");

	uint8NDArray ret(dim_vector(1, sz));
	int err2 = HybmeshClientToServer_get_data2(id, sz, (char*)ret.fortran_vec());
	check_err(err2, "failed to read data from the pipe");

	return octave_value(ret);
}

static octave_value_list oct_send_data(const octave_value_list& args){
	int id = args(1).array_value()(0);
	charNDArray data = args(2).char_array_value();
	int sz = data.numel();
	int err = HybmeshClientToServer_send_data(id, sz, data.fortran_vec());
	check_err(err, "failed to send data to the pipe");
	return octave_value_list();
}

static octave_value_list oct_break_connection(const octave_value_list& args){
	int id = args(1).array_value()(0);
	int err = HybmeshClientToServer_delete(id);
	check_err(err, "failed to close hybmesh connection");
	return octave_value_list();
}

// arg0 - procedure code:
//    1 - "require_connection", (char*) => connection id
//    2 - "get_signal",         (id) => char
//    3 - "send_signal",        (id, char)
//    4 - "get_data",           (id) => uint8 array
//    5 - "send_data",          (id, char*)
//    6 - "break_connection",   (id)
DEFUN_DLD(core_hmconnection_oct, args, nargsout, ""){
	int code = args(0).array_value()(0);
	switch (code){
		case 1: return oct_require_connection(args);
		case 2: return oct_get_signal(args);
		case 3: return oct_send_signal(args);
		case 4: return oct_get_data(args);
		case 5: return oct_send_data(args);
		case 6: return oct_break_connection(args);
		default: check_err(0, "unknown instruction");
	};
	return octave_value_list();
}
