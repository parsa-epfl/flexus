import random

def labelled_block(block, k):                                                               
        nblock = []                                                                         
        substring = "{}"       
        # standard converts the number to a 1-16 scale                            
        standard = k%32
        flipped = 32 - standard           
        # print(block)                       
        if( k%32 ==0):
            for j in block[0]:   
                # print(j)                                                                  
                x = j.find(substring)                                                           
                # print(x)                                                                      
                if(x!=-1):                                                                      
                    # print("found:", j.format(k))                                              
                    nblock.append( j.format(k+1))                                               
                else:                                                                           
                    # print("Not found ", j)                                                    
                    nblock.append(j) 
        elif(standard < 16 ):
            for j in block[0]:                                                                     
                x = j.find(substring)                                                           
                # print(x)                                                                      
                if(x!=-1):                                                                      
                    # print("found:", j.format(k + flipped - standard + 1))                                              
                    nblock.append( j.format( k + flipped - standard)   )                                            
                else:                                                                           
                    # print("Not found ", j)                                                    
                    nblock.append(j)
        elif(standard > 16 ):
            for j in block[0]:                                                                     
                x = j.find(substring)                                                           
                # print(x)                                                                      
                if(x!=-1):                                                                      
                    # print("found:", j.format(k + flipped - standard + 1))                                              
                    nblock.append( j.format( k + flipped - standard+1)   )                                            
                else:                                                                           
                    # print("Not found ", j)                                                    
                    nblock.append(j)             
        else:
            for j in block[0]:                                                                     
                x = j.find(substring)                                                           
                # print(x)                                                                      
                if(x!=-1):                                                                      
                    # print("found:", j.format(k + flipped - standard + 1))                                              
                    nblock.append( j.format( k +16)   )                                            
                else:                                                                           
                    # print("Not found ", j)                                                    
                    nblock.append(j)                                                                                                   
        print(k, nblock)            
        return nblock                                                                       
                                                                                            
                        
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
    '\tmov x1, #64',
    '\tmov x2, #0',
    '\tmov x6, #0',
    '\thint 92',
    '\thint 91',

    ]
    
    add_block=[
    [
        'B label{}'
    ]
]

    ldr_block = [
        [    

        'B label{}'
        ]
    ]
    
    num_blocks = (16383 // 1) + 1


    # Generate the assembly code
    for i in range(0,num_blocks):
        if (i%16==0 and i>0):
            instructions.extend(labelled_block(ldr_block,i))
        else:
            instructions.extend(labelled_block(add_block,i))
        if i < num_blocks:  # Add jumps at the end of each block except the last one
            label = f"label{i + 1}"
            labels.append(label)
    # print(instructions)
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
    # Write the assembly code to a file                                                     
    with open("test.s", "w") as file:          
        for instruction in initialization_block:
            file.write(f"{instruction}\n")                                                                                                          
        for i, instruction in enumerate(instructions):                                      
            if i % 1 == 0 and i > 0:  # Print labels                                        
                file.write(f"\n{labels.pop(0)}:\n")                                                                     
            file.write(f"\t{instruction}\n")                                                
        # Add a return instruction at the end      
                                         
        file.write(f"\n{labels.pop(0)}:\n")    
        file.write("\thint 92\n")
        file.write("\thint 91\n")                                             
        file.write("\tMOV X0, #0\n")                                                        
        file.write("\tMOV X8, #93\n")                                                       
        file.write("\tSVC #0\n")  


    # for block in instructions:
    #     for instruction in block:
    #         print(instruction)
    print('instructions', len(instructions))
    print('labels', len(labels))
    print('num_blocks', num_blocks)

# Call the generate_assembly_code
generate_assembly_code()
