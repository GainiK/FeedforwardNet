//
// Created by yaochen on 29/12/16.
//TODO-1: modify conv function to compute input arrays' dimensions that are not equal
//TODO-2: add conv function with pooling (conv fusion pool)

#ifndef _CONV_LAYER_2_H_
#define _CONV_LAYER_2_H_

#include <iostream>
#include <fstream>
#include "activation_functions.h"
//#include "config.h"

using namespace std;

extern const bool tbl[6][16];

template <typename T, typename W, typename G, int _INPUT_SIZE_, int _CONV_KERNEL_SIZE_, int _CONV_PADDING_, int _CONV_STRIDE_, int _IN_CHANNEL_NUM_, int _OUT_CHANNEL_NUM_, int _GROUP_>
class conv_layer_2 {

private:
    int conv_layer_number;
    int out_data_size;
public:
    conv_layer_2() : conv_layer_number(0) { out_data_size = (_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1;
    };

    /************************************************************************************************/
    void conv_kernel_a(
            T *in_data,
            W *kernel_weights,
            G *out_data) {

        T in_data_temp[_INPUT_SIZE_][_INPUT_SIZE_];
        W kernel_weights_temp[_IN_CHANNEL_NUM_][_CONV_KERNEL_SIZE_][_CONV_KERNEL_SIZE_];
//        G out_data_par[(_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1][(_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1];

			imgH: for (int i = _CONV_KERNEL_SIZE_ / 2 - _CONV_PADDING_; i < _INPUT_SIZE_ + _CONV_PADDING_ - _CONV_KERNEL_SIZE_ / 2; i += _CONV_STRIDE_) {
                imgV: for (int j = _CONV_KERNEL_SIZE_ / 2 - _CONV_PADDING_; j < _INPUT_SIZE_ + _CONV_PADDING_ - _CONV_KERNEL_SIZE_ / 2; j += _CONV_STRIDE_) {
                    WindowH: for (int ii = (-_CONV_KERNEL_SIZE_ / 2); ii <= _CONV_KERNEL_SIZE_ / 2; ii = ii + 1) {
                        WindowV: for (int jj = -_CONV_KERNEL_SIZE_ / 2; jj <= _CONV_KERNEL_SIZE_ / 2; jj = jj + 1) {
                            if ((i + ii >= 0) && (i + ii < _INPUT_SIZE_) && (j + jj >= 0) && (j + jj < _INPUT_SIZE_)) {//if overlapped
                                T data = *(in_data + _INPUT_SIZE_*(i + ii)+(j + jj));
                                W weight = *(kernel_weights + (ii + _CONV_KERNEL_SIZE_ / 2)*_CONV_KERNEL_SIZE_+(jj + _CONV_KERNEL_SIZE_ / 2));
								*(out_data+ ((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)*((i - _CONV_KERNEL_SIZE_ / 2 + _CONV_PADDING_) / _CONV_STRIDE_)+(
                                        (j - _CONV_KERNEL_SIZE_ / 2 + _CONV_PADDING_) / _CONV_STRIDE_)) += data * weight;
                            }
                        }
                    }
                }
            }

#if _C_DEBUG_MODE_
#if _KERNEL_DEBUG_
        int conv_layer_count = 0;
        ofstream conv_kernel_a;
        conv_kernel_a.open("conv_kernel_a.txt", ios::app);
        for (int j = 0; j < _INPUT_SIZE_ ; j++) {
            for (int k = 0; k < _INPUT_SIZE_ ; k++) {
                conv_kernel_a << in_data[j][k] << " "; // i?
            }
            conv_kernel_a << endl;
        }
        conv_kernel_a << endl;
        for (int j = 0; j < _CONV_KERNEL_SIZE_; j++) {
            for (int k = 0; k < _CONV_KERNEL_SIZE_; k++) {
                conv_kernel_a << kernel_weights[j][k] << " "; // i?
            }
            conv_kernel_a << endl;
        }
        conv_kernel_a << endl;
        for (int j = 0; j < _INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_ + 1; j++) {
            for (int k = 0; k < _INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_ + 1; k++) {
                conv_kernel_a << out_data[j][k] << " "; //
            }
            conv_kernel_a << endl;
        }
        conv_kernel_a << endl;
        conv_kernel_a.close();
#endif
#endif
    }

    /************************************************************************************************/
    //3D array to 3D array convolution layer without connection table
    void conv_layer_a(
            char activation_type,
            T *in_data3D,
            W *kernel_weights,
            W *kernel_bias,
            G *out_data3D) {

//#pragma HLS ARRAY_PARTITION variable=in_data3D cyclic factor=3 dim=1 partition

#if _C_DEBUG_MODE_
#if _KERNEL_DEBUG_
        cout << "Starting convolution layer ...." << endl;
#endif
#endif

		for (int c = 0; c < _GROUP_; c++) {//group loop
			for (int b = c * _OUT_CHANNEL_NUM_ / _GROUP_; b < c * _OUT_CHANNEL_NUM_ / _GROUP_ + _OUT_CHANNEL_NUM_ / _GROUP_; b++) {//output kernel loop
				for (int a = c * _IN_CHANNEL_NUM_ / _GROUP_; a < c * _IN_CHANNEL_NUM_ / _GROUP_ + _IN_CHANNEL_NUM_ / _GROUP_; a++) {//input kernel loop
//#pragma HLS inline off
//#pragma HLS UNROLL factor=16
					conv_kernel_a(in_data3D+a*_INPUT_SIZE_*_INPUT_SIZE_,
						kernel_weights+(b * _IN_CHANNEL_NUM_ / _GROUP_ + a % (_IN_CHANNEL_NUM_ / _GROUP_))*_CONV_KERNEL_SIZE_*_CONV_KERNEL_SIZE_,
						out_data3D + b*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
						*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1));
				}
				for (int j = 0; j < (_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1; j++) {
					for (int k = 0; k < (_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1; k++) {
#if _ACT_RELU_
                        *(out_data3D + b*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
                            *((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
                            +j*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)+k) 
                        = Relu_64((*(out_data3D + b*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
                                *((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
                                + j*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1) + k) + *(kernel_bias+b)));
#else
                        *(out_data3D + b*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
                                       *((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
                          +j*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)+k) = 0;
//						*(out_data3D + b*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
//							*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
//							+j*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)+k)
//							= f(activation_type, (*(out_data3D + b*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
//								*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
//								+ j*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1) + k) + *(kernel_bias+b)));
#endif
					}
				}
			}
		}

#if _C_DEBUG_MODE_
#if _KERNEL_DEBUG_
        cout << "Finished convolution layer ...." << endl;
        ofstream out_conv_a;
        out_conv_a.open("conv_layer_a.txt", ios::app);

        out_conv_a << "input 3D arry to conv layer ..................." << endl;
        for (int i = 0; i < _IN_CHANNEL_NUM_; i++) {
            for (int j = 0; j < _INPUT_SIZE_; j++) {
                for (int k = 0; k < _INPUT_SIZE_; k++) {
                    out_conv_a << *(in_data3D + i*_INPUT_SIZE_ *_INPUT_SIZE_+ j*_INPUT_SIZE_ + k) << " ";
                }
                out_conv_a << endl;
            }
            out_conv_a << endl;
        }
        out_conv_a << endl;


        out_conv_a << "output from conv layer .........................." << endl;
        for (int i = 0; i < _OUT_CHANNEL_NUM_; i++) {
            for (int j = 0; j < (_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1; j++) {
                for (int k = 0; k < (_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1; k++) {
                    out_conv_a << *(out_data3D + i*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
						*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1)
						+ j*((_INPUT_SIZE_ + _CONV_PADDING_ * 2 - _CONV_KERNEL_SIZE_) / _CONV_STRIDE_ + 1) + k) << " ";
                }
                out_conv_a << endl;
            }
            out_conv_a << endl;
        }
        out_conv_a.close();
        cout << endl;
#endif
#endif

    }

};

#endif
