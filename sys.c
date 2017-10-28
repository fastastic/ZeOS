// sys.c - Syscalls implementation
#include <devices.h>
#include <utils.h>
#include <io.h>
#include <mm.h>
#include <mm_address.h>
#include <sched.h>
#include <system.h>

#define LECTURA 0
#define ESCRIPTURA 1

//SYSTEM CALLS (RUTINAS DE SERVICIO)

int zeos_ticks;

int check_fd(int fd, int permissions) {
	if (fd!=1) return -9; /*EBADF*/
	if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
	return 0;
}

int sys_ni_syscall() {
	return -38; /*ENOSYS*/
}

int sys_getpid() {
	return current()->PID;
}

int sys_fork() {
	if (list_empty(&freequeue)) { /* miramos si hay algun pcb disponible */
		return -1; //deberia se return -ENOMEM pero no tenemos el errno.h implementado
	}
	struct list_head *free = list_first(&freequeue); /* obtenemos el primer elemento de la freequeue */
	list_del(free); /* eliminamos de la freequeue el elemento que hemos sacado */
	struct task_struct *pcbChild;
	pcbChild = list_head_to_task_struct(free); /* a partir del list_head sacamos el task_struct del proceso */
	union task_union *uChild = (union task_union*) pcbChild; /* convertimos a union task_union el task_struct, asi ya esta inicializado */

	struct task_struct *pcbFather = current();
	union task_union *uFather = (union task_union*) pcbFather; /* task union del padre */
	copy_data(uFather, uChild, sizeof(union task_union)); /* copiamos el task_union del padre en el hijo */

	allocate_DIR(pcbChild); /* inicializamos el directorio de paginas del proceso hijo*/

	page_table_entry *child_pag = get_PT(pcbChild); /* cogemos la pagina del proceso hijo */

	//SEGUIR A PARTIR DE AQUI (AP. 2.D)




	return 0;
	
}

void sys_exit() { 

}

int sys_write(int fd, char * buffer, int size) {
	int res;

	if ((res = check_fd(fd, ESCRIPTURA)) < 0) return -1;  // Comprova fd
	if (buffer == NULL) return -1;						// Comprova buffer
	if (size < 0) return -1;							// Comprova mida

	return sys_write_console(buffer, size);
}

int sys_gettime() {
	return zeos_ticks;
}
