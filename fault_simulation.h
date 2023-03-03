/*
Copyright 2022 Mohammad Riazati

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _FAULT_SIMULATION_H
#define _FAULT_SIMULATION_H

//arguments["fault-simulation"] = ""; //ACTIVE

#include "data-types.h"
#include <fstream>

template <typename T>
void fault_injection(T element_type, void *activation, int current_layer, int faulty_layer, int faulty_fmap, int faulty_bit) {
	if (arguments["fault-simulation"] != "ACTIVE") return;
	if (current_layer != faulty_layer) return;

	//FX_SIZE_W, FX_SIZE_I is not defined in eight-bit-int (int8_t) mode
	/*
	ap_fixed<FX_SIZE_W,FX_SIZE_I>* new_act = (ap_fixed<FX_SIZE_W,FX_SIZE_I>*)activation;
	ap_fixed<FX_SIZE_W,FX_SIZE_I> val = new_act[faulty_fmap];
	ap_uint<4> bit = FX_SIZE_W - faulty_bit - 1;				//fault loc
	ap_ufixed<FX_SIZE_W,FX_SIZE_I> sh = 1 << (FX_SIZE_I - 1);
	sh = sh >> bit;					//aligning fault loc in the param		
	new_act[faulty_fmap] = val ^ sh;
	*/

	//Not sure if it works for Xilinx datatypes. Should be tested.
	//Note: an extract from Xilinx source
		//struct ap_fixed_base : _AP_ROOT_TYPE<_AP_W, _AP_S> {
		//public:
			//static const int width = _AP_W;
			//static const int iwidth = _AP_I;

	//string temp_str = typeid(T).name();
	T* new_act = (T*)activation;
	T val = new_act[faulty_fmap];
	int size_of_t = sizeof(T) * 8;
	int bit = size_of_t - faulty_bit - 1;	//fault loc
	unsigned int sh = 1 << (size_of_t - 1);

	sh = sh >> bit;	//aligning fault loc in the param		
	new_act[faulty_fmap] = val ^ sh;
}

template <typename T, std::size_t x, std::size_t y, std::size_t z>
void fault_injection(T (*activation)[x][y][z], int current_layer, int faulty_layer, int faulty_fmap, int faulty_bit) {
	T temp_element = 0;
	T* temp_array = (T*)(activation);
	fault_injection(temp_element, temp_array, current_layer, faulty_layer, faulty_fmap, faulty_bit);
}

template <typename T, std::size_t x>
void fault_injection(T (*activation)[x], int current_layer, int faulty_layer, int faulty_fmap, int faulty_bit) {
	T temp_element = 0;
	T* temp_array = (T*)(activation);
	fault_injection(temp_element, temp_array, current_layer, faulty_layer, faulty_fmap, faulty_bit);
}

//confidence level
int fault_count_gen(int n_size) {
	if (arguments["fault-count"] != "") return stoi(arguments["fault-count"]);
	
	//constant parameters
	const double t2 = 3.8416; 	//t**2
	const double e2 = 0.0001;	//e**2
	const double p2 = 0.25;		//p*(1-p)
	int result = static_cast<int>(n_size / (1 + ((e2 * (n_size - 1)) / (t2 * p2))) + 1) / 10;

	return result;
}

vector<vector<int>> fault_list_generator_ASBF(int layers_count, vector<int> pixels_in_layers_lst, int fault_count, int word_length)	{	//ASBF: Activations Single Bit Fault
	//generate random faults and insert into fault_list

	vector<vector<int>> fault_list(fault_count);
	for (int i = 0; i < fault_count; i++) {
		fault_list[i] = vector<int>(3);
	}
	
	for (int fault_index = 0; fault_index < fault_count; fault_index++)	{
		int layer = rand() % (layers_count); //0 to layers_count - 1
		int fmap = rand() % (pixels_in_layers_lst[layer]); //0 to pixels_in_layers_lst - 1
		int bit = rand() % (word_length); //0 to bit_length - 1

		fault_list[fault_index][0] = layer + 1;
		fault_list[fault_index][1] = fmap;
		fault_list[fault_index][2] = bit;
	}
	
	return fault_list;
}

void ExportFaultListToCsv(vector<vector<int>> fault_list) {
	string file_name = "fault_list.csv";
	ofstream csv_file(file_name, ios::out);

	csv_file << "layer_number" << "," << "pixel" << "," << "bit" << endl;

	for (auto &row : fault_list) {
		csv_file << row[0] << "," << row[1] << "," << row[2] << endl;
	}
	csv_file.close();
	
	AddToLog(GetLogFileName(), "Fault list saved to " + file_name);
}

vector<vector<int>> ImportFaultListFromCsv(string file_name) {
	int layer, fmap, bit;
	vector<vector<int>> result;

	string file_location = FindInputFile(file_name);
	if (file_location == "") return result;
	ifstream csv_file(file_location);
	
	string temp_string;
	getline(csv_file, temp_string);

	char delimiter;

	while (true) {
		csv_file >> layer >> delimiter >> fmap >> delimiter >> bit;
		if (!csv_file) break;
		result.push_back({layer, fmap, bit});
	}

	csv_file.close();

	AddToLog(GetLogFileName(), "\nUsing existing fault list");
	AddToLog(GetLogFileName(), "Fault list loaded from " + file_location + "\n");

	return result;
}

vector<vector<int>> CreateFaultInjectionList(int layers_count, vector<int> pixels_in_layers_lst, int word_length) {
	auto temp = ImportFaultListFromCsv("fault_list.csv");
	if (temp.size() > 0) {
		//AddToLog(GetLogFileName(), "Using existing fault_list.csv");
		return temp;
	}

	//int layers_count = 7;
 	//int pixels_in_layers_lst[] = {24*24*6, 12*12*6, 8*8*16, 4*4*16, 256, 120, 84};
	
	int pixels_count = 0;
	for (int l = 0; l < layers_count; l++) pixels_count = pixels_count + pixels_in_layers_lst[l];
	
	//const int word_length = 16;
	int n_size = word_length * pixels_count;	//word_length = 16, in the current implementation
	int fault_count = fault_count_gen(n_size);
	
	auto result = fault_list_generator_ASBF(layers_count, pixels_in_layers_lst, fault_count, word_length);
	ExportFaultListToCsv(result);
	return result;
}

#endif //_FAULT_SIMULATION_H