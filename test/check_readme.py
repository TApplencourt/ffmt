#!/usr/bin/env python3
"""Every C++ code block of README.md that is a whole program (has int main)
must be an exact copy of a file in examples/, which the ids build and run
(FFMT_EXTRA_FILES). Prints each block that is not, then exits 1 if any."""
import glob
import os
import re
import sys

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
examples = {open(f).read() for f in glob.glob("examples/*")}
readme = open("README.md").read()
bad = [readme.count("\n", 0, m.start()) + 1
       for m in re.finditer(r"```cpp\n(.*?)```", readme, re.S)
       if "int main(" in m.group(1) and m.group(1) not in examples]
for line in bad:
    print("README.md:%d: a program that is not a copy of a file in examples/" % line)
sys.exit(1 if bad else 0)
