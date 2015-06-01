"""
Fix autogenerated py files to contain header information.
Writes to the file in-place
"""

import sys
import os

def lines_to_pre_append():
    lines = list()
    # PYLINT ingnore flags
    lines.append("#pylint: skip-file\n")
    return lines


def main(argv):
    """
        Main entry point
        
        Args:
        argv (list): List of strings giving command line arguments The full absolulte path to the file to wrap is mandatory.
        
    """
  
    argv.reverse()
    to_wrap = argv[0]
    if not os.path.exists(to_wrap):
        raise ValueError("%s : Does not exist." % to_wrap)
    
    with open(to_wrap, "r+") as f:
        existing = f.read();
        f.seek(0);
        # Add initial lines
        for line in lines_to_pre_append():
            f.write(line)
        f.write(existing)
    
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
