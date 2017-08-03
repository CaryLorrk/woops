#!/bin/sh

cd `dirname $0`/tensorflow
bazel build -c opt  --config=cuda --copt=-march=native --cxxopt="-D_GLIBCXX_USE_CXX11_ABI=0" \
        tensorflow/tools/pip_package:build_pip_package && \
    bazel-bin/tensorflow/tools/pip_package/build_pip_package /tmp/pip && \
    pip --no-cache-dir install /tmp/pip/tensorflow-*.whl && \
    cp /tmp/pip/tensorflow-*.whl .
