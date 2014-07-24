#ifdef __cplusplus
extern "C" {
#endif
#include "../flexus/core/qemu/api.h"

//[???]TODO
//Make sure read_reg is working correctly for SPARC, and x86.
//What all does it need to have access to.
//what register does the int map to?
//for sparg is it like 0-7 maps to general.
//what about floating point regs,  mmu regs and dmmu regs.

//Other sparc registers
//aregs[8](alternate general regs), 
//bregs[8](backup for normal regs),
//iregs[8](interrupt general regs)
//mregs[8](mmu general regs)

//possibly can figure out how to use register window for sparc
//regwptr;
//wim = window invalid mask
//cwp = index of current register window

//TODO: there is a saprc CPUTimer struct that might be used for set tick
//frequency
//QEMU_set_tick_frequency --LEAVING AS A DUMMY FUNCTION FOR NOW--

//[???]TODO functions that should be looked at more

//QEMU_clear_exception
//QEMU_simulation_break
//QEMU_read_physical_memory, TODO: but format might be off. It reads correctly, (tested against qemu monitor's xp which reads physical addresses)
//QEMU_logical_to_physical At least somewhat works, tested using in monitor.c with x(print virtual address), and xp(print physical address) 
//TODO: the general idea works, it might not be the best way to do it. 
//And it might need to be somewhat cpu specific.(at least 32/64bit specific)

//QEMU_mem_op_is_write
//QEMU_mem_op_is_read
//QEMU_mem_op_is_data --not entirely sure what this is supposed to do.
//QEMU_insert_callback
//QEMU_delete_callback

//[???]TODO functions that I think do what they should
//QEMU_read_register
//QEMU_get_program_counter
//QEMU_get_cpu_by_index, TODO: tested with one cpu( QEMU_get_cpu_by_index(0), works) Should be tested more.

//[???]Notes:
//We might want to avoid using CPUState->env_ptr if we can avoid it, because env_ptr is architecture specific 
//functions currently specific to X86
//QEMU_read_register
//QEMU_get_program_counter


int QEMU_clear_exception(void)
{
    //[???]Not sure what this function can access. 
    //functions in qemu-2.0.0/include/qom/cpu.h might be useful if they are useable
    //not sure if this is how to use FOREACH
//    CPUState *cpu;
//    CPU_FOREACH(cpu){
//        cpu_reset_interrupt(cpu, -1);//but this needs a CPUState and a mask//really have no idea what this does
        //mask = -1 since it should be 0xFFFF...FF for any sized int.
        //and we want to clear all exceptions(?)
        
        //cpu_reset(cpu); // this might be what we want instead--probably not
        //since it completely resets the cpu, so basically rebooting. 
    //}
    //I am not sure if there is anyway to return an exception number,
    //because each cpu could have a different exception error
  
    //There might not be anything like SIMICS exceptions, so for now leaving this function blank
    return 42;
}

//TODO not sure if these are how it should be done, or if they work at all.
//so FCC2 would be 
#define FSR_FCC2_SHIFT 100
#define FSR_FCC2 (1ULL << FSR_FCC2_SHIFT)
            //and FCC3
#define FSR_FCC3_SHIFT 101
#define FSR_FCC3 (1ULL << FSR_FCC3_SHIFT)
uint64_t QEMU_read_register(conf_object_t *cpu, int reg_index)
{
    
	REQUIRES(cpu->type == QEMUCPUState);
    //[???]Need to find what cpu->object will have when it is a QEMUCPUState to finish. 
    //It will probably be something simple like return cpu->object->registers[reg_index]
    //------------------Probably not what we are looking for ----------------------
    //I think the cpustate will be from QEMU-2.0.0/target-i386/cpu.h:772-941
    //this might be the standard format for what we need here
    //but each processor could be different.
    //-----------------------------------------------------------------------------
    //There is the CPUState and functions in include/qom/cpu.h that is looking like it
    //has what we need. AND conf_object_t will probably point to a general CPUState
    
    //The function we want might be in CPUClass gdb_read_register, but this might just be
    //for debugging.
    
    //Looking into it more how qemu does it in their monitor is using env->cpu_core_dump,
    //but I am not really sure how it works, and I think it does it by printing.
    //And within the function it gets the registers by doing env_ptr->regs.
    //So I think this should work.
    //TODO figuring out how they get regs in monitor.c might be worth it.
    //So we don't have to right the code to get registers for each target.
    //cpu_dump_state(CPUState, FILE*, fprintf_function, int flags)
    //possibly
    //cpu_dump_state(cpu, stderr, &fprintf, CPU_DUMP_FPU); prints out registers, 
    //to use this will need a parse function.
    /*output of cpu_dump_state with CPU_DUMP_FPU flag
        EAX=00007000 EBX=fff0ff06 ECX=0001114d EDX=00000000
        ESI=000f6120 EDI=00000006 EBP=fffffff0 ESP=00006db8
        EIP=000f0daa EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
        ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
        CS =0008 00000000 ffffffff 00cf9b00 DPL=0 CS32 [-RA]
        SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
        DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
        FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
        GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
        LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
        TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
        GDT=     000f6688 00000037
        IDT=     000f66c6 00000000
        CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
        DR0=0000000000000000 DR1=0000000000000000 DR2=0000000000000000 DR3=0000000000000000 
        DR6=00000000ffff0ff0 DR7=0000000000000400
        EFER=0000000000000000
        FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80
        FPR0=0000000000000000 0000 FPR1=0000000000000000 0000
        FPR2=0000000000000000 0000 FPR3=0000000000000000 0000
        FPR4=0000000000000000 0000 FPR5=0000000000000000 0000
        FPR6=0000000000000000 0000 FPR7=0000000000000000 0000
        XMM00=00000000000000000000000000000000 XMM01=00000000000000000000000000000000
        XMM02=00000000000000000000000000000000 XMM03=00000000000000000000000000000000
        XMM04=00000000000000000000000000000000 XMM05=00000000000000000000000000000000
        XMM06=00000000000000000000000000000000 XMM07=00000000000000000000000000000000
        */


    //Currently both of these are probably wrong.
    //SPARC is using a register window, which I don't really get. 
    //x86 can't access all registers
    //And I have no idea how flexus maps registers. 
    //So I have no idea what reg 0 ==s 
    //
    CPUState * qemucpu = cpu->object;
    CPUArchState *env_ptr = qemucpu->env_ptr;
#ifdef TARGET_I386
    //not sure how exactly to handle xmm regs since there are 16, but the monitor only displays 8
    if(reg_index<16){
        return (uint64_t)(env_ptr->regs[reg_index]);//regs[reg_index] is a target_ulong
    }else if(reg_index<32){//xmm regs
        //So this returns a 64bit number
        //so how is it supposed to deal with 128bits???
        //possibly it only reads the first 8 xmm registers?
        //
    }else if(reg_index<40){//MM registers 
        //which are part of the FP registers.
        return env_ptr->fpregs[reg_index-32].mmx.q;
        //return the mmx part of the fpureg, as a uint64
    }else{
        //cpu_compute_eflags(CPUX86State); might do what we want.
 /*102  eflags masks 
 103 #define CC_C    0x0001
 104 #define CC_P    0x0004
 105 #define CC_A    0x0010
 106 #define CC_Z    0x0040
 107 #define CC_S    0x0080
 108 #define CC_O    0x0800
 109 
 110 #define TF_SHIFT   8
 111 #define IOPL_SHIFT 12
 112 #define VM_SHIFT   17
 113 
 114 #define TF_MASK                 0x00000100
 115 #define IF_MASK                 0x00000200
 116 #define DF_MASK                 0x00000400
 117 #define IOPL_MASK               0x00003000
 118 #define NT_MASK                 0x00004000
 119 #define RF_MASK                 0x00010000
 120 #define VM_MASK                 0x00020000
 121 #define AC_MASK                 0x00040000
 122 #define VIF_MASK                0x00080000
 123 #define VIP_MASK                0x00100000
 124 #define ID_MASK                 0x00200000
*/ 
        //possibly helper_read_eflags(CPUX86State) from target-i386/cc_helper.c:332 Would be useful.
        //it contains:
        //uint32_t eflags;
        //eflags = cpu_cc_compute_all(env, CC_OP);
        //eflags |= (env->df & DF_MASK);
        //eflags |= env->eflags & ~(VM_MASK | RF_MASK);
        //return eflags;
        //          
        //ONLY PROBLEM WITH USING EFLAGS IS NOT ALL OF IT IS CC regs aren't set DF and CC flags are set to 0
        //At least that is what target-i386/cpu.h says
        //
        //So far using env_ptr->eflags seems to be working, but should be tested more.
        switch(reg_index){
            case 40://pc
                return (env_ptr->segs[R_CS].base + env_ptr->eip);
                break;
            case 41://CF    integer condition code flags
                return (env_ptr->eflags & CC_C);
                break;
            case 42://DST   DST field is used to cache PF flag//not sure about this
                return (env_ptr->eflags & CC_P);
                break;
            case 43://AF integer condition code flags
                return (env_ptr->eflags & CC_A);
                break;
            case 44://ZF integer condition code flags
                return (env_ptr->eflags & CC_Z);
                break;
            case 45://SF integer condition code flags
                return (env_ptr->eflags & CC_S);
                break;
            case 46://OF integer condition code flags
                return (env_ptr->eflags & CC_O);
                break;
            case 47://DF integer condition code flags
                if(env_ptr->df==-1){//DF = 1//df is the 11th bit
                    return 0x00000400;
                }else if(env_ptr->df==1){//DF = 0
                    return 0;
                }
                break;
            case 48://EFLAGS    the whole 32 bits eflags
                return (env_ptr->eflags);
                break;
            case 49://C0    floating point cc flags
                return (env_ptr->eflags);
                break;
            case 50://C1    floating point cc flags
                return (env_ptr->eflags);
                break;
            case 51://C2    floating point cc flags
                return (env_ptr->eflags);
                break;
            case 52://C3    floating point cc flags
                return (env_ptr->eflags);
                break;
            case 53://TOP   floating point stack top
                return (env_ptr->fpstt);
                break;
            case 54://Not used  Dummy number that can be used.
             //   return (env_ptr->eflags);
                break;

        }
    }
    
#elif TARGET_SPARC64//probably wrong--also I think v9==sparc64
   //TODO look more at target-sparc/win_helper.c:284 --has to do with pstate and general regs 
    if(reg_index<8){//general registers
        return env_ptr->gregs[reg_index];
    }else if(reg_index<32){//register window regs
        return env_ptr->regwptr[reg_index-8];//Register window registers(i,o,l regs)
    }else if(reg_index<96){//floating point regs
        //since there are only 32 FP regs in qemu and they are each defined as a double, union two 32bit ints
        //I think that it might be that fp reg 0 woill be reg 32 and reg 33  
        //fpr
        if(reg_index % 2){//the register is odd
            //number/2 is always positive if it was an odd number we do the upper?
            //Possibly should make a new variable for clarity.
            //reg_index/2 because each register number is being counted twice(fp 0 is reg_index 0_lower(or upper?) and reg_index 1 is fp 0 upper(or lower)
            //- 16 is because 32/2 = 16 and we need to subtract -32 since we are starting the fp regs at reg_index 32 instead of 0.
            return (env_ptr->fpr[reg_index/2-16]).l.lower;//lower based on monitor.c:~4000(search for "fpr[0].l.upper
        }else{
            return (env_ptr->fpr[reg_index/2-16]).l.upper;//upper?
        } 
    }else{//all specific cases now
        switch(reg_index){
        case 96://FCC0
            //FFC == Floating point condition code registers--so possibly part of the floating point state register?
            //Possibly something to do with the xcc register?
            return (env_ptr->fsr & FSR_FCC0); //not sure what it is in qemusparc
            break;
        case 97://FCC1
            return (env_ptr->fsr & FSR_FCC1); //not sure what it is in qemusparc
            //but there isn't FSR_FCC2 or 3... 
    
            //This is what they are for FCC0 and 1
            //define FSR_FCC1_SHIFT 11
            //define FSR_FCC1  (1ULL << FSR_FCC1_SHIFT)
            //define FSR_FCC0_SHIFT 10
            //define FSR_FCC0  (1ULL << FSR_FCC0_SHIFT)

            //possibly for 2 and 3 it is just going to be adding 2 or 3 to 0
            //so FCC2 would be 
            //define FSR_FCC2_SHIFT 100
            //define FSR_FCC2 (1ULL << FSR_FCC_2_SHIFT)
            //and FCC3
            //define FSR_FCC3_SHIFT 101
            //define FSR_FCC3 (1ULL << FSR_FCC_3_SHIFT)
            break;
        case 98://FCC2
            return (env_ptr->fsr & FSR_FCC2);
            break;
        case 99://FCC3
            return (env_ptr->fsr & FSR_FCC3);//FIXME no idea if these work or how to test
            break;
        case 100://CC
            //return env_ptr-> //might be harder to do since qemu does lazy cc codes.
            return cpu_get_ccr(env_ptr);//not sure if this is right(ccr might not be cc);
            //possibly needs some bit manipulation?
            break;
        case 101://PC
            return env_ptr->pc; 
            break;
        case 102://NPC
            return env_ptr->npc; 
            break;
        case 103://AEXC --part of fsr
            return (env_ptr->fsr & FSR_AEXC_MASK); 
            break;
        case 104://CEXC --part of fsr
            return (env_ptr->fsr & FSR_CEXC_MASK); 
            break;
        case 105://FTT --part of fsr
            return (env_ptr->fsr & FSR_FTT_MASK); //isn't defined for some reason, will have to look more at target-sparc/cpu.h:183
            break;
        case 106://DUDL --part of fprs
            //return (env_ptr->fsr & FSR_FTT_MASK);//haven't even found anything about the DUDL reg with googling.
            break;
        case 107://FEF --part of fprs
            return (env_ptr->fsr & FPRS_FEF);
            break;
        case 108://Y
            return (env_ptr->y);
            break;
        case 109://GSR
            return (env_ptr->gsr);
            break;
        case 110://CANSAVE
            return (env_ptr->cansave);
            break;
        case 111://CANRESTORE
            return (env_ptr->canrestore);
            break;
        case 112://OTHERWIN
            return (env_ptr->otherwin);
            break;
        case 113://CLEANWIN
            return (env_ptr->cleanwin);
            break;
        case 114://CWP
            //return (env_ptr->cwp);//currently wrong for some reason, gave 5 when QEMU monitor.c gave 2
            //---I think the problem might have been caused by me not stoping the simulation
            //before checking the values. So during the time between checking with monitor.c's function
            //and my own it had changed.
            
            //sounds specific to sparc64, do we care about 32bit sparc?
            return cpu_get_cwp64(env_ptr);//seems to actually work
            break;
        case 115://ASI
            return (env_ptr->asi);
            break;
        //116-123 arent in the simics thing so they probably aren't used?
        //THESE MIGHT BE SIMICS SPECIFIC?
        case 124://Not_Used --not sure what this means 
            //return (env_ptr->asi);
            break;
        case 125://Sync --not sure what this means
            //return (env_ptr->asi);
            break;
        } 
    }
#else
    return -1;//error
#endif
    //NOTE: will probably be best to write the code to access registers for each target.
    //This is because even if we don't to be able to search through the results of
    //cpu_dump_state, we would need to have a large list of the definitions
    //for the name of each register.
}

uint32_t QEMU_read_phys_memory(conf_object_t *cpu,
		physical_address_t pa, int bytes)
{
	REQUIRES(0 <= bytes && bytes <= 8);
    //[???]If this can read up to 8 bytes shouldn't it return a uint64_t to store that much?
    //Could be wrong and this isn't how the function works though.
    //The function we want might be in CPUClass memory_rw_debug, but I am not sure, it wants
    //a vaddr so maybe not. 
    //Possibly cpu_unassigned_access is the function we are looking for. 
    //needs CPUState, hwaddr(I think ==phys)
    //bool is_write, bool is_exec, int opaque, int size
    //not sure how it returns anything.
	
    //There is a function cpu_physical_memory_read
    char buf[8];
    cpu_physical_memory_read(pa,&buf, bytes);
    //need to switch buf to int somehow probably
    //probably something silly like
    uint64_t t = 0;
    int i = 0;     
    while(i!=4){//while loop because compiler complained about using a for loop
        //not sure on formatting. I think it should be buf[0]buf[1]buf[2]...buf[7] 
        t = t<<8;//shift one byte
        //Might need to check endianess here currently matches qemu monitors output.
        
        //possible way to fix is two loops one gets first 4 bytes second gets next 4
        //where it is buf [3-0] and buf [7-4]
        //now works for 32bit words since as it is now it is buf[7-0]
        t = (t | ((uint64_t)( buf[7-i] & 0x00000000000000ff)));//read in the second word
        t = (t | ((uint64_t)( buf[3-i] & 0x00000000000000ff))<<4*8);//read in the first word
        //printf("\naddress: Ox%x%x\n",t>>8*4, t);//printf only outputs 32bits
        //so we need to do two output statements
        i++;
    }
    return t;
}

conf_object_t *QEMU_get_cpu_by_index(int index)
{
    //[???]This assumes that the code will have access to some list of cpus
    //I am not sure where that list is so I am not sure how to do this
    //need to allocate memory? 
    conf_object_t *cpu = malloc(sizeof(conf_object_t));
    //where will it get deleted?
    cpu->object = qemu_get_cpu(index);
    if(cpu->object){
        cpu->type = QEMUCPUState;
    }
    ENSURES(cpu->type == QEMUCPUState);
	return cpu;
}


int QEMU_get_processor_number(conf_object_t *cpu){
    CPUState * env = cpu->object;
    return env->cpu_index;
}


conf_object_t *QEMU_get_all_processors(int *numCPUs)
{
    QemuOpts * opts;
    opts = qemu_opts_find(qemu_find_opts("smp-opts"), NULL); 
    int c= qemu_opt_get_number(opts, "cpus", 0); 
	*numCPUs = c;
    int size = sizeof(conf_object_t);
    //conf_object_t * cpus = malloc(c*size);
    CPUState *cpu;
    int indexes[c];
    int i = 0;
    //this might not be needed, might be able to assume that cpu indexes
    //go from 0-#cpus
    CPU_FOREACH(cpu) {
        
  /*      cpus[i].object = cpu;
        if(cpus[i].object){
            cpus[i].type = QEMUCPUState;
        }*/
        indexes[i] = cpu->cpu_index;
        i++;
    }
    conf_object_t * cpus = malloc(size*c);
    i = 0;
    while(i < c){
        cpus[i].object = qemu_get_cpu(indexes[i]);
        if(cpus[i].object){//probably not needed error checking
            cpus[i].type = QEMUCPUState;
        }
        i++;
        
    }
    return cpus;
}

int QEMU_set_tick_frequency(conf_object_t *cpu, double tick_freq) 
{
    //[???]Need to figure out what QEMUCPUState has to do, but it will probably be
    //something like
    //cpu->object->tick_freq = tick_freq;
    //with some error checking code as it returns. return -1 if error?
	//Possibly it isn't as simple as just updating
    //So far I haven't been able to find anything on how to update
    //tick_freq in QEMUs CPUState struct for target-i386
    return 42;
}

uint64_t QEMU_get_program_counter(conf_object_t *cpu) 
{
	REQUIRES(cpu->type == QEMUCPUState);
    //[???]In QEMU the PC is updated only after translation blocks not sure where it is stored
    //1194     *cs_base = env->segs[R_CS].base;
    //1195     *pc = *cs_base + env->eip;
    //from cpu.h.
    //This looks like how they get the cpu there so I will just do that here(env_ptr is a CPUX86State pointer
    //which is what I think cpu->object is.
    //requires x86

    CPUState * qemucpu = cpu->object;
    CPUArchState *env_ptr = qemucpu->env_ptr;
#ifdef TARGET_i386//tested using a function in monitor.c, and it seems to work
    return (env_ptr->segs[R_CS].base + env_ptr->eip);
#elif TARGET_SPARC64//probably wrong
    return (env_ptr->pc);
#else
    return -1;//error--problem, no way to tell its an error
    //probably should just remove the #else and return -1; 
#endif
}

physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va) 
{
	REQUIRES(cpu->type == QEMUCPUState);
    //[???]fetch tells us if it is an instruction or data. possibly will be the exact same code for both
    physical_address_t pa= cpu_get_phys_page_debug(cpu->object, va);
    //assuming phys address and logical address are the right size
    //this gets the page then we need to do get the place in the page using va
    logical_address_t mask = 0x0000000000000FFF; //The offset seems to be 12bits for 32bit or 64bit addresses
    pa = pa + (va & mask);

    //could be wrong, but I think it does the same thing either way.
    if(fetch == QEMU_DI_Instruction){
        //do instruction things.
    }else{
        //do data things
        }
	return pa;
}

