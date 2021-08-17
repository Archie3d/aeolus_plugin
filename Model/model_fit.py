import json
from math import log10
from Partials import *

partials_files = [
    "036-C.txt",
    "042-F#.txt",
    "048-C.txt",
    "054-F#.txt",
    "060-C.txt",
    "066-F#.txt",
    "072-C.txt",
    "078-F#.txt",
    "084-C.txt",
    "090-F#.txt",
    "096-C.txt"
]

NUM_HARMONICS = 64
NUM_NOTES = 11

h_lev = []
h_ran = []
h_att = []
h_atp = []

partials_per_note = []

for file in partials_files:
    partials = Partials()
    partials.read_from_file(file)
    partials_per_note.append(partials)

for h in range(NUM_HARMONICS):
    lev = [];
    ran = [];
    att = [];
    atp = [];

    mask = 0
    bit = 1

    for pidx in range(NUM_NOTES):
        x_lev = -100.0
        x_ran = 0.0
        x_att = 0.05
        x_atp = 0.0

        if pidx < len(partials_per_note):
            p = partials_per_note[pidx]

            if h < len(p.partials):
                if p.partials[h].mid_level > 0.0:
                    mask = mask | bit
                    x_lev = 20.0 * log10(p.partials[h].mid_level)

                x_ran = p.partials[h].level_randomization
                x_att = p.partials[h].attack

                if x_att > 0.5:
                    x_att = 0.5

                x_atp = p.partials[h].attack_profile if x_att > 0.05 else 0.0

                if x_atp > 2.4:
                    x_atp = 2.4
                if x_atp < -1.2:
                    x_atp = -1.2

        lev.append(x_lev)
        ran.append(x_ran)
        att.append(x_att)
        atp.append(x_atp)

        bit <<= 1

    h_lev.append({
        "mask": mask,
        "values": lev
    })

    h_ran.append({
        "mask": mask,
        "values": ran
    })

    h_att.append({
        "mask": mask,
        "values": att
    })

    h_atp.append({
        "mask": mask,
        "values": atp
    })

output = {
    "h_lev": h_lev,
    "h_ran": h_ran,
    "h_att": h_att,
    "h_atp": h_atp
}

result = json.dumps(output, indent=2)

with open("result.json", "w") as f:
    f.write(result)

