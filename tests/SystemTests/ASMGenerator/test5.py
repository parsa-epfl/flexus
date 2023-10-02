import ctypes
import subprocess
def generate_cache_misses():
    	# Define the assembly instructions to generate cache misses
    	code = """
ldr x0, =first_array	// Replace with the address of your array
mov x1, #64
mov x2, #0
mov x6, #0

loop:
	ldr x3, [x0]
	add x0, x0, x1
	add x6, x6, #1
	ldr x4, [x0]
	cmp x6, #25
	beq exit_program
	b loop
exit_program:
	mov x8, #93
	mov x0, #0
	svc #0

.data
first_array: .word 0 , 10 , 20 , 30 , 40 , 50 , 60 , 70 , 80 , 90 , 100 , 110 , 12
   """

    	return code

# Write the assembly code to a file
with open('test.s', 'w') as file:
    	file.write(generate_cache_misses())
# Assemble the code using the appropriate AArch64 assembler
assemble_command = ['as', '-o', 'ASMGENX.o', 'test.s']
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
