from __future__ import print_function
import uproot
import tensorflow as tf
from tensorflow.python.framework import dtypes
from tensorflow.python import debug as tf_debug

import numpy as np


def pad_to(x, n):
    ret = np.zeros((x.shape[0], n), dtype=np.float32)
    for i in range(x.shape[0]):
        nmax = min(len(x[i]), n)
        ret[i, :nmax] = x[i][:nmax]
    return ret

nbins = 40
max_jets = 30

batch_size = 1000
max_events = 100000

ev = uproot.open("data/0E0B09D0-6A59-E811-9F00-008CFAFBF576.root")["Events"]
arr = ev.arrays(["nJet", "Jet_pt", "Jet_eta", "genWeight"])
for i in range(10):
    print(i, arr["nJet"][i], arr["Jet_pt"][i])

jet_pt = arr["Jet_pt"]
jet_eta = arr["Jet_eta"]

t_jet_pt = tf.placeholder(tf.float32, shape=[None, max_jets], name="jet_pts")
t_jet_eta = tf.placeholder(tf.float32, shape=[None, 30], name="jet_etas")
t_event_weights = tf.placeholder(tf.float32, shape=[None, 1], name="event_weights")
t_abs_jet_eta = tf.abs(t_jet_eta)

t_jets_masked = tf.math.less_equal(t_abs_jet_eta, 1.5)
t_selected_jets = tf.where(t_jets_masked, t_jet_pt, tf.zeros_like(t_jet_pt))

t_sum_jet_pt = tf.reduce_sum(t_selected_jets, name="sum_jet_pts", axis=1)
t_histogram_inds = tf.histogram_fixed_width_bins(t_sum_jet_pt, [0.0, 1000.0], nbins)
t_histogram_contents_up = tf.unsorted_segment_sum(t_event_weights, t_histogram_inds, nbins)

with tf.Session() as sess:
    hist = np.zeros((1,nbins))
    for ibatch in range(0, max_events , batch_size):
        batch_out = sess.run(t_histogram_contents_up, {
            t_jet_pt: pad_to(jet_pt[ibatch:ibatch+batch_size], max_jets),
            t_jet_eta: pad_to(jet_eta[ibatch:ibatch+batch_size], max_jets),
            t_event_weights: arr["genWeight"][ibatch:ibatch+batch_size].reshape(batch_size, 1)
        })
        print("batch=", ibatch, "shape=", batch_out.shape, batch_out.T)
        hist += batch_out.T
    print(hist)