import subprocess
import os
import mmap
import ctypes
import subprocess


# Your existing code here


# A1 = b"\x90\x8b\xc7\x83\xc0\x01\x83\xc0\x01\x83\xc0\x01\x83\xc0\x01\xc3"


assembly_code = b'\x90\x8b\xc7\x83\xc0\x01\x83\xc0\x01\x83\xc0\x01\x83\xc0\x01\x90'
ADD1 = b'\x83\xc0\x01\x90'
RET =  b"\x83\xc0\x01\xc3"
for i in range(0,8187):
   assembly_code = assembly_code + ADD1
assembly_code = assembly_code + RET


# assembly_code = A1.encode()
print(assembly_code)




buf = mmap.mmap(-1, 32768, prot=mmap.PROT_READ | mmap.PROT_WRITE | mmap.PROT_EXEC)


ftype = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int)
fpointer = ctypes.c_void_p.from_buffer(buf)


f = ftype(ctypes.addressof(fpointer))


buf.write(assembly_code)
r = f(41)
print("Bytes:", len(assembly_code)," r:", r)


del fpointer
buf.close()


# Write the assembly code to a file
with open('assembly_code.bin', 'wb') as file:
   file.write(assembly_code)


# Set the executable permission for the binary file
os.chmod('assembly_code.bin', 0o755)


# Run Valgrind using subprocess
# valgrind_command = ['valgrind', '--tool=memcheck', '--leak-check=yes', './assembly_code.bin']
# process = subprocess.Popen(valgrind_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
# stdout, stderr = process.communicate()


# # Print the output
# print("Valgrind stdout:\n", stdout.decode())
# print("Valgrind stderr:\n", stderr.decode())
