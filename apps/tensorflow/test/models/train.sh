#!/bin/sh

DATASETS_DIR=/root/volume/datasets
DATASET=cifar10
MODEL=inception_v3
TRAIN_DIR=/root/volume/log/cifar10-inception_v3-2/train
OPTIMIZER=sgd
WOOPS_CONFIG=../config.in
python models/slim/train_image_classifier.py \
    --woops_config_file=${WOOPS_CONFIG} \
    --train_dir=$TRAIN_DIR \
    --dataset_name=$DATASET \
    --dataset_split_name=train \
    --dataset_dir=$DATASETS_DIR/$DATASET \
    --optimizer=$OPTIMIZER \
    --model_name=$MODEL \
    --save_summaries_secs=600 \
    --save_interval_secs=600 \
    --max_number_of_steps=10000
