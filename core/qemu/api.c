#ifdef __cplusplus
extern "C" {
#endif
#include "core/qemu/api.h"

#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)

#define dbg_printf(...)
#endif

#include "../flexus/core/qemu/api.h"
//Functions that don't do what they should
//--being left as dummy functions currently
//QEMU_set_tick_frequency
//QEMU_clear_exception --simics exceptions, not cpu exceptions
//
//QEMU_simulation_break --not really sure on this one.


//Functions I am not sure on(wasn't the last person to work on them)
//The callback functions


//Functions that I think do what they should, but are hard to test
//QEMU_read_register
//--SPARC64 -still have no idea what the DUDL register is
//Specific registers that are hard to test.
//--all floating point


//Functions that the formatting might be wrong on (and should be tested more)
//QEMU_read_phys_memory --not sure what all should be taken account of in terms of
//                 --endianess and byte ordering.
//mem_op_is functions --not 100% sure they are right, but I think they are easy


//functions that should be tested more, but I think are correct.
//QEMU_get_cpu_by_index
//QEMU_get_all_processors
//QEMU_get_processor_number
//QEMU_logical_to_physical--might not work with sparc Look at target-sparc/mmu_helper.c:848

//QEMU_get_program_counter

//-------------------------------------------------------------------
//functions that have been hard to test and should be focused on for testing.
//read_reg
//read_physical_memory --Also its formatting might need to change, or take context into account.
//Callback functions


//TODO: there is a sparc CPUTimer struct that might be used for set tick frequency

//[???]TODO functions that should be looked at more
//QEMU_read_physical_memory, TODO: but format might be off. It reads correctly, (tested against qemu monitor's xp which reads physical addresses)
//QEMU_logical_to_physical At least somewhat works, tested using in monitor.c with x(print virtual address), and xp(print physical address) 


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
    //There might not be anything like SIMICS exceptions, so for now leaving this function blank
    return 42;
}

//Defines for sparc floating point condition codes. 
//so FCC2 would be 
#ifndef TARGET_SPARC64
#define FSR_FCC2_SHIFT 100
#define FSR_FCC2 (1ULL << FSR_FCC2_SHIFT)
//and FCC3
#define FSR_FCC3_SHIFT 101
#define FSR_FCC3 (1ULL << FSR_FCC3_SHIFT)
#endif


//Could cause problems in api.h, to fix just have it always
//be __uint128_t
#ifdef TARGET_I386//128bit for x86 because the xmm regs are 128 bits long.
uint128_t qemu_read_register(conf_object_t *cpu, int reg_index)
#else
uint64_t QEMU_read_register(conf_object_t *cpu, int reg_index)
#endif
{
	REQUIRES(cpu->type == QEMUCPUState);
    
    CPUState * qemucpu = cpu->object;
    CPUArchState *env_ptr = qemucpu->env_ptr;
#ifdef TARGET_I386//tested using a function in monitor.c, and it seems to work
    if(reg_index<16){
        return (env_ptr->regs[reg_index]);//regs[reg_index] is a target_ulong
    }else if(reg_index<32){//xmm regs
#ifdef TARGET_X86_64//Currently having it return 128bits
        __uint128_t temp = (__uint128_t)env_ptr->xmm_regs[reg_index-16]._q[0] | (__uint128_t)env_ptr->xmm_regs[reg_index-16]._q[1]<<8*8; 
        return temp; 
#else//if it is 32bit, it only has 8 XMM regs
        if(reg_index<24){
            __uint128_t temp = (__uint128_t)env_ptr->xmm_regs[reg_index-16]._q[0] | (__uint128_t)env_ptr->xmm_regs[reg_index-16]._q[1]<<8*8; 
            return temp; 
        }
#endif
    }else if(reg_index<40){//MMx registers 
        //which are the mantissa part of the FP registers.
        return env_ptr->fpregs[reg_index-32].mmx.q;
        //return the mmx part of the fpureg, as a uint64
    }else{
//Masks for the Floating point condition flags.
#define C0 0x0100
#define C1 0x0200
#define C2 0x0400
#define C3 0x4000
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
                return (env_ptr->eflags);//Need to come up with some way to test 
                break;
                //From a website!
                //TODO look at more -> fpsr==fpus in qemu? register (floating point status/state register
                //C0 x01000 Condition bit 0
                //C1 0x0200 Condition bit 1
                //C2 0x0400 Condition bit 2
                //C3 0x4000 Condition bit 3
            case 49://C0    floating point cc flags
                return (env_ptr->fpus & C0);
                break;
            case 50://C1    floating point cc flags
                return (env_ptr->fpus & C1);
                break;
            case 51://C2    floating point cc flags
                return (env_ptr->fpus & C2);
                break;
            case 52://C3    floating point cc flags
                return (env_ptr->fpus & C3);
                break;
            case 53://TOP   floating point stack top
                return (env_ptr->fpstt);
                break;
            case 54://Not used  Dummy number that can be used.
             //   return (env_ptr->eflags);
                break;

        }
    }
   
