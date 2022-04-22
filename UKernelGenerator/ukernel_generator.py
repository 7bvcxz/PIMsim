import re

def op_code(word):
    if "NOP" in word:
        FLOW_FLAG = 1
        return 0b0000 << 28
    elif "JUMP" in word:
        FLOW_FLAG = 1
        return 0b0001 << 28
    elif "EXIT" in word:
        EXIT_FLAG = 1
        return 0b0010 << 28
    elif "MOV" in word:
        return 0b0100 << 28
    elif "FILL" in word:
        return 0b0101 << 28
    elif "ADD" in word:
        ARIT_FLAG = 1
        return 0b1000 << 28
    elif "MUL" in word:
        ARIT_FLAG = 1
        return 0b1001 << 28
    elif "MAC" in word:
        ARIT_FLAG = 1
        return 0b1010 << 28
    elif "MAD" in word:
        ARIT_FLAG = 1
        return 0b1011 << 28
    else:
        raise

def aam_code(word):
    if "AAM" in word:
        return 1 << 15
    else:
        return 0

def src_code(word, loc):
    shifter = 25 - loc * 3
    idx_shifter = 8 - loc * 4
    out = 0b0
    
    if "BANK" in word:
        out = 0b000 << shifter
    elif "GRF_A" in word:
        out = 0b001 << shifter
    elif "GRF_B" in word:
        out = 0b010 << shifter
    elif "SRF_A" in word:
        out = 0b011 << shifter
    elif "SRF_M" in word:
        out = 0b100 << shifter
    else:
        idx_shifter = 11 - loc * 11
    
    if any(value.isdigit() for value in word):
        tmp = int(re.sub(r'[^0-9]', '', word))
        if "-" in word:
            tmp = tmp | (0b1 << 7)
        tmp = tmp << idx_shifter
        out = out | tmp
    return out


def convert_to_ukernel(words):
    ukernel_code = 1 << 32 # later gone
    ukernel_code = ukernel_code | op_code(words[0])
    ukernel_code = ukernel_code | aam_code(words[0])
    if len(words) >= 2:
        ukernel_code = ukernel_code | src_code(words[1], 0)
    if len(words) >= 3:
        ukernel_code = ukernel_code | src_code(words[2], 1)
    if len(words) >= 4:
        ukernel_code = ukernel_code | src_code(words[3], 2)
    ukernel_code = ukernel_code - (1 << 32) # later gone
    return ukernel_code

input_file = open("input.txt", "r")
output_file = open("output.txt", "w")
idx = 0
for line in input_file:
    words = line.split()
    ukernel_code = convert_to_ukernel(words)
    output_file.write("ukernel[")
    output_file.write(str(idx))
    output_file.write("]=")
    output_file.write(str(bin(ukernel_code)))
    output_file.write(";  // ")
    output_file.write(line)
    idx = idx + 1
     

