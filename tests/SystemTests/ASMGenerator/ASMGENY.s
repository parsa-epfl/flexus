@ This is auto-generated assembly code

.text

	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label2:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label3:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label4:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label5:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label6:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label7:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label8:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label9:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label10:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label11:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label12:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label13:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label14:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label15:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label16:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label17:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label18:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label19:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label20:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label21:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label22:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label23:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label24:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label25:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label26:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label27:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label28:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label29:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label30:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label31:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label32:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label33:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label34:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label35:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label36:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label37:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label38:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label39:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label40:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label41:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label42:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label43:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label44:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label45:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label46:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label47:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label48:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label49:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label50:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label51:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label52:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label53:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label54:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label55:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label56:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label57:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label58:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label59:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label60:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label61:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label62:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label63:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label64:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label65:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label66:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label67:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label68:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label69:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label70:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label71:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label72:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label73:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label74:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label75:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label76:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label77:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label78:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label79:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label80:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label81:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label82:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label83:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label84:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label85:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label86:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label87:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label88:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label89:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label90:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label91:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label92:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label93:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label94:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label95:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label96:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label97:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label98:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label99:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label100:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label101:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label102:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label103:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label104:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label105:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label106:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label107:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label108:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label109:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label110:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label111:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label112:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label113:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label114:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label115:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label116:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label117:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label118:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label119:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label120:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label121:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label122:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label123:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label124:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label125:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label126:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label127:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label128:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label129:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label130:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label131:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label132:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label133:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label134:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label135:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label136:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label137:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label138:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label139:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label140:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label141:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label142:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label143:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label144:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label145:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label146:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label147:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label148:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label149:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label150:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label151:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label152:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label153:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label154:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label155:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label156:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label157:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label158:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label159:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label160:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label161:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label162:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label163:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label164:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label165:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label166:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label167:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label168:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label169:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label170:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label171:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label172:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label173:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label174:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label175:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label176:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label177:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label178:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label179:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label180:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label181:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label182:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label183:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label184:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label185:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label186:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label187:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label188:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label189:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label190:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label191:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label192:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label193:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label194:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label195:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label196:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label197:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label198:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label199:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label200:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label201:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label202:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label203:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label204:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label205:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label206:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label207:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label208:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label209:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label210:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label211:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label212:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label213:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label214:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label215:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label216:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label217:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label218:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label219:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label220:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label221:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label222:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label223:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label224:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label225:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label226:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label227:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label228:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label229:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label230:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label231:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label232:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label233:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label234:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label235:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label236:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label237:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label238:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label239:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label240:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label241:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label242:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label243:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label244:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label245:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label246:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label247:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label248:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label249:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label250:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label251:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label252:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label253:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label254:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label255:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label256:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label257:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label258:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label259:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label260:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label261:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label262:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label263:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label264:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label265:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label266:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label267:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label268:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label269:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label270:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label271:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label272:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label273:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label274:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label275:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label276:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label277:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label278:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label279:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label280:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label281:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label282:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label283:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label284:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label285:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label286:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label287:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label288:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label289:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label290:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label291:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label292:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label293:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label294:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label295:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label296:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label297:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label298:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label299:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label300:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label301:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label302:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label303:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label304:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label305:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label306:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label307:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label308:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label309:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label310:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label311:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label312:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label313:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label314:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label315:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label316:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label317:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label318:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label319:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label320:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label321:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label322:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label323:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label324:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label325:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label326:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label327:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label328:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label329:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label330:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label331:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label332:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label333:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label334:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label335:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label336:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label337:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label338:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label339:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label340:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label341:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label342:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label343:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label344:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label345:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label346:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label347:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label348:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label349:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label350:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label351:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label352:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label353:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label354:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label355:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label356:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label357:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label358:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label359:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label360:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label361:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label362:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label363:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label364:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label365:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label366:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label367:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label368:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label369:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label370:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label371:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label372:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label373:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label374:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label375:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label376:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label377:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label378:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label379:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label380:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label381:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label382:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label383:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label384:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label385:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label386:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label387:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label388:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label389:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label390:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label391:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label392:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label393:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label394:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label395:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label396:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label397:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label398:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label399:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label400:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label401:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label402:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label403:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label404:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label405:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label406:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label407:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label408:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label409:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label410:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label411:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label412:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label413:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label414:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label415:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label416:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label417:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label418:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label419:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label420:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label421:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label422:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label423:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label424:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label425:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label426:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label427:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label428:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label429:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label430:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label431:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label432:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label433:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label434:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label435:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label436:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label437:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label438:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label439:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label440:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label441:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label442:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label443:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label444:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label445:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label446:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label447:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label448:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label449:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label450:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label451:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label452:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label453:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label454:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label455:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label456:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label457:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label458:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label459:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label460:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label461:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label462:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label463:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label464:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label465:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label466:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label467:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label468:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label469:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label470:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label471:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label472:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label473:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label474:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label475:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label476:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label477:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label478:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label479:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label480:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label481:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label482:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label483:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label484:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label485:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label486:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label487:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label488:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label489:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label490:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label491:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label492:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label493:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label494:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label495:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label496:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label497:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label498:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label499:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label500:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label501:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label502:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label503:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label504:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label505:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label506:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label507:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label508:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label509:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label510:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label511:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label512:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label513:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label514:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label515:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label516:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label517:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label518:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label519:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label520:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label521:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label522:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label523:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label524:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label525:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label526:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label527:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label528:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label529:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label530:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label531:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label532:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label533:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label534:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label535:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label536:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label537:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label538:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label539:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label540:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label541:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label542:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label543:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label544:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label545:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label546:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label547:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label548:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label549:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label550:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label551:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label552:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label553:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label554:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label555:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label556:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label557:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label558:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label559:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label560:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label561:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label562:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label563:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label564:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label565:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label566:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label567:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label568:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label569:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label570:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label571:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label572:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label573:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label574:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label575:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label576:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label577:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label578:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label579:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label580:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label581:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label582:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label583:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label584:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label585:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label586:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label587:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label588:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label589:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label590:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label591:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label592:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label593:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label594:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label595:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label596:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label597:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label598:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label599:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label600:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label601:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label602:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label603:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label604:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label605:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label606:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label607:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label608:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label609:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label610:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label611:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label612:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label613:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label614:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label615:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label616:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label617:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label618:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label619:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label620:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label621:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label622:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label623:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label624:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label625:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label626:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label627:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label628:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label629:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label630:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label631:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label632:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label633:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label634:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label635:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label636:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label637:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label638:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label639:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label640:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label641:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label642:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label643:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label644:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label645:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label646:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label647:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label648:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label649:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label650:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label651:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label652:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label653:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label654:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label655:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label656:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label657:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label658:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label659:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label660:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label661:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label662:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label663:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label664:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label665:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label666:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label667:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label668:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label669:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label670:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label671:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label672:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label673:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label674:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label675:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label676:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label677:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label678:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label679:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label680:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label681:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label682:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label683:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label684:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label685:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label686:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label687:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label688:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label689:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label690:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label691:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label692:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label693:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label694:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label695:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label696:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label697:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label698:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label699:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label700:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label701:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label702:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label703:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label704:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label705:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label706:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label707:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label708:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label709:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label710:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label711:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label712:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label713:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label714:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label715:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label716:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label717:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label718:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label719:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label720:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label721:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label722:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label723:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label724:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label725:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label726:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label727:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label728:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label729:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label730:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label731:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label732:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label733:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label734:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label735:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label736:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label737:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label738:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label739:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label740:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label741:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label742:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label743:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label744:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label745:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label746:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label747:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label748:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label749:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label750:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label751:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label752:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label753:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label754:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label755:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label756:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label757:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label758:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label759:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label760:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label761:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label762:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label763:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label764:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label765:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label766:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label767:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label768:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label769:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label770:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label771:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label772:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label773:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label774:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label775:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label776:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label777:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label778:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label779:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label780:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label781:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label782:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label783:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label784:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label785:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label786:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label787:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label788:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label789:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label790:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label791:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label792:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label793:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label794:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label795:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label796:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label797:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label798:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label799:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label800:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label801:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label802:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label803:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label804:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label805:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label806:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label807:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label808:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label809:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label810:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label811:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label812:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label813:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label814:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label815:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label816:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label817:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label818:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label819:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label820:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label821:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label822:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label823:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label824:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label825:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label826:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label827:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label828:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label829:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label830:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label831:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label832:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label833:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label834:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label835:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label836:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label837:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label838:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label839:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label840:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label841:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label842:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label843:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label844:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label845:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label846:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label847:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label848:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label849:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label850:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label851:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label852:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label853:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label854:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label855:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label856:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label857:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label858:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label859:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label860:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label861:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label862:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label863:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label864:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label865:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label866:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label867:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label868:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label869:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label870:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label871:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label872:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label873:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label874:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label875:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label876:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label877:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label878:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label879:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label880:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label881:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label882:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label883:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label884:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label885:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label886:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label887:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label888:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label889:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label890:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label891:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label892:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label893:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label894:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label895:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label896:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label897:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label898:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label899:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label900:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label901:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label902:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label903:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label904:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label905:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label906:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label907:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label908:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label909:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label910:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label911:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label912:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label913:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label914:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label915:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label916:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label917:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label918:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label919:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label920:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label921:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label922:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label923:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label924:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label925:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label926:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label927:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label928:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label929:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label930:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label931:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label932:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label933:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label934:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label935:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label936:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label937:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label938:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label939:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label940:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label941:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label942:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label943:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label944:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label945:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label946:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label947:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label948:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label949:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label950:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label951:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label952:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label953:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label954:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label955:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label956:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label957:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label958:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label959:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label960:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label961:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label962:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label963:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label964:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label965:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label966:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label967:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label968:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label969:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label970:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label971:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label972:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label973:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label974:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label975:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label976:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label977:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label978:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label979:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label980:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label981:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label982:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label983:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label984:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label985:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label986:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label987:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label988:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label989:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label990:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label991:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label992:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label993:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label994:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label995:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label996:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label997:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label998:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label999:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1000:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1001:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1002:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1003:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1004:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1005:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1006:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1007:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1008:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1009:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1010:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1011:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1012:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1013:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1014:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1015:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1016:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1017:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1018:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1019:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1020:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1021:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1022:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1023:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1024:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1025:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1026:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1027:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1028:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1029:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1030:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1031:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1032:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1033:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1034:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1035:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1036:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1037:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1038:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1039:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1040:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1041:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1042:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1043:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1044:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1045:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1046:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1047:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1048:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1049:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1050:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1051:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1052:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1053:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1054:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1055:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1056:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1057:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1058:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1059:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1060:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1061:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1062:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1063:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1064:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1065:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1066:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1067:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1068:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1069:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1070:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1071:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1072:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1073:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1074:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1075:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1076:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1077:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1078:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1079:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1080:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1081:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1082:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1083:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1084:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1085:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1086:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1087:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1088:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1089:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1090:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1091:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1092:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1093:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1094:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1095:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1096:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1097:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1098:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1099:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1100:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1101:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1102:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1103:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1104:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1105:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1106:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1107:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1108:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1109:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1110:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1111:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1112:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1113:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1114:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1115:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1116:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1117:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1118:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1119:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1120:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1121:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1122:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1123:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1124:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1125:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1126:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1127:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1128:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1129:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1130:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1131:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1132:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1133:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1134:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1135:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1136:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1137:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1138:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1139:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1140:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1141:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1142:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1143:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1144:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1145:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1146:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1147:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1148:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1149:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1150:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1151:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1152:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1153:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1154:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1155:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1156:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1157:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1158:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1159:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1160:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1161:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1162:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1163:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1164:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1165:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1166:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1167:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1168:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1169:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1170:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1171:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1172:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1173:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1174:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1175:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1176:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1177:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1178:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1179:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1180:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1181:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1182:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1183:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1184:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1185:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1186:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1187:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1188:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1189:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1190:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1191:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1192:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1193:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1194:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1195:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1196:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1197:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1198:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1199:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1200:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1201:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1202:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1203:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1204:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1205:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1206:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1207:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1208:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1209:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1210:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1211:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1212:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1213:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1214:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1215:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1216:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1217:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1218:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1219:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1220:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1221:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1222:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1223:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1224:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1225:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1226:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1227:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1228:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1229:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1230:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1231:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1232:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1233:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1234:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1235:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1236:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1237:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1238:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1239:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1240:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1241:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1242:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1243:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1244:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1245:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1246:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1247:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1248:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1249:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1250:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1251:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1252:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1253:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1254:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1255:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1256:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1257:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1258:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1259:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1260:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1261:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1262:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1263:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1264:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1265:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1266:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1267:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1268:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1269:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1270:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1271:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1272:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1273:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1274:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1275:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1276:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1277:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1278:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1279:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1280:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1281:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1282:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1283:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1284:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1285:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1286:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1287:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1288:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1289:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1290:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1291:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1292:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1293:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1294:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1295:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1296:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1297:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1298:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1299:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1300:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1301:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1302:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1303:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1304:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1305:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1306:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1307:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1308:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1309:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1310:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1311:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1312:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1313:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1314:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1315:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1316:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1317:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1318:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1319:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1320:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1321:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1322:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1323:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1324:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1325:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1326:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1327:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1328:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1329:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1330:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1331:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1332:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1333:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1334:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1335:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1336:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1337:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1338:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1339:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1340:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1341:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1342:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1343:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1344:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1345:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1346:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1347:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1348:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1349:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1350:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1351:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1352:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1353:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1354:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1355:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1356:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1357:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1358:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1359:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1360:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1361:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1362:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1363:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1364:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1365:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1366:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1367:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1368:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1369:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1370:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1371:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1372:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1373:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1374:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1375:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1376:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1377:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1378:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1379:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1380:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1381:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1382:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1383:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1384:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1385:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1386:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1387:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1388:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1389:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1390:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1391:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1392:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1393:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1394:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1395:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1396:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1397:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1398:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1399:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1400:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1401:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1402:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1403:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1404:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1405:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1406:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1407:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1408:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1409:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1410:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1411:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1412:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1413:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1414:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1415:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1416:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1417:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1418:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1419:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1420:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1421:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1422:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1423:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1424:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1425:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1426:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1427:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1428:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1429:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1430:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1431:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1432:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1433:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1434:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1435:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1436:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1437:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1438:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1439:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1440:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1441:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1442:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1443:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1444:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1445:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1446:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1447:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1448:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1449:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1450:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1451:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1452:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1453:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1454:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1455:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1456:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1457:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1458:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1459:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1460:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1461:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1462:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1463:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1464:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1465:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1466:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1467:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1468:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1469:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1470:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1471:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1472:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1473:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1474:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1475:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1476:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1477:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1478:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1479:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1480:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1481:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1482:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1483:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1484:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1485:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1486:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1487:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1488:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1489:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1490:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1491:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1492:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1493:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1494:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1495:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1496:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1497:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1498:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1499:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1500:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1501:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1502:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1503:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1504:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1505:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1506:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1507:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1508:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1509:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1510:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1511:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1512:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1513:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1514:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1515:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1516:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1517:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1518:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1519:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1520:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1521:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1522:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1523:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1524:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1525:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1526:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1527:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1528:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1529:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1530:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1531:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1532:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1533:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1534:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1535:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1536:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1537:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1538:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1539:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1540:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1541:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1542:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1543:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1544:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1545:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1546:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1547:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1548:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1549:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1550:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1551:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1552:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1553:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1554:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1555:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1556:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1557:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1558:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1559:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1560:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1561:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1562:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1563:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1564:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1565:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1566:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1567:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1568:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1569:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1570:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1571:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1572:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1573:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1574:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1575:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1576:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1577:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1578:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1579:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1580:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1581:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1582:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1583:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1584:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1585:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1586:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1587:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1588:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1589:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1590:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1591:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1592:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1593:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1594:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1595:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1596:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1597:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1598:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1599:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1600:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1601:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1602:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1603:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1604:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1605:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1606:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1607:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1608:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1609:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1610:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1611:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1612:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1613:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1614:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1615:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1616:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1617:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1618:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1619:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1620:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1621:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1622:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1623:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1624:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1625:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1626:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1627:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1628:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1629:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1630:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1631:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1632:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1633:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1634:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1635:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1636:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1637:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1638:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1639:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1640:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1641:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1642:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1643:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1644:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1645:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1646:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1647:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1648:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1649:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1650:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1651:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1652:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1653:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1654:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1655:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1656:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1657:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1658:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1659:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1660:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1661:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1662:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1663:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1664:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1665:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1666:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1667:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1668:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1669:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1670:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1671:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1672:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1673:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1674:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1675:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1676:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1677:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1678:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1679:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1680:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1681:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1682:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1683:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1684:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1685:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1686:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1687:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1688:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1689:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1690:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1691:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1692:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1693:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1694:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1695:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1696:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1697:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1698:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1699:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1700:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1701:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1702:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1703:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1704:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1705:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1706:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1707:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1708:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1709:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1710:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1711:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1712:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1713:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1714:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1715:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1716:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1717:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1718:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1719:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1720:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1721:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1722:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1723:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1724:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1725:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1726:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1727:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1728:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1729:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1730:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1731:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1732:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1733:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1734:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1735:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1736:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1737:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1738:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1739:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1740:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1741:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1742:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1743:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1744:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1745:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1746:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1747:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1748:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1749:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1750:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1751:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1752:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1753:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1754:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1755:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1756:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1757:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1758:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1759:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1760:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1761:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1762:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1763:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1764:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1765:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1766:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1767:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1768:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1769:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1770:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1771:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1772:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1773:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1774:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1775:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1776:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1777:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1778:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1779:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1780:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1781:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1782:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1783:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1784:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1785:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1786:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1787:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1788:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1789:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1790:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1791:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1792:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1793:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1794:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1795:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1796:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1797:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1798:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1799:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1800:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1801:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1802:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1803:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1804:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1805:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1806:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1807:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1808:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1809:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1810:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1811:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1812:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1813:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1814:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1815:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1816:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1817:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1818:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1819:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1820:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1821:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1822:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1823:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1824:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1825:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1826:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1827:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1828:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1829:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1830:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1831:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1832:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1833:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1834:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1835:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1836:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1837:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1838:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1839:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1840:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1841:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1842:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1843:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1844:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1845:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1846:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1847:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1848:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1849:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1850:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1851:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1852:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1853:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1854:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1855:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1856:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1857:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1858:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1859:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1860:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1861:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1862:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1863:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1864:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1865:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1866:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1867:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1868:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1869:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1870:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1871:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1872:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1873:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1874:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1875:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1876:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1877:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1878:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1879:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1880:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1881:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1882:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1883:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1884:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1885:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1886:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1887:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1888:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1889:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1890:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1891:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1892:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1893:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1894:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1895:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1896:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1897:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1898:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1899:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1900:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1901:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1902:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1903:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1904:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1905:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1906:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1907:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1908:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1909:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1910:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1911:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1912:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1913:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1914:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1915:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1916:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1917:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1918:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1919:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1920:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1921:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1922:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1923:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1924:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1925:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1926:
	ADD r0, r1, r2
	MOV r3, #10
	LDR r4, [r0]
	CMP r4, #0

