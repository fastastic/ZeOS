/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */


struct task_struct *list_head_to_task_struct(struct list_head *l) {
  return (struct task_struct*)((int)l & (0xfffff000)); /* hay que decirle al compilador que la direccion obtenida haciendo la mascara es la direccion de un task_struct, de ahi hacer la mascara */
}


extern struct list_head blocked;

struct list_head freequeue; /* declaración de la freequeue */
struct list_head readyqueue; /* declaración de la readyqueue */
struct task_struct *idle_task; /* declaración del idle_task */

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void) {
	struct list_head *free = list_first(&freequeue); /* obtenemos el primer elemento de la freequeue */
	list_del(free); /* eliminamos de la freequeue el elemento que hemos sacado */
	struct task_struct *pcbIdle;
	pcbIdle = list_head_to_task_struct(free); /* a partir del list_head sacamos el task_struct del proceso */
	pcbIdle->PID = 0; /* asignamos el PID 0 al proceso */
	allocate_DIR(pcbIdle); /* inicializamos el directorio de paginas del proceso */

	union task_union *uIdle = (union task_union*) pcbIdle; /* convertimos a union task_union el task_struct, asi ya esta inicializado */
	uIdle->stack[1023] = (unsigned long) &cpu_idle; /* asignamos en la ultima posición de la pila la dirección de la función pcb_idle */
	uIdle->stack[1022] = 0; /* ebp = 0 */
	pcbIdle->reg_kernel_esp = (unsigned long)&(uIdle->stack[1022]); /* asignamos al campo del task_struct de pcbIdle reg_kernel_esp el valor inicial del registro ebp, la linia de arriba, osea 0 */
	idle_task = pcbIdle; /* hacemos que la variable global idle_task sea igual a pcbIdle */

}

void init_task1(void) {
	struct list_head *free = list_first(&freequeue); /* obtenemos el primer elemento de la freequeue */
	list_del(free); /* eliminamos de la freequeue el elemento que hemos sacado */
	struct task_struct *pcbInit;
	pcbInit = list_head_to_task_struct(free); /* a partir del list_head sacamos el task_struct */
	pcbInit->PID = 1; /* asignamos el PID 1 al proceso */ 
	allocate_DIR(pcbInit); /* inicializamos el directorio de paginas del proceso task */
	set_user_pages(pcbInit); /* asignamos páginas físicas al proceso y añadimos la traducción a la tabla de páginas */

	union task_union *uInit = (union task_union*) pcbInit; /* convertimos a union task_union el task_struct, asi ya esta inicializado */
	tss.esp0 = (unsigned long)&(uInit->stack[KERNEL_STACK_SIZE]); // hacemos que el registro esp0 de la tss apunte a la pila de sistema, no estoy seguro
	set_cr3(pcbInit->dir_pages_baseAddr); /* el registro cr3 pasa a apuntar al directorio de págianas de pcb_init */
}


void init_sched(){
	INIT_LIST_HEAD(&freequeue); /* inicializamos la freequeue */

	for (int i=0; i<NR_TASKS; ++i) {
		/* cogemos del vector task que contiene 10 task_union el list_head y los encolamos en la readyqueue */
		list_add_tail(&(task[i].task.list), &freequeue);		
	}

	INIT_LIST_HEAD(&readyqueue); /* inicializamos la readyqueue */

}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

void inner_task_switch(union task_union*t) {
	tss.esp0 = (unsigned long)&(t->stack[KERNEL_STACK_SIZE]); //  actualizamos la tss haciendo que el registro esp0 apunte a la cima de la pila de sistema de t
	set_cr3(&((t->task).dir_pages_baseAddr)); // hacemos que cr3 apunte al directorio de paginas del task struct de t
}

void task_switch(union task_union*t) {
	/* 1- se guardan los registros esi, edi y ebx
	 * 2- se llama a inner_task_switch con el task_union del proceso nuevo como parámetro
	 * 3- se restauran los registros esi, edi y ebx
	 */

	__asm__ ("push %esi");
	__asm__ ("push %edi");
	__asm__ ("push %ebx");
	inner_task_switch(t);
	__asm__ ("pop %ebx");
	__asm__ ("pop %edi");
	__asm__ ("pop %esi");
}

/* continuar a partir de inner_task_switch */

