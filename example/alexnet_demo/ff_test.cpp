//This is the main function of a LeNet-5 model based application.
//Application description: LeNet-5 image recognition with std image.
//Using stb_image instead of OpenCV to eliminate library dependency.

#include <iostream>
#include <cstring>
//#include <cmath>
//#include <vector>
#include <fstream>
#include <algorithm>
//#include <iterator>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <time.h>
#include <ap_fixed.h>
#include "config.h"
#include "construct_net.h"
#include "../../fpga_cnn/data_type.h"
#include "../../fpga_cnn/image_converter.h"
#include "../../fpga_cnn/weight_bias.h"
#include "../../fpga_cnn/conv_layer.h"
//#include "../fpga_cnn/data_quantize.h"
#include "../../fpga_cnn/read_mnist.h"
#include "../../fpga_cnn/softmax.h"
#include "../../fpga_cnn/predict.h"
#include "../../fpga_cnn/accuracy.h"
#include "../../fpga_cnn/pow_function.h"
#include "../../fpga_cnn/resize_image.h"
#include "get_config_params.h"
//#include "../fpga_cnn/set_mean.h"

using namespace std;

const unsigned char * loadfile(const std::string &file, int &size)
{
	std::ifstream fs(file.c_str(), std::ios::binary);
	fs.seekg(0, std::ios::end);
	size = fs.tellg();
	char * data = new char[size];
	fs.seekg(0);
	fs.read(data, sizeof(char) * size);
	fs.close();
	return (unsigned char *)data;
}

int main() {
	//net weight src *****************************
	const char* weight_src = "net_weights.txt";

	//load mean file *****************************
	ifstream ifs1("net_mean.txt");
	float channel_mean[3] = { 0 };
	string str1;
	string y1 = "[";
	string y2 = "]";

	if (!ifs1) {
		cout << "mean data file not found !" << endl;
		getchar();
	}
	int index = 0;
	while (ifs1 >> str1) {
		int p1 = str1.find(y1, 0);  // p为找到的位置,-1为未找到
		if (p1 >= 0) {
			str1.erase(p1, y1.length());
		}
		int p2 = str1.find(y2, 0);  // p为找到的位置,-1为未找到
		if (p2 >= 0) {
			str1.erase(p2, y2.length());
		}
		float f = atof(str1.c_str());
		channel_mean[index] = f;
		index++;
	}
	ifs1.close();

	//load val (image_name,image_class) set file *****************************
	ifstream ifs("val.txt");
	string val_name_class[10][2];
	string str;

	if (!ifs) {
		cout << "val data file not found !" << endl;
		getchar();
	}
	int num = 0;
	while (ifs >> str&&num<20) {//num:4 pair (image_name,image_class)
		if (num % 2 == 0) {//image_name
			val_name_class[num / 2][0] = str;
		}
		else {//image_class
			val_name_class[num / 2][1] = str;
		}
		num++;
	}
	ifs.close();


	//load val image file *****************************
#if _KERNEL_DEBUG_
#if _HLS_MODE_
	string image_dir = "ILSVRC2012_val_00000003.JPEG";//validation dataset dir
#else
	string image_dir = "./ILSVRC2012_img_val/ILSVRC2012_val_00000003.JPEG";
#endif
	float in_data_3D_channel_swap[3][375][500] = { 0 };
	//input data array
	float in_data_3D[3][227][227] = { 0 };
	data_type in_data_3D_int[3][227][227] = { 0 };
	int w;
	int h;
	int channels;
	int size;
	const unsigned char * data = loadfile(image_dir, size);
	const unsigned char * image_orig = stbi_load_from_memory(data, size, &w, &h, &channels, 3);
	//STBI_default = 0, // only used for req_comp 
	//STBI_grey = 1,
	//STBI_grey_alpha = 2,
	//STBI_rgb = 3,
	//STBI_rgb_alpha = 4

	for (int i = 0; i < 3; i++) {
		for (int j = i; j < w * h * 3; j += 3) {
			in_data_3D_channel_swap[2 - i][j / (w * 3)][(j % (w * 3) - i) / 3] = (float)image_orig[j];//range:0--255
		}
	}

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < h; j++) {
			for (int k = 0; k < w; k++) {
				in_data_3D_channel_swap[i][j][k] /= 255;//range:0--1
			}
		}
	}

	resize_image(in_data_3D_channel_swap, h, w, in_data_3D);//in_data after crop

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 227; j++) {
			for (int k = 0; k < 227; k++) {
				in_data_3D[i][j][k] = in_data_3D[i][j][k] * 255 - channel_mean[i];
			}
		}
	}

	cout << "testing point 2" << endl;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 227; j++) {
			for (int k = 0; k < 227; k++) {
				in_data_3D_int[i][j][k] = (data_type)in_data_3D[i][j][k];
			}
		}
	}

	data_type output_min = (data_type)0;
    data_type output_max = (data_type)0;

	ofstream indata;
	indata.open("in_data_crop_mean.txt", ios::app);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 227; j++) {
			for (int k = 0; k < 227; k++) {
				indata << in_data_3D_int[i][j][k] << " ";
				if(in_data_3D_int[i][j][k] < output_min){
					output_min = in_data_3D_int[i][j][k];
				}
				if(in_data_3D_int[i][j][k] > output_max){
					output_max = in_data_3D_int[i][j][k];
				}
			}
			indata << endl;
		}
		indata << endl;
	}
	indata << endl;
	indata.close();
	ofstream output_range;
	output_range.open("input_layer_output_range_a.txt", ios::app);
    output_range << "output range from input layer .........................." << endl;
    output_range << output_min << "~~~" << output_max << endl;
    output_range << endl;
    output_range.close();
