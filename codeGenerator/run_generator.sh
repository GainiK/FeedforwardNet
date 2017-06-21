#!/bin/bash

mkdir -p ../example/test_demo
mkdir -p ../example/test_demo/hls_impl
mkdir -p ../example/test_demo/inference_net
mkdir -p ../example/test_demo/inference_net/stb_image
mkdir -p ../example/test_demo/net_inputs

copy_file(){
	if [ -e $1 ]
	then
	    if [[ $3 -eq 1 ]]
	    then
	    	cp -f  $1 $2
	    else
		cp -r  $1 $2
	    fi
	else
	    echo "No $1 file"
	fi
}

read -p "Please enter the path to folder with test images: "  test_img_folder

prm_file_name="net_config_params.txt"

copy_file "../fpga_cnn/caffe_converter/$prm_file_name" "." 1

if grep -q "conv" "$prm_file_name"; 
then
copy_file "../fpga_cnn/conv_layer_one_dim.h" "../example/test_demo/inference_net/" 1
fi

if grep -q "pool" "$prm_file_name"; 
then
copy_file "../fpga_cnn/pool_layer_one_dim.h" "../example/test_demo/inference_net/" 1
fi

if grep -q "lrn" "$prm_file_name"; 
then
copy_file "../fpga_cnn/lrn_layer_one_dim.h" "../example/test_demo/inference_net/" 1
fi

if grep -q "fc" "$prm_file_name"; 
then
copy_file "../fpga_cnn/fc_layer_one_dim.h" "../example/test_demo/inference_net/" 1
fi


copy_file "../scripts/hls_script.tcl" "../example/test_demo/hls_impl/" 1
copy_file "../scripts/syn.sh" "../example/test_demo/hls_impl/" 1
copy_file "../stb_image" "../example/test_demo/inference_net/" 2
copy_file "../fpga_cnn/data_type.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/activation_functions.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/pow_function.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/softmax_one_dim.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/weight_bias_one_dim.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/image_converter.h" "../example/test_demo/inference_net/" 1

copy_file "$test_img_folder" "../example/test_demo/net_inputs/" 2

copy_file "../fpga_cnn/caffe_converter/net_mean.txt" "../example/test_demo/net_inputs/" 1
copy_file "../fpga_cnn/caffe_converter/net_weights.txt" "../example/test_demo/net_inputs/" 1
copy_file "../example/caffe_demos/alexnet/val.txt" "../example/test_demo/net_inputs/" 1
copy_file "../scripts/Makefile" "../example/test_demo/" 1
copy_file "../fpga_cnn/predict_one_dim.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/accuracy_one_dim.h" "../example/test_demo/inference_net/" 1
copy_file "../fpga_cnn/resize_image.h" "../example/test_demo/inference_net/" 1

python generator.py $prm_file_name 
python generator_config.py $prm_file_name
python generator_ff_test.py $prm_file_name