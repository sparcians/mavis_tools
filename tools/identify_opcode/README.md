# RISC-V Opcode Identification Utility

Identifies all possible instructions (and the extensions that contain them) that map to a given opcode

## Usage
```
Usage: identify_opcode [OPTION] <opcode>
Identifies all RISC-V instructions that map to the given opcode

Optional arguments:
  -h [ --help ]                         print this help message
  -m [ --mavis ] path (=mavis)          path to mavis
  -x [ --xlen ] xn                      restrict search to given XLEN(s)

Required arguments:
  <opcode>                              opcode to find
```
