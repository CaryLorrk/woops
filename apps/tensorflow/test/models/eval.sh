#!/bin/sh

DATASETS_DIR=/root/volume/datasets
DATASET=cifar10
MODEL=inception_v3
LOG_DIR=/tmp/log/cifar10-inception_v3-2
EVAL_DIR=${LOG_DIR}/eval
TRAIN_DIR=${LOG_DIR}/train
rm -rf ${EVAL_DIR}
python models/slim/eval_image_classifier.py \
    --checkpoint_dir=${TRAIN_DIR} \
    --eval_dir=${EVAL_DIR} \
    --dataset_name=${DATASET} \
    --dataset_split_name=test \
    --dataset_dir=${DATASETS_DIR}/${DATASET} \
    --model_name=${MODEL}
