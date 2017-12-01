#!/bin/sh

scheme=/root/volume/benchmarks/log
train_dir=${scheme}/train
eval_dir=${scheme}/eval

mkdir -p ${eval_dir}
rm -f ${eval_dir}/*
cd tf_cnn_benchmarks
python tf_cnn_benchmarks.py  \
    --eval=true \
    --model=resnet110 \
    --data_dir=cifar10 \
    --data_name=cifar10 \
    --train_dir=${train_dir} \
    --eval_dir=${eval_dir}

