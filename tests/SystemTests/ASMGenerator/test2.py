import random

COUNTER_END = 1000

def labelled_block(block, k, num_blocks):
    nblock = []
    substring = "{}"
    substring_GT= "B.GT"
    substring_LT= "B.LT"
    substring_EQ= "B.EQ"

    for j in block:
        instr = []
        for ins in j:
            x = ins.find(substring)
            if x != -1:
                GT = ins.find(substring_GT)
                LT = ins.find(substring_LT)
                EQ = ins.find(substring_EQ)
                if GT != -1 and k < num_blocks-1:
                    instr.append(ins.format(k  + 2))
                elif LT != -1 and k > 2:
                    instr.append(ins.format(k - 1))
                elif EQ != -1:
                    instr.append(ins.format(k + 1))
                else:
                    instr.append(ins.format(k + 1))

            else:
                instr.append(ins)
        nblock.append(instr)
    return nblock

def generate_assembly_code():
    # Array to hold the assembly program
    instructions = []
    # Array to hold the labels to be printed for each code block
    labels = []

    # Define the fundamental code blocks
  
    initialization_block=[
    '.global _start',
    '.section .data',
    '\tend_value: .quad {}'.format(COUNTER_END),
    '.section .text\n',
    '_start:',
    '\tmov x0, #0',
	'\tldr x1, =end_value',
	'\tldr x2, [x1]\n\n',
    ]
    
    operation_blocks = [
        [    
            'add x0, x0, #1',
            'add x1, x0, #1',          
            'add x3, x1, #1',
            'add x4, x3, #1',      
            'add x5, x4, #1',         
            'add x6, x5, #1',        
            'add x7, x6, #1',         
            'add x8, x7, #1',       
            'add x9, x8, #1',      
            'add x10, x9, #1',    
            'add x11, x10, #1',     
            'add x12, x11, #1',       
            'add x13, x12, #1',
            'add x14, x13, #1',      
            'add x15, x14, #1',       
        ],
        # Add more operation blocks here
        [            
            'add x0, x0, #1',
            'mov x3, #4',
            'mov x4, #5',
            'mov x5, #6',
            'mov x6, #7',
            'mov x7, #8',
            'mov x8, #9',
            'mov x3, #4',
            'mov x9, #10',
            'mov x10, #11',
            'mov x11, #12',
            'mov x12, #13',
            'mov x13, #14',
            'mov x14, #15',
            'mov x15, #16',
            'mov x16, #17'
        ]
    ]

    branching_blocks = [
        ["CMP x0, x2",
         "B.EQ label{}", 
         "B.GT label{}", 
         "B.LT label{}"
        ],
        ["CMP x0, x2",
         "B.EQ label{}", 
         "B.LT label{}",
         "B.GT label{}" 
        ],
        ["CMP x0, x2",
         "B.LT label{}",
         "B.EQ label{}", 
         "B.GT label{}"
        ],
        ["CMP x0, x2",
         "B.LT label{}",
         "B.GT label{}", 
         "B.EQ label{}" 
        ],
        ["CMP x0, x2",
         "B.GT label{}", 
         "B.EQ label{}", 
         "B.LT label{}"
        ],
        ["CMP x0, x2",
         "B.GT label{}",
         "B.LT label{}", 
         "B.EQ label{}" 
        ]
    ]
    
    num_blocks = (8087 // len(operation_blocks[0] + branching_blocks[0] + reset_counter_block[0])) + 1


    # Generate the assembly code
    for i in range(num_blocks):
        op_block = random.choices(operation_blocks)
        branch_block = random.choices(branching_blocks)
        instructions.extend(op_block)
        instructions.extend(labelled_block(branch_block, i, num_blocks))
        if i < num_blocks:  # Add jumps at the end of each block except the last one
            label = f"label{i + 1}"
            labels.append(label)

    # # Generate the assembly code for branching blocks
    # for i in range(num_branching_blocks):
    #     instructions.extend(labelled_block(block, i + num_operation_blocks))
    #     if i < num_branching_blocks:  # Add jumps at the end of each block except the last one
    #         label = f"label{i + num_operation_blocks + 1}"
    #         labels.append(label)

    # Adjust the number of instructions to reach 8087
    num_instructions = sum(len(block) for block in instructions)
    extra_instructions = 8087 - num_instructions
    if extra_instructions > 0:
        for _ in range(extra_instructions):
            instructions.append(["NOP"])  # Add NOP (No Operation) instructions

    # Write the assembly code to a file
    with open("test.s", "w") as file:
        for instruction in initialization_block:
            file.write(f"{instruction}\n")
        for i, block in enumerate(instructions):
            if i % 2 == 0 and i > 0:  # Print labels
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

    for block in instructions:
        for instruction in block:
            print(instruction)
    print('instructions', len(instructions))
    print('labels', len(labels))
    print('num_blocks', num_blocks)
# Call the generate_assembly_code
generate_assembly_code()
