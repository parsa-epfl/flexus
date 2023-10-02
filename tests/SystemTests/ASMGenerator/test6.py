import random


def generate_assembly_code():
    # Array to hold the assembly program
    instructions = []
    # Array to hold the labels to be printed for each code block
    labels = []

    # Define the fundamental code blocks
  
    initialization_block=[
    '.global _start',
    '.section .text\n',
    '_start:',
    '\tldr x0, =first_array',  
    '\tmov x1, #64',
    '\tmov x2, #0',
    '\tmov x6, #0',
    '\thint 92',
    '\thint 91',
    ]
    
    add_block=[
    [
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'add x0, x0, #4',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP'
    ]
]

    ldr_block = [
        [    
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'ldr x4, [x0]',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP',
        'NOP'
        ]
    ]

    data_blocks = [
    
        '.data',
        'first_array: .word 0 , 10 , 20 , 30 , 40 , 50 , 60 , 70 , 80 , 90 , 100 , 110 , 12'
    
    ]
    
    num_blocks = (16383 // 16) + 1


    # Generate the assembly code
    for i in range(0,num_blocks):
        if (i%16==0 and i>0):
            instructions.extend(ldr_block)
        else:
            instructions.extend(add_block)
        if i < num_blocks:  # Add jumps at the end of each block except the last one
            label = f"label{i + 1}"
            labels.append(label)
    num_instructions = sum(len(block) for block in instructions)
    extra_instructions = 16383 - num_instructions
    if extra_instructions > 0:
        for _ in range(extra_instructions):
            instructions.append([
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP",
                "NOP"
                ])  # Add NOP (No Operation) instructions

    # Write the assembly code to a file
    with open("test.s", "w") as file:
        for instruction in initialization_block:
            file.write(f"{instruction}\n")
        for i, block in enumerate(instructions):
            if i % 1 == 0 and i > 0:  # Print labels
                if labels:
                    file.write(f"\n{labels.pop(0)}:\n")
            for instruction in block:
                file.write(f"\t{instruction}\n")
        # Add a return instruction at the end
        if labels:
            file.write(f"\n{labels.pop(0)}:\n")
        file.write("\tMOV X0, #0\n")
        file.write("\tMOV X8, #93\n")
        file.write("\tSVC #0\n")

        for instruction in data_blocks:
            file.write(f"\n{instruction}\n")
        file.write("\nhint 92")
        file.write("\nhint 91")
    for block in instructions:
        for instruction in block:
            print(instruction)
    print('instructions', len(instructions))
    print('labels', len(labels))
    print('num_blocks', num_blocks)

# Call the generate_assembly_code
generate_assembly_code()
