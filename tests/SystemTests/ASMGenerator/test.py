import subprocess                                                                   
import random                                                                       
                                                                                    
def labelled_block(block, k):                                                               
        nblock = []                                                                         
        substring = "{}"                                                                    
                                                                                            
        for j in block:                                                                     
            x = j.find(substring)                                                           
            # print(x)                                                                      
            if(x!=-1):                                                                      
                # print("found:", j.format(k))                                              
                nblock.append( j.format(k+1))                                               
            else:                                                                           
                # print("Not found ", j)                                                    
                nblock.append(j)                                                            
        return nblock                                                                       
                                                                                            
                                                                                            
def generate_assembly_code(): 

    #array to hold the assembly program                                             
    instructions = []                                                                       
    #array to hold the labels to be printed to each code block                              
    labels = []                                                                             
    # Calculate the number of code blocks required                                          
    num_blocks = (8087// 4) + 1                                                             
    #define the fundamental code blocks                                                     
    code_blocks = [                                                                         
        # Block 3                                                                           
        ["LSL x12, x13, #2", "AND x14, x15, x12", "BLT label{}", "MOV x15, #5"],            
        # Block 4                                                                           
        ["CMP x0, x1", "MOV x2, #15", "EOR x3, x4, x5", "BEQ label{}"],                     
        # Block 5                                                                           
        ["SUBS x6, x7, #1", "ADDS x8, x9, #1", "BGE label{}", "LSR x10, x11, #3"],          
        # Block 6                                                                           
        ["BLT label{}", "MOV x12, #0", "ORR x13, x14, x15", "CMP x0, x1"],                  
        # Block 11                                                                          
        ["ADD x14, x15, x2", "CMP x14, #0", "BNE label{}", "LSR x3, x4, #3"],              
        # Block 12                                                                          
        ["MOV x5, #40", "CMP x5, #0", "BEQ label{}", "AND x6, x7, x8"],                     
        # Block 13                                                                          
        ["SUB x9, x10, x11", "CMP x9, #0", "BEQ label{}", "ORR x12, x13, x14"],             
        # Block 14                                                                          
        ["ADD x15, x2, x3", "MOV x4, #50", "CMP x15, #0", "BNE label{}"],                   
        # Block 15                                                                          
        ["MOV x5, #60", "CMP x5, #0", "BEQ label{}", "EOR x6, x7, x8"],                     
        # Block 16                                                                          
        ["SUBS x9, x10, x11", "MOV x12, #11", "CMP x9, #0", "BEQ label{}"]                  
    ]                                                                                       
                                                                                            
    # Generate the assembly code                                                            
    for i in range(0,num_blocks):                                                           
        block = random.choice(code_blocks)                                                  
        instructions.extend(labelled_block(block,i))                                        
        if i < num_blocks:  # Add jumps at the end of each block except the last one 
            label = f"label{i+1}"                                                           
            labels.append(label)                                                            
                                                                                            
    # Adjust the number of instructions to reach 8087                                       
    num_instructions = len(instructions)                                                    
    extra_instructions = 8087 - num_instructions                                            
    if extra_instructions > 0:                                                              
        for _ in range(extra_instructions):                                                 
            instructions.append("NOP")  # Add NOP (No Operation) instructions               
    print(instructions)                                                  
    # Write the assembly code to a file                                                     
    with open("test.s", "w") as file:                                                       
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
                                                                                            
    # for instruction_block in instructions:                                                  
    #     print(instruction_block)                                                            
                                                                                            
# call the generate_assembly code                                                           
generate_assembly_code()         

# # Assemble the code using the appropriate AArch64 assembler                                                                                                
# assemble_command = ['as', '-g', '-o', 'test.o', 'test.s']                                                            
# process = subprocess.Popen(assemble_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)                                 
# stdout, stderr = process.communicate()                                                                                                                     
                                                                                                                                                           
# # Print the output                                                                                                           
# print("assemble_command stdout:\n", stdout.decode())                                                                                                       
# print("assemble_command stderr:\n", stderr.decode())                                                                                                       
                                                                                                                        
# # Link the object file and create an executable                                                                                                            
# link_command = ['ld','-g', '-o', 'test', 'test.o']                                                                     
# process = subprocess.Popen(link_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)                                   
# stdout, stderr = process.communicate()                                                                                     
                                                                                                                                                           
# # Print the output                                                                                                                                         
# print("link_command stdout:\n", stdout.decode())                                                                             
# print("link_command stderr:\n", stderr.decode())                                                                             
                                                        
