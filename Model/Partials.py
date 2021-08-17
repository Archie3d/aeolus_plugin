"""
Parse the par-text-frame-format exported from SPEAR.
"""

import numpy as np
from math import log10
import peakutils
from scipy.optimize import curve_fit

#===============================================================================

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
        k = n * i // 24
        x = 1.0 - z - 1.5 * y
        y += w * x

        if k == j:
            d = 0.0
        else:
            d = w * y * p / (k - j)

        while j < k:
            m = float(j) / float(n)
            j += 1
            att.append((1.0 - m) * z + m)
            z += d

        i += 1

    return att

def get_attgain(n, level):
    def f(x, p):
        assert len(x) == n
        a = attgain(n, p)
        for i in range(n):
            a[i] *= level

        return a

    return f

def median(input):
    arr = sorted(input)
    return arr[len(arr)//2] if len(arr) % 2 else (arr[len(arr)//2] + arr[len(arr)//2-1])/2

#===============================================================================

class Partial:
    def __init__(self):
        self.freq = []
        self.level = []
        self.lowest_freq = 0.0
        self.highest_freq = 0.0
        self.mid_freq = 0.0

        self.mid_level = 0.0
        self.level_randomization = 0.0
        self.attack = 0
        self.attack_profile = 0.0

    def calc_freq_range(self):
        f_sum = 0.0
        f_num = 0

        for freq in self.freq:
            if freq > 0.0:
                f_sum += freq
                f_num += 1

            if self.lowest_freq == 0.0 or freq < self.lowest_freq:
                self.lowest_freq = freq

            if self.highest_freq == 0.0 or freq > self.highest_freq:
                self.highest_freq = freq

        self.mid_freq = 0.0 if f_num == 0 else f_sum/f_num

    def characterize(self, time):
        levels = np.array(self.level)
        peaks = peakutils.indexes(levels, thres=0.01, min_dist=5)
        troughs = peakutils.indexes(-levels, thres=0.01, min_dist=5)

        self.mid_level = median(self.level)
        lev_max = self.mid_level

        for peak in peaks:
            lev_max = max(lev_max, self.level[peak])

        lev_min = lev_max

        for trough in troughs:
            if self.level[trough] > 0.5*lev_max:
                lev_min = min(lev_min, self.level[trough])

        self.level_randomization = 20.0 * log10(lev_max / lev_min) if lev_min > 0.0 else 0.0

        idx_attack = max(5, peaks[0])
        lev_attack = self.level[idx_attack]

        while self.level[idx_attack] > 0.5*(lev_attack+self.mid_level):
            idx_attack += 1

        self.attack = time[idx_attack]

        xdata = np.linspace(0, 1, idx_attack)
        ydata = self.level[0:idx_attack]
        assert len(xdata) == len(ydata)
        func = get_attgain(len(xdata), self.mid_level)

        popt, pcov = curve_fit(func, xdata, ydata, p0=[2.0], bounds=(-1.4, 10.0))
        self.attack_profile = popt[0]


#===============================================================================

class Partials:
    def __init__(self):
        self._partials_count = 0
        self._frame_count = 0
        self._time = []
        self._partials = []

    def read_from_file(self, filePath):
        with open(filePath, 'r') as file:
            self._read_header(file)
            self._read_frame_data(file)

    @property
    def partials_count(self):
        return self._partials_count

    @property
    def time(self):
        return self._time

    @property
    def partials(self):
        return self._partials

    def _read_header(self, file):
        format = file.readline().rstrip()

        if format != "par-text-frame-format":
            raise Exception(f"Unexpected file format: {format}")

        point_type = file.readline().rstrip()

        if point_type != "point-type index frequency amplitude":
            raise Exception("Unexpected point type")

        partials_count = file.readline().rstrip().split()

        if len(partials_count) != 2 or partials_count[0] != "partials-count":
            raise Exception("Failed to read the number of partials")

        partials_count_num = int(partials_count[1])

        if partials_count_num <= 0:
            raise Exception(f"Invalid number of partials: {partials_count_num}")

        frame_count = file.readline().rstrip().split()

        if len(frame_count) != 2 or frame_count[0] != "frame-count":
            raise Exception("Failed to read the nmber of frames")

        frame_count_num = int(frame_count[1])

        if frame_count_num <= 0:
            raise Exception(f"Invalid number of frames {frame_count_num}")

        frame_data = file.readline().rstrip()

        if frame_data != "frame-data":
            raise Exception("Frame data not found")

        self._partials_count = partials_count_num
        self._frame_count = frame_count_num

    def _read_frame_data(self, file):
        self._time = []
        self._partials = []

        for i in range(self._partials_count):
            self._partials.append(Partial())

        eof = False

        while not eof:
            line = file.readline().rstrip()

            if line == "":
                eof = True
            else:
                self._parse_frame_line(line)

        for partial in self._partials:
            partial.calc_freq_range()
            partial.characterize(self._time)

        self._partials.sort(key=lambda p: p.mid_freq)

    def _parse_frame_line(self, line):
        s = line.split()
        time = float(s[0])
        n_partials = int(s[1])
        assert n_partials <= self._partials_count

        self._time.append(time)

        idx = 2

        freq = {}
        amp = {}

        while idx < len(s):
            assert idx + 2 < len(s)
            partial_number = int(s[idx])
            assert partial_number >= 0 and partial_number < self._partials_count
            partial_freq = float(s[idx + 1])
            partial_amp = float(s[idx + 2])

            freq[partial_number] = partial_freq
            amp[partial_number] = partial_amp

            idx += 3

        for i in range(self._partials_count):
            if i in freq:
                assert i in amp
                self._partials[i].freq.append(freq[i])
                self._partials[i].level.append(amp[i])
            else:
                self._partials[i].freq.append(0.0)
                self._partials[i].level.append(0.0)
