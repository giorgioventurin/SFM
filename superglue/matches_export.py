import numpy as np
import os
from os import path

dataset_path = 'datasets/superglue'
match_pairs_paths = ['dump_match_pairs_1', 'dump_match_pairs_2']

for i in range(len(match_pairs_paths)):
    filenames = [f for f in os.listdir(path.join(dataset_path, match_pairs_paths[i])) if path.isfile(path.join(dataset_path, match_pairs_paths[i], f))]
    npz = np.load(path.join(dataset_path, match_pairs_paths[i], filenames[0]))

    print(npz.files)
    print(npz['keypoints0'].shape)
    print(npz['keypoints0'].shape)
    print(npz['matches'].shape)
    print()
    print(npz['matches'][:])

    # TODO: export keypoints from npz data
