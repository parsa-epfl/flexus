import random
import subprocess

def generate_assembly_code():
    instructions = []
    labels = []

    # Define the assembly code blocks
    code_blocks = [
       # Block 1
       ["ADD x0, x1, x2", "MOV x3, #10", "LDR x4, [x0]", "CMP x4, #0"],
       # Block 2
       ["SUB x5, x6, x7", "MUL x8, x9, x10", "BNE label1", "STR x11, [x5]"],
       # Block 3
       ["LSL x12, x13, #2", "AND x14, x15, x12", "BLT label2", "MOV x15, #5"],
       # Block 4
       ["CMP x0, x1", "MOV x2, #15", "EOR x3, x4, x5", "BEQ label3"],
       # Block 5
       ["SUBS x6, x7, #1", "ADDS x8, x9, #1", "BGE label4", "LSR x10, x11, #3"],
       # Block 6
       ["BLT label5", "MOV x12, #0", "ORR x13, x14, x15", "CMP x0, x1"],
       # Block 7
       ["LDR x2, [x3]", "CMP x4, #0", "BNE label6", "BL label7"],
       # Block 8
       ["MOV x5, #10", "LDR x6, [x7]", "CMP x6, #0", "BEQ label8"],
       # Block 9
       ["STR x8, [x9]", "MOV x10, #20", "CMP x10, #0", "BNE label9"],
       # Block 10
       ["LDR x11, [x12]", "CMP x11, #0", "BEQ label10", "MOV x13, #30"],
       # Block 11
       ["ADD x14, x15, x2", "CMP x14, #0", "BNE label11", "LSR x3, x4, #3"],
       # Block 12
       ["MOV x5, #40", "CMP x5, #0", "BEQ label12", "AND x6, x7, x8"],
       # Block 13
       ["SUB x9, x10, x11", "CMP x9, #0", "BEQ label13", "ORR x12, x13, x14"],
       # Block 14
       ["ADD x15, x2, x3", "MOV x4, #50", "CMP x15, #0", "BNE label14"],
       # Block 15
       ["MOV x5, #60", "CMP x5, #0", "BEQ label15", "EOR x6, x7, x8"],
       # Block 16
       ["SUBS x9, x10, x11", "MOV x12, #70", "CMP x9, #0", "BEQ label16"]
    ]

    # Calculate the number of code blocks required
    num_blocks = (8087 // 4) + 1

    # Generate the assembly code
    for i in range(num_blocks):
        block = random.choice(code_blocks)
        instructions.extend(block)
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
    with open("ASMGENX.s", "w") as file:
        file.write("@ This is auto-generated assembly code\n\n")
        file.write(".text\n\n")
        for i, instruction in enumerate(instructions):
            if i % 4 == 0 and i > 0:  # Print labels
                file.write(f"\n{labels.pop(0)}:\n")
            file.write(f"\t{instruction}\n")
        # Add a return instruction at the end
        file.write(f"\n{labels.pop(0)}:\n")
        file.write("\tMOV X0, #0\n")
        file.write("\tMOV X8, #93\n")
        file.write("\tSVC #0\n")

generate_assembly_code()

# Assemble the code using the appropriate AArch64 assembler
assemble_command = ['as', '-o', 'ASMGENX.o', 'ASMGENX.s']
process = subprocess.Popen(assemble_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = process.communicate()

# Print the output
print("assemble_command stdout:\n", stdout.decode())
print("assemble_command stderr:\n", stderr.decode())

# Link the object file and create an executable
link_command = ['ld', '-o', 'ASMGENX', 'ASMGENX.o']
process = subprocess.Popen(link_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = process.communicate()

# Print the output
print("link_command stdout:\n", stdout.decode())
print("link_command stderr:\n", stderr.decode())