label1927:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1928:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1929:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1930:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1931:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1932:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1933:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1934:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1935:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1936:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1937:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1938:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1939:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1940:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1941:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1942:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1943:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1944:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1945:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label1946:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1947:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1948:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1949:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1950:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1951:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1952:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1953:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1954:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1955:
	MOV r5, #10
	LDR r6, [r7]
	CMP r6, #0
	BEQ label8

label1956:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1957:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1958:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1959:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1960:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label1961:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1962:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1963:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1964:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1965:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1966:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1967:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1968:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1969:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1970:
	MOV r5, #60
	CMP r5, #0
	BEQ label15
	EOR r6, r7, r8

label1971:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1972:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1973:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1974:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1975:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1976:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1977:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1978:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1979:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1980:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1981:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label1982:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1983:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label1984:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1985:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1986:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1987:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1988:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label1989:
	STR r8, [r9]
	MOV r10, #20
	CMP r10, #0
	BNE label9

label1990:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label1991:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1992:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label1993:
	SUB r9, r10, r11
	CMP r9, #0
	BEQ label13
	ORR r12, r13, r14

label1994:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1995:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label1996:
	ADD r14, r15, r2
	CMP r14, #0
	BNE label11
	LSR r3, r4, #3

label1997:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label1998:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label1999:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label2000:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label2001:
	SUBS r9, r10, r11
	MOV r12, #70
	CMP r9, #0
	BEQ label16

label2002:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label2003:
	SUB r5, r6, r7
	MUL r8, r9, r10
	BNE label1
	STR r11, [r5]

label2004:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label2005:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label2006:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label2007:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label2008:
	SUBS r6, r7, #1
	ADDS r8, r9, #1
	BGE label4
	LSR r10, r11, #3

label2009:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label2010:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5

label2011:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label2012:
	LDR r11, [r12]
	CMP r11, #0
	BEQ label10
	MOV r13, #30

label2013:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label2014:
	ADD r15, r2, r3
	MOV r4, #50
	CMP r15, #0
	BNE label14

label2015:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label2016:
	LDR r2, [r3]
	CMP r4, #0
	BNE label6
	BL label7

label2017:
	BLT label5
	MOV r12, #0
	ORR r13, r14, r15
	CMP r0, r1

label2018:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label2019:
	CMP r0, r1
	MOV r2, #15
	EOR r3, r4, r5
	BEQ label3

label2020:
	MOV r5, #40
	CMP r5, #0
	BEQ label12
	AND r6, r7, r8

label2021:
	LSL r12, r13, #2
	AND r14, r15, r12
	BLT label2
	MOV r15, #5