#endif

#if _BATCH_MODE_
	//load val image set file *****************************
	string root_dir = "./ILSVRC2012_img_val/";
	float imagenet_test_data_channel_swap[10][3][500][500] = { 0 };
	float imagenet_test_data[10][3][227][227] = { 0 };
	data_type imagenet_test_data_int[10][3][227][227] = { 0 };
	for (int image_num = 0; image_num < 10; image_num++) {
		string image_dir = root_dir + val_name_class[image_num][0];
		int w;
		int h;
		int channels;
		int size;
		const unsigned char * data = loadfile(image_dir, size);
		const unsigned char * image_orig = stbi_load_from_memory(data, size, &w, &h, &channels, 3);
		//STBI_default = 0, // only used for req_comp 
		//STBI_grey = 1,
		//STBI_grey_alpha = 2,
		//STBI_rgb = 3,
		//STBI_rgb_alpha = 4

		for (int i = 0; i < 3; i++) {
			for (int j = i; j < w * h * 3; j += 3) {
				imagenet_test_data_channel_swap[image_num][2 - i][j / (w * 3)][(j % (w * 3) - i) / 3] = (float)image_orig[j];//range:0--255
			}
		}

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < h; j++) {
				for (int k = 0; k < w; k++) {
					imagenet_test_data_channel_swap[image_num][i][j][k] /= 255;//range:0--1
				}
			}
		}

		resize_image(imagenet_test_data_channel_swap[image_num], h, w, imagenet_test_data[image_num]);//in_data after crop

		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 227; j++) {
				for (int k = 0; k < 227; k++) {
					imagenet_test_data[image_num][i][j][k] = imagenet_test_data[image_num][i][j][k] * 255 - channel_mean[i];
				}
			}
		}

		for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 227; j++) {
			for (int k = 0; k < 227; k++) {
				imagenet_test_data_int[image_num][i][j][k] = (data_type)imagenet_test_data[image_num][i][j][k];
			}
		}
	}

		ofstream indata;
		indata.open("in_data_crop_mean.txt", ios::app);
		indata << "image_num: " << image_num << "**********" << endl;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 227; j++) {
				for (int k = 0; k < 227; k++) {
					indata << imagenet_test_data_int[image_num][i][j][k] << " ";
				}
				indata << endl;
			}
			indata << endl;
		}
		indata << endl;
		indata.close();
	}
