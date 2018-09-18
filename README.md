# woops-grpc

## requisite
nvidia-docker

## build tensorflow
```shell
git clone --recursive git@taiji.csie.ntu.edu.tw:hilorrk/woops.git
cd woops
make -f Makefile.docker
make -f Makefile.docker run(first time)/ start(container was created)
make -f Makefile.docker attach
# in docker
cd woops/apps/tensorflow
./configure.sh(first time)
./build.sh
```

## test
```shell
cd test
python linear.py
python mnist_softmax/mnist_softmax.py
cd benchmarks
./train.sh
./eval.sh
```

## Usage
Please read the architecture in [paper](paper.pdf)
Consistency Controller: src/consistency
Placement: src/placement
Storages: in apps

