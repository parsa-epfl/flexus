#ifdef __cplusplus
extern "C" {
#endif
#include "core/qemu/api.h"
//[???]TODO functions
//QEMU_read_phys_memory
//QEMU_set_tick_frequency
//QEMU_logical_to_physical
//QEMU_simulation_break

//[???]TODO functions that should be looked at more
//All of them there is a more general CPUState that should probably be used
//
//QEMU_clear_exception

//[???]TODO functions that I think do what they should
//QEMU_read_register

//[???]Notes:
//We might want to avoid using CPUState->env_ptr if we can avoid it, because env_ptr is architecture specific 
//functions currently specific to X86
//QEMU_read_register
//QEMU_get_program_counter
//

int QEMU_clear_exception(void)
{
    //[???]Not sure what this function can access. 
    //functions in qemu-2.0.0/include/qom/cpu.h might be useful if they are useable
    //not sure if this is how to use FOREACH
    CPUState *cpu;
    CPU_FOREACH(cpu){
        cpu_reset_interrupt(cpu, -1);//but this needs a CPUState and a mask
        //mask = -1 since it should be 0xFFFF...FF for any sized int.
        //and we want to clear all exceptions(?)

        //cpu_reset(cpu); // this might be what we want instead 
    }
    //I am not sure if there is anyway to return an exception number,
    //because each cpu could have a different exception error
    return 42;
}

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
#ifdef TARGET_I386
    return (uint64_t)cpu->object->env_ptr->regs[reg_index];//regs[reg_index] is a target_ulong
#endif
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
	return 42;
}

conf_object_t *QEMU_get_cpu_by_index(int index)
{
    //[???]This assumes that the code will have access to some list of cpus
    //I am not sure where that list is so I am not sure how to do this
    
    //qemu_get_cpu returns a CPUState probably needs something other then just casting.
    //cpu->object = qemu_get_cpu(index)
    //cpu->type = QEMUCPUState
    conf_object_t *cpu = (conf_object_t*)qemu_get_cpu(index);
    ENSURES(cpu->type == QEMUCPUState);
	return cpu;
}

int QEMU_set_tick_frequency(conf_object_t *cpu, double tick_freq) 
{
    //[???]Need to figure out what QEMUCPUState has to do, but it will probably be
    //something like
    //cpu->object->tick_freq = tick_freq;
    //with some error checking code as it returns. return -1 if error?
	//Possibly it isn't as simple as just updating
    //So far I haven't been able to find anything on how to update
    // tick_freq in QEMUs CPUState struct for target-i386
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
    return (cpu->object->env_ptr->segs[R_CS].base + cpu->object->env_ptr->eip);
}

physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
					data_or_instr_t fetch, logical_address_t va) 
{
    //[???]fetch tells us if it is an instruction or data. possibly will be the exact same code for both
    if(fetch == QEMU_DI_Instruction){
        //do instruction things.
    }else{
        //do data things
        }
	REQUIRES(cpu->type == QEMUCPUState);
	return 42;
}

//----------------------------Should be fairly simple here----------------

//[???] I assume these are treated as booleans were 1 = true, 0 = false
int QEMU_mem_op_is_data(generic_transaction_t *mop)
{
    //[???]not sure on this one
    //possibly
    //if it is read or write. 
    if((mop->type == QEMU_Trans_Store) || (mop->type == QEMU_Trand_Load)){
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
        QEMU_all_callbacks[event] = containerNew;
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
    //check if container == null
    if(container!=null){
        //now perform first check 
        while(container!=null){
            if(container->id == callback_id){
                //if container->next==null we do not need to do anything
                if(container->next != null){
                    prev->next = container->next;
                    //remove container from the linked list
                }
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
    //[???] it might be as simple as calling cpu_exit(CPUState *cpu) but we don't have a cpustate
    //or it could be pause_all_vcpus(void)
    pause_all_vcpus();
	return;
}

#ifdef __cplusplus
}
#endif