#endif

	char tan_h = 't';
	char relu = 'r';
	char none = 'i';
	int in_number_conv = 0;  // number of convolutional layer
	int in_number_fc = 0;// number of fully_connected layer
	int in_number_pooling = 0;

	//std_vec_t data_in;
	//float in_data_1[3][227][227] = { 0 };
	////convert image to data matrix
	//	const std::string filename = ".\\example\\cat.jpg";
	//	convert_image(filename, 0, 1, 227, 227, data_in);
	//	for (int i = 0; i < data_in.size(); i++) {
	//		in_data_1[i/51529][(i % 51529) / 227][(i % 51529) % 227] = data_in[i];
	//	}

#if _BATCH_MODE_
	/*float mnist_train_data[60000][1][32][32];
	float mnist_train_label[60000][10] = { 0 };
	float mnist_test_data[10000][1][32][32];
	float mnist_test_label[10000][10] = { 0 };
	getSrcData(mnist_train_data, mnist_train_label, mnist_test_data, mnist_test_label);
	cout << "getSrcData end!!!!!!!!!!!!!!" << endl;*/
#endif

	// Prepare weights and bias for convolution layer 1
	float conv_1_weight2D[288][11][11] = { 0 };
	data_type_w          conv_1_weight2D_int[288][11][11] = {0};
	float conv_1_bias2D[96] = { 0 };
	data_type_w          conv_1_bias2D_int[96] = {0};
	load_weight_conv(
		weight_src,
		conv_1_weight2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	load_bias_conv(
		weight_src,
		conv_1_bias2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	in_number_conv++;

	data_type_w weight_min = (data_type)0;
    data_type_w weight_max = (data_type)0;

	for(int i = 0; i < 288; i++){
        for(int j = 0; j < 11; j++){
            for(int k = 0; k < 11; k++){
//                conv_1_weight2D[i][j][k] = round(conv_1_weight2D[i][j][k] * 100)/100;
//                if(conv_1_weight2D[i][j][k] >= -0.04 && conv_1_weight2D[i][j][k] <= 0.04){
//                    conv_1_weight2D[i][j][k] = 0;
//                }
                conv_1_weight2D_int[i][j][k] = (data_type_w)(conv_1_weight2D[i][j][k]);
                if(conv_1_weight2D_int[i][j][k] < weight_min){
					weight_min =conv_1_weight2D_int[i][j][k] ;
				}
				if(conv_1_weight2D_int[i][j][k] > weight_max){
					weight_max =conv_1_weight2D_int[i][j][k] ;
				}
            }
        }
    }
    ofstream weight_range;
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from conv_1 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 96; i++){
//        conv_1_bias2D[i] = round(conv_1_bias2D[i] * 100)/100;
        conv_1_bias2D_int[i] = (data_type_w)(conv_1_bias2D[i]);
        cout << conv_1_bias2D_int[i] << "  " ;
    }
    cout << endl;

	// Prepare weights and bias for convolution layer 2
	float conv_2_weight2D[12288][5][5] = { 0 };
	data_type_w          conv_2_weight2D_int[12288][5][5] = {0};
	float conv_2_bias2D[256] = { 0 };
	data_type_w          conv_2_bias2D_int[256] = {0};

	load_weight_conv(
		weight_src,
		conv_2_weight2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	load_bias_conv(
		weight_src,
		conv_2_bias2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	in_number_conv++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 12288; i++){
        for(int j = 0; j < 5; j++){
            for(int k = 0; k < 5; k++){
//                conv_1_weight2D[i][j][k] = round(conv_1_weight2D[i][j][k] * 100)/100;
//                if(conv_1_weight2D[i][j][k] >= -0.04 && conv_1_weight2D[i][j][k] <= 0.04){
//                    conv_1_weight2D[i][j][k] = 0;
//                }
                conv_2_weight2D_int[i][j][k] = (data_type_w)(conv_2_weight2D[i][j][k]);
                if(conv_2_weight2D_int[i][j][k] < weight_min){
					weight_min =conv_2_weight2D_int[i][j][k] ;
				}
				if(conv_2_weight2D_int[i][j][k] > weight_max){
					weight_max =conv_2_weight2D_int[i][j][k] ;
				}            
			}
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from conv_2 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 256; i++){
//        conv_1_bias2D[i] = round(conv_1_bias2D[i] * 100)/100;
        conv_2_bias2D_int[i] = (data_type_w)(conv_2_bias2D[i]);
        cout << conv_2_bias2D_int[i] << "  " ;
    }
    cout << endl;

	// Prepare weights and bias for convolution layer 3
	float conv_3_weight2D[98304][3][3] = { 0 };
	data_type_w          conv_3_weight2D_int[98304][3][3] = { 0 };//
	float conv_3_bias2D[384] = { 0 };
	data_type_w          conv_3_bias2D_int[384] = {0};

	load_weight_conv(
		weight_src,
		conv_3_weight2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	load_bias_conv(
		weight_src,
		conv_3_bias2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	in_number_conv++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 98304; i++){
        for(int j = 0; j < 3; j++){
            for(int k = 0; k < 3; k++){
//                conv_1_weight2D[i][j][k] = round(conv_1_weight2D[i][j][k] * 100)/100;
//                if(conv_1_weight2D[i][j][k] >= -0.04 && conv_1_weight2D[i][j][k] <= 0.04){
//                    conv_1_weight2D[i][j][k] = 0;
//                }
                conv_3_weight2D_int[i][j][k] = (data_type_w)(conv_3_weight2D[i][j][k]);
                if(conv_3_weight2D_int[i][j][k] < weight_min){
					weight_min =conv_3_weight2D_int[i][j][k] ;
				}
				if(conv_3_weight2D_int[i][j][k] > weight_max){
					weight_max =conv_3_weight2D_int[i][j][k] ;
				} 
            }
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from conv_3 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 384; i++){
//        conv_1_bias2D[i] = round(conv_1_bias2D[i] * 100)/100;
        conv_3_bias2D_int[i] = (data_type_w)(conv_3_bias2D[i]);
        cout << conv_3_bias2D_int[i] << "  " ;
    }
    cout << endl;

	// Prepare weights and bias for convolution layer 4
	float conv_4_weight2D[73728][3][3] = { 0 };
	data_type_w          conv_4_weight2D_int[73728][3][3] = { 0 };//
	float conv_4_bias2D[384] = { 0 };
	data_type_w          conv_4_bias2D_int[384] = {0};

	load_weight_conv(
		weight_src,
		conv_4_weight2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	load_bias_conv(
		weight_src,
		conv_4_bias2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	in_number_conv++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 73728; i++){
        for(int j = 0; j < 3; j++){
            for(int k = 0; k < 3; k++){
//                conv_1_weight2D[i][j][k] = round(conv_1_weight2D[i][j][k] * 100)/100;
//                if(conv_1_weight2D[i][j][k] >= -0.04 && conv_1_weight2D[i][j][k] <= 0.04){
//                    conv_1_weight2D[i][j][k] = 0;
//                }
                conv_4_weight2D_int[i][j][k] = (data_type_w)(conv_4_weight2D[i][j][k]);
                if(conv_4_weight2D_int[i][j][k] < weight_min){
					weight_min =conv_4_weight2D_int[i][j][k] ;
				}
				if(conv_4_weight2D_int[i][j][k] > weight_max){
					weight_max =conv_4_weight2D_int[i][j][k] ;
				} 
            }
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from conv_4 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 384; i++){
//        conv_1_bias2D[i] = round(conv_1_bias2D[i] * 100)/100;
        conv_4_bias2D_int[i] = (data_type_w)(conv_4_bias2D[i]);
        cout << conv_4_bias2D_int[i] << "  " ;
    }
    cout << endl;

	// Prepare weights and bias for convolution layer 5
	float conv_5_weight2D[49152][3][3] = { 0 };
	data_type_w          conv_5_weight2D_int[49152][3][3] = {0};
	float conv_5_bias2D[256] = { 0 };
	data_type_w          conv_5_bias2D_int[256] = {0};

	load_weight_conv(
		weight_src,
		conv_5_weight2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	load_bias_conv(
		weight_src,
		conv_5_bias2D,
		weight_bias_record,
		nn_channel_size_conv,
		nn_in_number_conv,
		nn_out_number_conv,
		in_number_conv);
	in_number_conv++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 49152; i++){
        for(int j = 0; j < 3; j++){
            for(int k = 0; k < 3; k++){
//                conv_1_weight2D[i][j][k] = round(conv_1_weight2D[i][j][k] * 100)/100;
//                if(conv_1_weight2D[i][j][k] >= -0.04 && conv_1_weight2D[i][j][k] <= 0.04){
//                    conv_1_weight2D[i][j][k] = 0;
//                }
                conv_5_weight2D_int[i][j][k] = (data_type_w)(conv_5_weight2D[i][j][k]);
                if(conv_5_weight2D_int[i][j][k] < weight_min){
					weight_min =conv_5_weight2D_int[i][j][k] ;
				}
				if(conv_5_weight2D_int[i][j][k] > weight_max){
					weight_max =conv_5_weight2D_int[i][j][k] ;
				} 
            }
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from conv_5 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 256; i++){
//        conv_1_bias2D[i] = round(conv_1_bias2D[i] * 100)/100;
        conv_5_bias2D_int[i] = (data_type_w)(conv_5_bias2D[i]);
        cout << conv_5_bias2D_int[i] << "  " ;
    }
    cout << endl;

	// Prepare weights and bias for fc layer 6
	float fc_6_weight2D[1048576][6][6] = { 0 };
	data_type_w   fc_6_weight2D_int[1048576][6][6] = {0};
	float fc_6_bias2D[4096] = { 0 };
	data_type_w   fc_6_bias2D_int[4096] = {0};
	load_weight_fc(
		weight_src,
		fc_6_weight2D,
		weight_bias_record,
		nn_channel_size_fc,
		nn_in_number_fc,
		nn_out_number_fc,
		in_number_fc);
	load_bias_fc(
		weight_src,
		fc_6_bias2D,
		weight_bias_record,
		nn_channel_size_fc,
		nn_in_number_fc,
		nn_out_number_fc,
		in_number_fc);
	in_number_fc++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 1048576; i++){
        for(int j = 0; j < 6; j++){
            for(int k = 0; k < 6; k++){
//                fc_1_weight2D[i][j][k] = round(fc_1_weight2D[i][j][k] * 100)/100;
//                if(fc_1_weight2D[i][j][k] >= -0.05 && fc_1_weight2D[i][j][k] <= 0.05){
//                    fc_1_weight2D[i][j][k] = 0;
//                }
                fc_6_weight2D_int[i][j][k] = (data_type_w)(fc_6_weight2D[i][j][k]);
                if(fc_6_weight2D_int[i][j][k] < weight_min){
					weight_min =fc_6_weight2D_int[i][j][k] ;
				}
				if(fc_6_weight2D_int[i][j][k] > weight_max){
					weight_max =fc_6_weight2D_int[i][j][k] ;
				} 
            }
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from fc_6 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 4096; i++){
//        fc_1_bias2D[i] = round(fc_1_bias2D[i] * 100)/100;
        fc_6_bias2D_int[i] = (data_type_w)(fc_6_bias2D[i]);
    }

	// Prepare weights and bias for fc layer 7
	float fc_7_weight2D[16777216][1][1] = { 0 };
	data_type_w   fc_7_weight2D_int[16777216][1][1] = {0};
	float fc_7_bias2D[4096] = { 0 };
	data_type_w   fc_7_bias2D_int[4096] = {0};
	load_weight_fc(
		weight_src,
		fc_7_weight2D,
		weight_bias_record,
		nn_channel_size_fc,
		nn_in_number_fc,
		nn_out_number_fc,
		in_number_fc);
	load_bias_fc(
		weight_src,
		fc_7_bias2D,
		weight_bias_record,
		nn_channel_size_fc,
		nn_in_number_fc,
		nn_out_number_fc,
		in_number_fc);
	in_number_fc++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 16777216; i++){
        for(int j = 0; j < 1; j++){
            for(int k = 0; k < 1; k++){
//                fc_1_weight2D[i][j][k] = round(fc_1_weight2D[i][j][k] * 100)/100;
//                if(fc_1_weight2D[i][j][k] >= -0.05 && fc_1_weight2D[i][j][k] <= 0.05){
//                    fc_1_weight2D[i][j][k] = 0;
//                }
                fc_7_weight2D_int[i][j][k] = (data_type_w)(fc_7_weight2D[i][j][k]);
                if(fc_7_weight2D_int[i][j][k] < weight_min){
					weight_min =fc_7_weight2D_int[i][j][k] ;
				}
				if(fc_7_weight2D_int[i][j][k] > weight_max){
					weight_max =fc_7_weight2D_int[i][j][k] ;
				} 
            }
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from fc_7 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 4096; i++){
//        fc_1_bias2D[i] = round(fc_1_bias2D[i] * 100)/100;
        fc_7_bias2D_int[i] = (data_type_w)(fc_7_bias2D[i]);
    }

	// Prepare weights and bias for fc layer 8
	float fc_8_weight2D[4096000][1][1] = { 0 };
	data_type_w   fc_8_weight2D_int[4096000][1][1] = {0};
	float fc_8_bias2D[1000] = { 0 };
	data_type_w   fc_8_bias2D_int[1000] = {0};
	load_weight_fc(
		weight_src,
		fc_8_weight2D,
		weight_bias_record,
		nn_channel_size_fc,
		nn_in_number_fc,
		nn_out_number_fc,
		in_number_fc);
	load_bias_fc(
		weight_src,
		fc_8_bias2D,
		weight_bias_record,
		nn_channel_size_fc,
		nn_in_number_fc,
		nn_out_number_fc,
		in_number_fc);
	in_number_fc++;

	weight_min = (data_type)0;
    weight_max = (data_type)0;

	for(int i = 0; i < 4096000; i++){
        for(int j = 0; j < 1; j++){
            for(int k = 0; k < 1; k++){
//                fc_1_weight2D[i][j][k] = round(fc_1_weight2D[i][j][k] * 100)/100;
//                if(fc_1_weight2D[i][j][k] >= -0.05 && fc_1_weight2D[i][j][k] <= 0.05){
//                    fc_1_weight2D[i][j][k] = 0;
//                }
                fc_8_weight2D_int[i][j][k] = (data_type_w)(fc_8_weight2D[i][j][k]);
                if(fc_8_weight2D_int[i][j][k] < weight_min){
					weight_min =fc_8_weight2D_int[i][j][k] ;
				}
				if(fc_8_weight2D_int[i][j][k] > weight_max){
					weight_max =fc_8_weight2D_int[i][j][k] ;
				} 
            }
        }
    }
    weight_range.open("weight_range_a.txt", ios::app);
    weight_range << "weight range from fc_8 layer .........................." << endl;
    weight_range << weight_min << "~~~" << weight_max << endl;
    weight_range << endl;
    weight_range.close();

    for(int i = 0; i < 1000; i++){
//        fc_1_bias2D[i] = round(fc_1_bias2D[i] * 100)/100;
        fc_8_bias2D_int[i] = (data_type_w)(fc_8_bias2D[i]);
    }

#if _KERNEL_DEBUG_
	float fc_8_out[1000][1][1] = { 0 };
	data_type   fc_8_out_int[1000][1][1];
	clock_t start, finish;
	double totaltime;
	start = clock();
#endif

#if _BATCH_MODE_
	float fc_1_out_a[10][1000][1][1] = { 0 };
	float fc_1_out_temp[1000][1][1] = { 0 };
	data_type   fc_1_out_temp_int[1000][1][1];
	for(int i = 0; i < 1000; i++){
        fc_1_out_temp_int[i][0][0] = (data_type)(0);
    }
    cout<< endl;

	cout << "starting forward network batch process..........................." << endl;
	cout << "..........................................................." << endl;

	clock_t start, finish;
	double totaltime;
	start = clock();

	//for (int i = 0; i < 10000; i++) {//test mnist dataset
	for (int i = 0; i < 10; i++) {//test imagenet dataset
#endif

								 //Inference network process
		inference_net(
			relu, //activation function

#if _KERNEL_DEBUG_
			in_data_3D_int, //input pic data
#endif

#if _BATCH_MODE_
						//mnist_test_data[i], //input test mnist dataset
			imagenet_test_data_int[i],//input test imagenet dataset
#endif
								  //layer weights and bias inputs
			conv_1_weight2D_int,
			conv_1_bias2D_int,
			conv_2_weight2D_int,
			conv_2_bias2D_int,
			conv_3_weight2D_int,
			conv_3_bias2D_int,
			conv_4_weight2D_int,
			conv_4_bias2D_int,
			conv_5_weight2D_int,
			conv_5_bias2D_int,
			fc_6_weight2D_int,
			fc_6_bias2D_int,
			fc_7_weight2D_int,
			fc_7_bias2D_int,
			fc_8_weight2D_int,
			fc_8_bias2D_int,


#if _BATCH_MODE_
			fc_1_out_temp_int);
		//test mnist dataset
		/*for (int j = 0; j < 10; j++) {
		fc_1_out_a[i][j] = fc_1_out_temp[j];
		fc_1_out_temp[j] = 0;
		}*/
		//test imagenet dataset
		for (int j = 0; j < 1000; j++) {
			for (int k = 0; k < 1; k++) {
				for (int l = 0; l < 1; l++) {
					fc_1_out_a[i][j][k][l] = (float)(fc_1_out_temp_int[j][k][l]);
					fc_1_out_temp_int[j][k][l] = (data_type)(0);
				}
			}
		}
	}

	softmax(fc_1_out_a);

	predict(fc_1_out_a);

	//accuracy(fc_1_out_a, mnist_test_label);//for mnist dataset
	accuracy(fc_1_out_a, val_name_class);//for imagenet dataset

#endif

#if _KERNEL_DEBUG_
	//output fc data
	fc_8_out_int);

    for(int i=0;i<1000;i++){
		fc_8_out[i][0][0]=(float)(fc_8_out_int[i][0][0]);
	}
	softmax(fc_8_out);

	predict(fc_8_out);
#endif

	finish = clock();
	totaltime = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "predicted time is: " << totaltime << " s" << endl;

#if _HLS_MODE_
	cout << "finished inference processing ............" << endl;
	ofstream predict_output;
	predict_output.open("predict_output.txt", ios::app);
	for (int i = 0; i < 10; i++) {
		predict_output << fc_8_out[i][0][0] << " " << endl;
	}
	predict_output.close();
	cout << endl;
#endif

	return 0;

}

