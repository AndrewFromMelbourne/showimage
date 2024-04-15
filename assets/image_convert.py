#!/usr/bin/env python3
""" print_image """

import argparse

from PIL import Image
from numpy import asarray

from pathlib import Path

# ================================================================================

def main():
    """main"""

    # ----------------------------------------------------------------------------

    parser = argparse.ArgumentParser()
    parser.add_argument('name', help='image file name')
    args = parser.parse_args()

    # ----------------------------------------------------------------------------

    image = Image.open(args.name)

    image_grey = image.convert('L')
    numpydata = asarray(image_grey)

    name_stem = Path(args.name).stem
    name = f"image{name_stem}"
    length = image.width * image.height
    print(f"const uint8_t {name}[{length}] = ")
    print("{")

    codeWidth = 24

    index = 0
    for column in numpydata:
        for pixel in column:
            print('0x{:02x}, '.format(pixel), end='')

            index += 1

            if ((index % codeWidth) == 0):
                print("")
                print("    ", end='')

    print("};")

# ================================================================================

if __name__ == '__main__':
    main()
