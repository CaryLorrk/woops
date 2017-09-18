from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import sys

from dataset import read_data_sets
import tensorflow as tf
from tensorflow.python.client import timeline

FLAGS = None


def main(_):
    mnist = read_data_sets(FLAGS.data_dir, one_hot=True)

    # Create the model
    x = tf.placeholder(tf.float32, [None, 784])
    W = tf.Variable(tf.zeros([784, 10]), name='Weight')
    b = tf.Variable(tf.zeros([10]), name='Bias')
    y = tf.matmul(x, W) + b

    # Define loss and optimizer
    y_ = tf.placeholder(tf.float32, [None, 10])

    cross_entropy = tf.reduce_mean(
        tf.nn.softmax_cross_entropy_with_logits(labels=y_, logits=y))
    train_step = tf.train.GradientDescentOptimizer(0.5).minimize(cross_entropy)

    correct_prediction = tf.equal(tf.argmax(y, 1), tf.argmax(y_, 1))
    accuracy = tf.reduce_mean(tf.cast(correct_prediction, tf.float32))

    tf.woops_initialize_from_file('config.in')
    for var in tf.trainable_variables():
        tf.woops_register_trainable(str(var.name[:-2]))
    sess = tf.InteractiveSession()
    tf.global_variables_initializer().run()
    tf.woops_force_sync()
    # Train
    for i in range(10000):
        batch_xs, batch_ys = mnist.train.next_batch(100)
        if i % 1000 == 0:
            run_options = tf.RunOptions(trace_level=tf.RunOptions.FULL_TRACE)
            run_metadata = tf.RunMetadata()
            sess.run(train_step,
                     feed_dict={x: batch_xs, y_: batch_ys},
                     options=run_options,
                     run_metadata=run_metadata)
            tl = timeline.Timeline(run_metadata.step_stats)
            ctf = tl.generate_chrome_trace_format()
            with open('timeline-%d.json' % (i), 'w') as f:
                f.write(ctf)
            print(sess.run(accuracy, feed_dict={x: mnist.test.images,
                                        y_: mnist.test.labels}))
        else:
            sess.run(train_step,
                     feed_dict={x: batch_xs, y_: batch_ys})

        tf.woops_clock()

    # Test trained model
    print(sess.run(accuracy, feed_dict={x: mnist.test.images,
                                        y_: mnist.test.labels}))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--data_dir',
        type=str,
        default='datasets/mnist/input_data',
     help='Directory for storing input data')
    FLAGS, unparsed = parser.parse_known_args()
    tf.app.run(main=main, argv=[sys.argv[0]] + unparsed)
