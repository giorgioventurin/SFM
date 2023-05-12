import os
from os import path
from PIL import Image

dataset_path = 'datasets'
pairs_paths = ['images_1', 'images_2', 'images_3']
output_path = 'datasets/'

for i in range(len(pairs_paths)):
    filenames = [f for f in os.listdir(path.join(dataset_path, pairs_paths[i])) if path.isfile(path.join(dataset_path, pairs_paths[i], f))]

    out = open(path.join(dataset_path, pairs_paths[i] + '_pairs.txt'), 'w')

    for j in range(len(filenames) - 1):
        for k in range(j + 1, len(filenames)):
            out.write(filenames[j] + ' ' + filenames[k] + ' 0 0\n')
    out.close()
