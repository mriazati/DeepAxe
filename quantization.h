/*
Copyright 2022 Mohammad Riazati

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _QUANTIZATION_H
#define _QUANTIZATION_H

#include "data-types.h"

#ifndef __linux__
#pragma optimize("", off) //to avoid an error because of a bug in C++ compiler that happens in release make
#endif

void quantize_layer_output(void *layer_data_base, void *layer_data_quantized, int size, int config = 0) {
	bool enabled = true;

	DataType temp_base;
	
	if (config == 1) { //Output: both large data types
		for (int i = 0; i < size; ++i) {
			temp_base = ((DataType*)layer_data_base)[i];
			((DataType*)layer_data_quantized)[i] = DataType(temp_base);
		}

		return;
	}
	
	if (!enabled)	{
		for (int i = 0; i < size; ++i) {
			temp_base = ((DataType*)layer_data_base)[i];
			((DataType_short*)layer_data_quantized)[i] = DataType_short(temp_base);
		}

		return;
	}

	int quantization_size = sizeof(DataType_short) * 8;

	DataType min = 0, max = 0;
	
	for (int i = 0; i < size; i++) {
		if (i == 0) {
			min = max = ((DataType*)layer_data_base)[0];
		}
		else {
			if (min > ((DataType*)layer_data_base)[i])
				min = ((DataType*)layer_data_base)[i];
			if (max < ((DataType*)layer_data_base)[i])
				max = ((DataType*)layer_data_base)[i];
		}
	}

	string method = "one";
	//string method = "two";

	if (method == "one") {
		DataType S = (max - min) / (2 * quantization_size - 1);
		if (S == 0) S = 1;

		//Added on 2022-10-22
		if (true) {
			pair<long long, long long> possible_range;
			possible_range.first = -(long long)pow(2, sizeof(DataType_short) * 8);
			possible_range.second = (long long)pow(2, sizeof(DataType_short) * 8) - 1;
			if (min >= possible_range.first && max <= possible_range.second) 
				S = 1;
		}

		for (int i = 0; i < size; ++i) {
			temp_base = ((DataType*)layer_data_base)[i];
			((DataType_short*)layer_data_quantized)[i] = DataType_short(temp_base / S);
		}
	}
	else {
		//method two
		DataType new_max = (max - min) / 2;
		DataType new_min = -new_max;
		DataType shift = new_min - min;
		DataType divide;

		pair<long long, long long> possible_range;
		possible_range.first = -(long long)pow(2, sizeof(DataType_short) * 8);
		possible_range.second = (long long)pow(2, sizeof(DataType_short) * 8) - 1;
		if (min >= possible_range.first && max <= possible_range.second) {
			shift = 0;
			divide = 1;
		}
		else {
			if (max == min) 
				divide = 1;
			else 
				divide = (DataType) (2 * (max - min) / (possible_range.second - possible_range.first) + 1);
		}

		for (int i = 0; i < size; ++i) {
			temp_base = ((DataType*)layer_data_base)[i];
			temp_base += shift;
			temp_base /= divide;
			DataType_short temp_result = DataType_short(temp_base);
			((DataType_short*)layer_data_quantized)[i] = temp_result;
		}
	}

	return;
}
#ifndef __linux__
#pragma optimize("", on) 
#endif

#endif //_QUANTIZATION_H