#elif TARGET_SPARC64
    if(reg_index<8){//general registers
        return env_ptr->gregs[reg_index];
    }else if(reg_index<32){//register window regs
        return env_ptr->regwptr[reg_index-8];//Register window registers(i,o,l regs)
    }else if(reg_index<96){//floating point regs
        //since there are only 32 FP regs in qemu and they are each defined as a double, union two 32bit ints
        //I think that it might be that fp reg 0 will be reg 32 and reg 33  
        //fpr
        if(reg_index % 2){//the register is odd
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
            return (env_ptr->fsr & FSR_FCC0); //not sure what it is in qemusparc
            break;
        case 97://FCC1
            return (env_ptr->fsr & FSR_FCC1); //not sure what it is in qemusparc
    
            //This is what they are for FCC0 and 1
            //define FSR_FCC1_SHIFT 11
            //define FSR_FCC1  (1ULL << FSR_FCC1_SHIFT)
            //define FSR_FCC0_SHIFT 10
            //define FSR_FCC0  (1ULL << FSR_FCC0_SHIFT)

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
            //sounds specific to sparc64, do we care about 32bit sparc?
            return cpu_get_cwp64(env_ptr);//seems to actually work
            //there is no cpu_get_cwp(env). But I don't think it matters.
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
#endif
}
//get the physical memory for a given cpu TODO: WHAT DOES THIS RETURN
//Assuming it returns the AddressSpace of the cpu.
conf_object_t *QEMU_get_phys_memory(conf_object_t *cpu){
    //As far as I can tell it works.
    CPUState * Qcpu = cpu->object;
    conf_object_t *as = malloc(sizeof(conf_object_t));
    as->type = QEMUAddressSpace;
    as->object = Qcpu->as;
    return as;
}


uint64_t QEMU_read_phys_memory(conf_object_t *cpu,
		physical_address_t pa, int bytes)
{
	REQUIRES(0 <= bytes && bytes <= 8);
    char buf[8];
    cpu_physical_memory_read(pa,&buf, bytes);
    uint64_t t = 0;
    int i = 0;     
    while(i<4){
        //not sure on formatting. I think it should be buf[0]buf[1]buf[2]...buf[7] 
        t = t<<8;//shift one byte
        
        //TODO Might need to check endianess here currently matches qemu monitors output.
        //possible way to fix is two loops one gets first 4 bytes second gets next 4
        //where it is buf [3-0] and buf [7-4]
        //TODO figure out if this is the correct ordering of bytes.
        //possibly it should be the reverse, or something else 

#ifdef TARGET_X86_64//little endian
        t = (t | ((uint64_t)( buf[7-i] & 0x00000000000000ff))<<4*8);//read in the second word
        t = (t | ((uint64_t)( buf[3-i] & 0x00000000000000ff)));//read in the first word
#elif TARGET_SPARC64 //big endian(I think)

        t = (t | ((uint64_t)( buf[i+4] & 0x00000000000000ff)));//read in the second word
        t = (t | ((uint64_t)( buf[i] & 0x00000000000000ff))<<4*8);//read in the first word
#endif
        i++;
    }
    return t;
}

conf_object_t *QEMU_get_cpu_by_index(int index)
{
    //need to allocate memory 
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
//ifdef CONFIG_SMP--doesn't work for some reason
//so using smp_cpus isntead which seems to work.
    if(smp_cpus>1){
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
    }else{
        conf_object_t * cpus= malloc(sizeof(conf_object_t));
        cpus[0].object = qemu_get_cpu(0);
        cpus[0].type = QEMUCPUState;
        *numCPUs = 1;
        return cpus;
    }
}

int QEMU_set_tick_frequency(conf_object_t *cpu, double tick_freq) 
{
    return 42;
}
double QEMU_get_tick_frequency(conf_object_t *cpu){
    return 3.14;
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
#ifdef TARGET_I386//tested using a function in monitor.c, and it seems to work
    return (env_ptr->segs[R_CS].base + env_ptr->eip);
#elif TARGET_SPARC64//probably wrong
    return (env_ptr->pc);
#endif
}

physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va) 
{
    REQUIRES(cpu->type == QEMUCPUState);
//cpu_memory_rw_debug(ENV_GET_CPU(env), addr, buf, l, 0)
    CPUState * qemucpu = cpu->object;
    CPUArchState *env_ptr = qemucpu->env_ptr;
    //[???]fetch tells us if it is an instruction or data. possibly will be the exact same code for both
    //I think that the TARGET_x86_64 code will work for both, but am not 100% sure.
#ifdef TARGET_I386
    physical_address_t pa= cpu_get_phys_page_debug(cpu->object, (va & TARGET_PAGE_MASK));
    //assuming phys address and logical address are the right size
    //this gets the page then we need to do get the place in the page using va
    pa = pa + (va & ~TARGET_PAGE_MASK);
	return pa;
#elif TARGET_SPARC64
    physical_address_t pa= cpu_get_phys_page_debug(cpu->object, (va & TARGET_PAGE_MASK));
    //is immu different from dmmu?
    
    //assuming phys address and logical address are the right size
    //this gets the page then we need to do get the place in the page using va
   // logical_address_t mask = 0x0000000000000FFF; //The offset seems to be 12bits for 32bit or 64bit addresses
    pa = pa + (va & ~TARGET_PAGE_MASK);
	return pa;

#endif


    //could be wrong, but I think it does the same thing either way.
    if(fetch == QEMU_DI_Instruction){
        //do instruction things.
    }else{
        //do data things
    }
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

//[???]Not sure what this does if there is a simulation_break, shouldn't there be a simulation_resume? 
void QEMU_break_simulation(const char * msg)
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
    

    //I have not found anything that lets you send a message when you pause the simulation, but there can be a wakeup messsage.
    //in vl.c


    //possibly in cpus.c vm_stop, which takes in a state variable might not be resumeable 
    return;
}


