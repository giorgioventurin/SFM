import numpy as np
import os
from os import path

dataset_path = '../datasets/superglue'
match_pairs_paths = ['dump_match_pairs_1', 'dump_match_pairs_2', 'dump_match_pairs_3']
# match_pairs_paths = ['dump_match_pairs_3']

for i in range(len(match_pairs_paths)):
    filenames = [f for f in os.listdir(path.join(dataset_path, match_pairs_paths[i])) if path.isfile(path.join(dataset_path, match_pairs_paths[i], f))]
    filenames = sorted(filenames)

    for filename in filenames:
        npz = np.load(path.join(dataset_path, match_pairs_paths[i], filename))
        src = filename[0:2]
        dst = filename[3:5]

        # npz keys: keypoints0, keypoints1, matches, match_confidence
        points_i = str()
        points_j = str()
        matches = str()
        delimiter = " "
        scale = 100.0
        img_index = 0

        for k in range(npz['keypoints0'].shape[0]):
            points_i = points_i + "0" + delimiter + str(npz['keypoints0'][k][0]) + delimiter + str(npz['keypoints0'][k][1]) + "\n"
        for k in range(npz['keypoints1'].shape[0]):
            points_j = points_j + "1" + delimiter + str(npz['keypoints1'][k][0]) + delimiter + str(npz['keypoints1'][k][1]) + "\n"

        for k in range(npz['matches'].shape[0]):
            query_idx = k  # index of the keypoint/descriptor in first image that has a match
            if npz['matches'][k] != -1:
                train_idx = npz['matches'][k]  # index of the matched keypoint/descriptor in second image
                distance = (1.0 / npz['match_confidence'][k]) * scale  # use confidence as distance measure + scaling
                # information to build a DMatch object in OpenCV: imgIdx trainIdx queryIdx distance
                # imgIdx seems to be always 0
                # trainIdx is the index of the keypoint/descriptor of second image for the given match
                # queryIdx is the index of the keypoint/descriptor of first image for given match
                # distance: measured by means of confidence
                matches = matches + str(img_index) + delimiter + str(train_idx) + delimiter + str(query_idx) + delimiter + str(distance) + "\n"

        # MUST HAVE DIRECTORIES match_info_1, match_info_2 UNDER /datasets/superglue/ IN ORDER TO WORK!
        save_path = "../datasets/superglue/match_info_" + str(i+1) + "/"
        name = src + "_" + dst + ".txt"
        if int(dst) < int(src):
            name = dst + "_" + src + ".txt"
        name = os.path.join(save_path, name)
        file = open(name, "w")
        # file format:
        file.write(matches)  # list of matches --> matches in features_matcher.cpp:120,124
        file.write("\n")  # blank line
        file.write(points_i)  # list of keypoints of first image that have a match --> p_i in features_matcher.cpp:104
        file.write(points_j)  # list of keypoints of second image that have a match --> p_j in features_matcher.cpp:104
        file.close()

