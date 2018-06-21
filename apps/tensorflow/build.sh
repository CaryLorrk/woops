#!/bin/sh

cd `dirname $0`/tensorflow
ln -s /usr/local/cuda/lib64/stubs/libcuda.so /usr/local/cuda/lib64/stubs/libcuda.so.1
bazel build -c opt --copt=-mavx --copt=-mavx2 --copt=-mfma --copt=-mfpmath=both --copt=-msse4.2 --config=cuda --cxxopt="-D_GLIBCXX_USE_CXX11_ABI=0" \
        tensorflow/tools/pip_package:build_pip_package && \
    bazel-bin/tensorflow/tools/pip_package/build_pip_package /tmp/pip && \
    pip uninstall -y tensorflow && \
    pip install /tmp/pip/tensorflow-*.whl && \
    cp /tmp/pip/tensorflow-*.whl .
rm /usr/local/cuda/lib64/stubs/libcuda.so.1 
