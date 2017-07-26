#!/bin/sh

DATASETS_DIR=/root/volume/datasets
DATASET=cifar10
MODEL=inception_v3
TRAIN_DIR=/tmp/log/cifar10_cifarnet-base/train
OPTIMIZER=sgd
python models/slim/train_image_classifier.py \
    --train_dir=$TRAIN_DIR \
    --dataset_name=$DATASET \
    --dataset_split_name=train \
    --dataset_dir=$DATASETS_DIR/$DATASET \
    --optimizer=$OPTIMIZER \
    --model_name=$MODEL \
    --save_summaries_secs=60 \
    --save_interval_secs=60 \
    --max_number_of_steps=100000