//------------------------Should be fairly simple here-------------------------

//[???] I assume these are treated as booleans were 1 = true, 0 = false
int QEMU_mem_op_is_data(generic_transaction_t *mop)
{
    //[???]not sure on this one
    //possibly
    //if it is read or write. Might also need cache?
    if((mop->type == QEMU_Trans_Store) || (mop->type == QEMU_Trans_Load)){
        return 1;
    }else{
        return 0;
    }
}

int QEMU_mem_op_is_write(generic_transaction_t *mop)
{
//[???]Assuming Store == write.
    if(mop->type == QEMU_Trans_Store){
        return 1;
    }else{
        return 0;
    }
}

int QEMU_mem_op_is_read(generic_transaction_t *mop)
{
//[???]Assuming load == read.
    if(mop->type == QEMU_Trans_Load){
        return 1;
    }else{
        return 0;
    }
}
// note: see QEMU_callback_table in api.h
// should return a unique identifier to the callback struct
int QEMU_insert_callback(QEMU_callback_event_t event, QEMU_callback_t fn, void *arg) 
{
    //[???]use next_callback_id then update it
    //If there are multiple callback functions, we must chain them together.
    //error checking-
    if(event>=QEMU_callback_event_count){
        //call some sort of errorthing possibly
        return -1;
    }
    QEMU_callback_container_t *container = QEMU_all_callbacks.callbacks[event];
    //Probably should error check
    //Also not sure if I should be using alloc here.
    QEMU_callback_container_t *containerNew = malloc(sizeof(QEMU_callback_container_t));
    if(container == null){
        //Simple case there is not a callback function for event 
        containerNew->id = QEMU_all_callbacks.next_callback_id;
        containerNew->callback = fn;
        containerNew->callback_arg = arg;
        containerNew->next = null;
        QEMU_all_callbacks.callbacks[event] = containerNew;
    }else{
        //we need to add another callback to the chain
        containerNew->id = QEMU_all_callbacks.next_callback_id;
        containerNew->callback = fn;
        containerNew->callback_arg = arg;
        containerNew->next = null;
        //Now find the current last function in the callbacks list
        while(container->next!=null){
            container = container->next;
        }
        container->next = containerNew;
    }
    QEMU_all_callbacks.next_callback_id++;
    //not sure how to check for errors, but if there are any return -1
    return containerNew->id;
}

