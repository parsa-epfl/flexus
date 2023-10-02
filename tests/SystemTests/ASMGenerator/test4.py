def generate_nop_function():
    function_code = ''
    nop_instruction = '\tnop\n'
    return_instruction = '\tret\n'

    # Generate 32,767 NOP instructions
    for _ in range(32767):
        function_code += nop_instruction

    # Add return instruction
    function_code += return_instruction

    return function_code


def generate_aarch64_asm():
    asm_code = ''

    # Generate 10 functions
    for i in range(10):
        function_name = f'function{i}:'
        asm_code += function_name + '\n'

        # Generate NOP function
        nop_function = generate_nop_function()
        asm_code += nop_function

    # Call each function
    for i in range(10):
        function_call = f'bl function{i}\n'
        asm_code += function_call

    # Program exit
    asm_code += 'mov x8, #93\n'
    asm_code += 'mov x0, #0\n'
    asm_code += 'svc #0\n'

    return asm_code


# Generate AArch64 assembly code
assembly_code = generate_aarch64_asm()

# Save the assembly code to a file
with open('test.s', 'w') as f:
    f.write(assembly_code)
