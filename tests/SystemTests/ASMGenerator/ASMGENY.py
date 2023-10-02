import random
import subprocess
import os
import subprocess

def generate_assembly_code():
    instructions = []
    labels = []

    # Define the assembly code blocks
    code_blocks = [
        # Block 1
        ["ADD r0, r1, r2", "MOV r3, #10", "LDR r4, [r0]", "CMP r4, #0"],
        # Block 2
        ["SUB r5, r6, r7", "MUL r8, r9, r10", "BNE label1", "STR r11, [r5]"],
        # Block 3
        ["LSL r12, r13, #2", "AND r14, r15, r12", "BLT label2", "MOV r15, #5"],
        # Block 4
        ["CMP r0, r1", "MOV r2, #15", "EOR r3, r4, r5", "BEQ label3"],
        # Block 5
        ["SUBS r6, r7, #1", "ADDS r8, r9, #1", "BGE label4", "LSR r10, r11, #3"],
        # Block 6
        ["BLT label5", "MOV r12, #0", "ORR r13, r14, r15", "CMP r0, r1"],
        # Block 7
        ["LDR r2, [r3]", "CMP r4, #0", "BNE label6", "BL label7"],
        # Block 8
        ["MOV r5, #10", "LDR r6, [r7]", "CMP r6, #0", "BEQ label8"],
        # Block 9
        ["STR r8, [r9]", "MOV r10, #20", "CMP r10, #0", "BNE label9"],
        # Block 10
        ["LDR r11, [r12]", "CMP r11, #0", "BEQ label10", "MOV r13, #30"],
        # Block 11
        ["ADD r14, r15, r2", "CMP r14, #0", "BNE label11", "LSR r3, r4, #3"],
        # Block 12
        ["MOV r5, #40", "CMP r5, #0", "BEQ label12", "AND r6, r7, r8"],
        # Block 13
        ["SUB r9, r10, r11", "CMP r9, #0", "BEQ label13", "ORR r12, r13, r14"],
        # Block 14
        ["ADD r15, r2, r3", "MOV r4, #50", "CMP r15, #0", "BNE label14"],
        # Block 15
        ["MOV r5, #60", "CMP r5, #0", "BEQ label15", "EOR r6, r7, r8"],
        # Block 16
        ["SUBS r9, r10, r11", "MOV r12, #70", "CMP r9, #0", "BEQ label16"]
    ]

    # Calculate the number of code blocks required
    num_blocks = (8087 // 4) + 1

    # Generate the assembly code
    for i in range(num_blocks):
        block = random.choice(code_blocks)
        instructions.extend(block)
        if i < num_blocks - 1:  # Add jumps at the end of each block except the last one
            label = f"label{i+1}"
            labels.append(label)

    # Adjust the number of instructions to reach 8087
    num_instructions = len(instructions)
    extra_instructions = 8087 - num_instructions
    if extra_instructions > 0:
        for _ in range(extra_instructions):
            instructions.append("NOP")  # Add NOP (No Operation) instructions
    print("Length of Instructions:", len(instructions))
    # Write the assembly code to a file
    with open("ASMGENY.s", "w") as file:
        file.write("@ This is auto-generated assembly code\n\n")
        file.write(".text\n\n")
        for i, instruction in enumerate(instructions):
            if i % 4 == 0 and i > 0:  # Print labels
                file.write(f"\n{labels.pop(0)}:\n")
            file.write(f"\t{instruction}\n")
generate_assembly_code()

# Run build comamnds using subprocess
object_command = ['arm-linux-gnueabi-as', '-o', 'ASMGENY.o', 'ASMGENY.s']
executable_command = ['arm-linux-gnueabi-ld', '-o', 'ASMGENY', 'ASMGENY.o']

process = subprocess.Popen(object_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = process.communicate()
# Print the output
print("object_command stdout:\n", stdout.decode())
print("object_command stderr:\n", stderr.decode())


process = subprocess.Popen(executable_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
stdout, stderr = process.communicate()
# Print the output
print("executable_command stdout:\n", stdout.decode())
print("executable_command stderr:\n", stderr.decode())
ASMGENX