// note: see QEMU_callback_table in api.h
// should return a unique identifier to the callback struct
int QEMU_insert_callback(QEMU_callback_event_t event, (void*) fn) 
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
    if(container == NULL){
        //Simple case there is not a callback function for event 
        containerNew->id = QEMU_all_callbacks.next_callback_id;
        containerNew->callback = fn;
        containerNew->next = NULL;
        QEMU_all_callbacks.callbacks[event] = containerNew;
    }else{
        //we need to add another callback to the chain
        containerNew->id = QEMU_all_callbacks.next_callback_id;
        containerNew->callback = fn;
        containerNew->next = NULL;
        //Now find the current last function in the callbacks list
        while(container->next!=NULL){
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
   //check if container == NULL//NULL might not work?
    if(container!=NULL){
        //now perform first check
        if(container->id == callback_id){
            //if container->next==NULL we do not need to do anything
           // if(container->next != NULL){
                QEMU_all_callbacks.callbacks[event] = container->next;
                //remove container from the linked list
          //  }
            free(container);//might not be how it is used.
            return;
        }
        prev = container;
        container = container->next;
        while(container!=NULL){
            if(container->id == callback_id){
                //if container->next==NULL we do not need to do anything
            //    if(container->next != NULL){
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



void QEMU_execute_callbacks(
		  QEMU_callback_event_t event
		, QEMU_callback_args_t *event_data
		)
{
	dbg_printf("Executing callbacks for event %d\n", event);
	QEMU_callback_container_t *curr = QEMU_all_callbacks.callbacks[event];
	for (; curr != NULL; curr = curr->next) {
		dbg_printf("Executing callback id %"PRId64"\n");
		void *callback = curr->callback;
		switch (event) {
			// noc : class_data, conf_object_t
			case QEMU_config_ready:
			case QEMU_continuation:
			case QEMU_asynchronous_trap:
			case QEMU_exception_return:
			case QEMU_magic_instruction:
			case QEMU_ethernet_network_frame:
			case QEMU_ethernet_frame:
			case QEMU_periodic_event:
				(*(cb_func_noc_t)callback)(
						  event_data->noc->class_data
						, event_data->noc->obj
						);
				break;
			// nocIs : class_data, conf_object_t, int64_t, char*
			case QEMU_simulation_stopped:
				(*(cb_func_nocIs_t)callback)(
						  event_data->nocIs->class_data
						, event_data->nocIs->obj
						, event_data->nocIs->bigint
						, event_data->nocIs->string
						);
				break;
			// nocs : class_data, conf_object_t, char*
			case QEMU_xterm_break_string:
			case QEMU_gfx_break_string:
				(*(cb_func_nocs_t)callback)(
						  event_data->nocs->class_data
						, event_data->nocs->obj
						, event_data->nocs->string
						);
				break;
			// ncm : conf_object_t, generic_transaction_t
			case QEMU_stc_miss:
				(*(cb_func_ncm_t)callback)(
						  event_data->ncm->space
						, event_data->ncm->trans
						);
				break;
			default:
				dbg_printf("Event not found...\n");
				break;
		}
	}
}

#ifdef __cplusplus
}
#endif
