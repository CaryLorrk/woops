#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf


# Model parameters
with tf.variable_scope("test"):
    W = tf.Variable([.3], tf.float32, name='weight')
    b = tf.Variable([-.3], tf.float32, name='bias')

# Model input and output
x = tf.placeholder(tf.float32)
linear_model = W * x + b
y = tf.placeholder(tf.float32)
# loss
loss = tf.reduce_sum(tf.square(linear_model - y))  # sum of the squares
# optimizer
optimizer = tf.train.GradientDescentOptimizer(0.01)
train = optimizer.minimize(loss)

# training data
x_train = [1, 2, 3, 4]
y_train = [0, -1, -2, -3]

tf.woops_initialize_from_file("config.in")
for var in tf.trainable_variables():
    tf.woops_register_trainable(str(var.name[:-2]))
# training loop
init = tf.global_variables_initializer()
sess = tf.Session()
sess.run(init)
tf.woops_force_sync()

for i in range(10):
    print("==================start: "+str(i) + "==========================")
    sess.run(train, {x: x_train, y: y_train})
    tf.woops_clock()
    curr_W, curr_b, curr_loss = sess.run([W, b, loss], {x: x_train, y: y_train})
    print("W: %s b: %s loss: %s" % (curr_W, curr_b, curr_loss))

# evaluate training accuracy
curr_W, curr_b, curr_loss = sess.run([W, b, loss], {x: x_train, y: y_train})
print("W: %s b: %s loss: %s" % (curr_W, curr_b, curr_loss))