void QEMU_delete_callback(QEMU_callback_event_t event, uint64_t callback_id)
{
    //need to point prev->next to current->next
    //start with the first in the list and check its ID
    QEMU_callback_container_t *container = QEMU_all_callbacks.callbacks[event];
    QEMU_callback_container_t *prev;
    //primary goal find the correct id
    //check if container == null//null might not work?
    if(container!=null){
        //now perform first check
        if(container->id == callback_id){
            //if container->next==null we do not need to do anything
           // if(container->next != null){
                QEMU_all_callbacks.callbacks[event] = container->next;
                //remove container from the linked list
          //  }
            free(container);//might not be how it is used.
            return;
        }
        prev = container;
        container = container->next;
        while(container!=null){
            if(container->id == callback_id){
                //if container->next==null we do not need to do anything
            //    if(container->next != null){
                    prev->next = container->next;
                    //remove container from the linked list
              //  }
                free(container);//might not be how it is used.
                return;
            }
            prev = container;
            container = container->next;
        }
    }
    //if it isn't there don't do anything.
    return;
}


//[???]Not sure what this does
void QEMU_simulation_break(void)
{
    //[???]it could be pause_all_vcpus(void)
    //Causes the simulation to pause, can be restarted in qemu monitor by calling stop then cont
    //or can be restarted by calling resume_all_vcpus();
    //looking at it some functtions in vl.c might be useful
    pause_all_vcpus();
	
    //qemu_system_suspend();//from vl.c:1940//doesn't work at all
    //calls pause_all_vcpus(), and some other stuff.
    //For QEMU to know that they are paused I think.
    //qemu_system_suspend_request();//might be better from vl.c:1948
    //sort of works, but then resets cpus I think
    return;
}

#ifdef __cplusplus
}
#endif
