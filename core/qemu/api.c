#ifdef __cplusplus
extern "C" {
#endif

#include "api.h"
#include <inttypes.h>


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

struct QEMU_callback_table QEMU_all_callbacks = {0, {NULL}};

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

//Could cause problems in api.h, to fix just have it always
//be __uint128_t
#ifdef TARGET_I386//128bit for x86 because the xmm regs are 128 bits long.
uint128_t QEMU_read_register(conf_object_t *cpu, int reg_index)
#else
uint64_t QEMU_read_register(conf_object_t *cpu, int reg_index)
#endif
{
	REQUIRES(cpu->type == QEMUCPUState);
    CPUState * qemucpu = cpu->object;
	return cpu_read_register(qemucpu, reg_index);
}
//get the physical memory for a given cpu TODO: WHAT DOES THIS RETURN
//Assuming it returns the AddressSpace of the cpu.
conf_object_t *QEMU_get_phys_memory(conf_object_t *cpu){
    //As far as I can tell it works.
    conf_object_t *as = malloc(sizeof(conf_object_t));
    as->type = QEMUAddressSpace;
    as->object = cpu_get_address_space(cpu->object);
    return as;
}


uint64_t QEMU_read_phys_memory(conf_object_t *cpu,
		physical_address_t pa, int bytes)
{
	REQUIRES(0 <= bytes && bytes <= 8);
    uint8_t buf[8];
	cpu_physical_memory_rw(pa, buf, bytes, 0);
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
	return cpu_proc_num(env);
}


conf_object_t *QEMU_get_all_processors(int *numCPUs)
{
	//ifdef CONFIG_SMP--doesn't work for some reason
	//so using smp_cpus isntead which seems to work.
	if (smp_cpus > 1) {
		QemuOpts * opts;

		opts = qemu_opts_find(qemu_find_opts("smp-opts"), NULL); 
		int c= qemu_opt_get_number(opts, "cpus", 0); 
		*numCPUs = c;
		int indexes[c];
		
		//this might not be needed, might be able to assume that cpu indexes
		//go from 0-#cpus
		cpu_pop_indexes(indexes);	
		conf_object_t * cpus = malloc(sizeof(conf_object_t)*c);

		int i = 0;
		while(i < c){
			cpus[i].object = qemu_get_cpu(indexes[i]);
			if(cpus[i].object){//probably not needed error checking
				cpus[i].type = QEMUCPUState;
			}
			i++;

		}
		return cpus;
	} else {
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
	return cpu_get_program_counter(qemucpu);
}

physical_address_t QEMU_logical_to_physical(conf_object_t *cpu, 
		data_or_instr_t fetch, logical_address_t va) 
{
    REQUIRES(cpu->type == QEMUCPUState);
    CPUState * qemucpu = cpu->object;

	return mmu_logical_to_physical(qemucpu, va);
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
int QEMU_insert_callback(QEMU_callback_event_t event, void* fn) 
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
