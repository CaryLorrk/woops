#!/bin/sh

train_dir=/root/volume/benchmarks/log/train

mkdir -p ${train_dir}
rm -f ${train_dir}/*
cd tf_cnn_benchmarks
python tf_cnn_benchmarks.py \
    --batch_size=64 \
    --model=resnet110 \
    --num_batch=10000 \
    --optimizer=sgd \
    --save_summaries_steps=100 \
    --save_model_secs=100 \
    --summary_verbosity=1 \
    --data_dir=cifar10 \
    --data_name=cifar10 \
    --train_dir=${train_dir}
