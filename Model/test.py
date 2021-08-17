import matplotlib.pyplot as plt
import numpy as np
import peakutils

from Partials import *

def attgain(n, p):
    att = []

    w = 0.05
    y = 0.6

    if p > 0.0:
        y += 0.11 * p

    z = 0.0

    i = 1
    j = 0

    while i <= 24:
        k = int(n * i / 24)
        x = 1.0 - z - 1.5 * y
        y += w * x
        d = 0.0 if k == j else w * y * p / (k - j)

        while j < k:
            m = float(j) / float(n)
            j += 1
            att.append((1.0 - m) * z + m)
            z += d

        i += 1

    return att


partials = Partials()

partials.read_from_file("036-C.txt")

N = 2

peaks = peakutils.indexes(np.array(partials.partials[N].level), thres=0.01, min_dist=5)

peaks_x = []
peaks_y = []
for peak in peaks:
    peaks_x.append(partials.time[peak])
    peaks_y.append(partials.partials[N].level[peak])

plt.plot(partials.time, partials.partials[N].level)

plt.scatter(peaks_x, peaks_y)

f0 = partials.partials[0].mid_freq

attack = partials.partials[N].attack
attp = partials.partials[N].attack_profile
rnd = partials.partials[N].level_randomization
freq = partials.partials[N].mid_freq
freq_ratio = freq / f0
print(f"ATTACK={attack}, PROFILE={attp}, RAND={rnd}, F={freq} Fratio={freq_ratio}")
n_att = int(attack / 0.01)
att = np.array(attgain(n_att, attp)) * partials.partials[N].mid_level
att_x = np.arange(n_att) * 0.01

plt.plot(att_x, att)

plt.grid()
plt.show()
