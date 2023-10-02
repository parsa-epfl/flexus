.global _start
.section .data
    end_value: .quad 1000000

.section .text

_start:
    mov x0, #0           // Set counter to 0

    // Load the end value into registers x1 and x2
    ldr x1, =end_value
    ldr x2, [x1]

loop:
    add x0, x0, #1       // Increment the counter
    cmp x0, x2           // Compare the counter to x2
    bne loop            // Loop until the counter reaches the target value

    // Repetitive code block to exceed the I1 cache size
    // Adjust the number of repetitions as needed
    block:
        // Insert a sequence of instructions here
        // The sequence should be large enough to exceed the I1 cache size
        // Repeat the instructions multiple times
        // For example, add instructions that perform arithmetic or load/store operations

        // Repeat the code block
        add x1, x0, #1          // Add 1 to x0 and store the result in x1
        add x3, x1, #1          // Add 1 to x1 and store the result in x3
        add x4, x3, #1          // Add 1 to x3 and store the result in x4
        add x5, x4, #1          // Add 1 to x4 and store the result in x5
        add x6, x5, #1          // Add 1 to x5 and store the result in x6
        add x7, x6, #1          // Add 1 to x6 and store the result in x7
        add x8, x7, #1          // Add 1 to x7 and store the result in x8
        add x9, x8, #1          // Add 1 to x8 and store the result in x9
        add x10, x9, #1         // Add 1 to x9 and store the result in x10
        add x11, x10, #1        // Add 1 to x10 and store the result in x11
        add x12, x11, #1        // Add 1 to x11 and store the result in x12
        add x13, x12, #1        // Add 1 to x12 and store the result in x13
        add x14, x13, #1        // Add 1 to x13 and store the result in x14
        add x15, x14, #1        // Add 1 to x14 and store the result in x15
    	cmp x5, x2 	// Compare counter to a large number (adjust as needed)
    	bne block

    // Exit the program
    mov x8, #93          // System call number for exit
    mov x0, #0           // Exit status code
    svc #0               // Perform the system call

.end


