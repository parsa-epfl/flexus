import subprocess
import random

def labelled_block(block, k):
    nblock = []
    substring = "{}"

    for j in block:
        x = j.find(substring)
        if(x != -1):
            nblock.append(j.format(k+1))
        else:
            nblock.append(j)
    return nblock


def generate_assembly_code():

    # Array to hold the assembly program
    instructions = []
    # Array to hold the labels to be printed to each code block
    labels = []
    # Define the fundamental code blocks
    code_blocks = [
        # Block 1
        [
            "ADD X0, X1, X2",
            "SUB X3, X4, X5",
            "MUL X6, X7, X8",
            "SDIV X9, X10, X11",
            "NOP",
            "NOP",
            "CBZ X16, label{}",
            "CBNZ X17, label{}",
            "B label{}",
            "BL label{}",
            "MOV X18, #42",
            "CMP X19, X20",
            "BEQ label{}",
            "BNE label{}",
            "NOP",
            "NOP"
        ],
        # Block 2
        [
            "ADD X0, X1, X2",
            "SUB X3, X4, X5",
            "MUL X6, X7, X8",
            "SDIV X9, X10, X11",
            "NOP",
            "NOP",
            "CBZ X16, label{}",
            "CBNZ X17, label{}",
            "B label{}",
            "BL label{}",
            "MOV X18, #42",
            "CMP X19, X20",
            "BEQ label{}",
            "BNE label{}",
            "NOP",
            "NOP"
        ],
        # Block 3
        [
            "SUB X0, X1, X2",
            "ADD X3, X4, X5",
            "MUL X6, X7, X8",
            "SDIV X9, X10, X11",
            "NOP",
            "NOP",
            "CBZ X16, label{}",
            "CBNZ X17, label{}",
            "B label{}",
            "BL label{}",
            "MOV X18, #42",
            "CMP X19, X20",
            "BEQ label{}",
            "BNE label{}",
            "NOP",
            "NOP"
        ],

        # Block 4
        [
            "MOV X0, #10",
            "CMP X1, X2",
            "B.EQ label{}",
            "B.NE label{}",
            "ADD X3, X4, X5",
            "SUB X6, X7, X8",
            "MUL X9, X10, X11",
            "SDIV X12, X13, X14",
            "NOP",
            "NOP",
            "CBZ X19, label{}",
            "CBNZ X20, label{}",
            "B label{}",
            "BL label{}",
            "MOV X21, #24",
            "CMP X22, X23"
        ],

        # Block 5
        [
            "ADD X0, X1, X2",
            "SUB X3, X4, X5",
            "MUL X6, X7, X8",
            "SDIV X9, X10, X11",
            "NOP",
            "NOP",
            "CBZ X16, label{}",
            "CBNZ X17, label{}",
            "B label{}",
            "BL label{}",
            "MOV X18, #42",
            "CMP X19, X20",
            "BEQ label{}",
            "BNE label{}",
            "NOP",
            "NOP"
        ],

        # Block 6
        [
            "SUB X0, X1, X2",
            "ADD X3, X4, X5",
            "MUL X6, X7, X8",
            "SDIV X9, X10, X11",
            "NOP",
            "NOP",
            "CBZ X16, label{}",
            "CBNZ X17, label{}",
            "B label{}",
            "BL label{}",
            "MOV X18, #42",
            "CMP X19, X20",
            "BEQ label{}",
            "BNE label{}",
            "NOP",
            "NOP"
        ]
    ]

    # Calculate the number of code blocks required based on the available fundamental code blocks
    num_blocks = (8087 // len(code_blocks[0])) + 1

    # Generate the assembly code
    for i in range(0, num_blocks):
        block = random.choice(code_blocks)
        print(i, block)
        instructions.extend(labelled_block(block, i))
        if i < num_blocks:  # Add jumps at the end of each block except the last one 
            label = f"label{i+1}"                                                           
            labels.append(label)  
    # Adjust the number of instructions to reach 8087
    num_instructions = len(instructions)
    extra_instructions = 8087 - num_instructions
    if extra_instructions > 0:
        for _ in range(extra_instructions):
            instructions.append("NOP")  # Add NOP (No Operation) instructions

    # Write the assembly code to a file
    with open("test.s", "w") as file:
        file.write(".text\n\n")
        for i, instruction in enumerate(instructions):
            if i % len(code_blocks[0]) == 0 and i > 0:  # Print labels
                if labels:
                    file.write(f"\n{labels.pop(0)}:\n")
                else:
                    file.write(f"\nlabel{i+1}:\n")
            file.write(f"\t{instruction}\n")
        # Add a return instruction at the end
        if labels:
            file.write(f"\n{labels.pop(0)}:\n")
        else:
            file.write(f"\nlabel{num_blocks}:\n")
        file.write("\tMOV X0, #0\n")
        file.write("\tMOV X8, #93\n")
        file.write("\tSVC #0\n")

    print("instructions", len(instructions))

# Call the generate_assembly_code
generate_assembly_code()

# Assemble the code using the appropriate AArch64 assembler
assemble_command = ['as', '-g', '-o', 'test.o', 'test.s']
process = subprocess.Popen(assemble_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = process.communicate()

# Print the output
print("assemble_command stdout:\n", stdout.decode())
print("assemble_command stderr:\n", stderr.decode())

# Link the object file and create an executable
link_command = ['ld', '-g', '-o', 'test', 'test.o']
process = subprocess.Popen(link_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = process.communicate()

# Print the output
print("link_command stdout:\n", stdout.decode())
print("link_command stderr:\n", stderr.decode())
