#define MAX_IDT 256
#define INTERRUPT_GATE 0x8E
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define KB_DATA	0x60
#define KB_STATUS 0x64
#include <sys/kprintf.h>
#include <sys/defs.h>
#include <sys/portio.h>
#include <stdlib.h>
#include <sys/strings.h>
#include <sys/utils.h>
#include <sys/process.h>
#include <sys/paging.h>
#include <sys/gdt.h>
#include <sys/syscall.h>
#include <sys/tarfs.h>
#include <sys/elf64.h>
#include <sys/mem.h>
#include <sys/env.h>
#include "kb_map.h"
#define MAX_SCRIPT_CMDS 50
extern int X;
extern int Y;
extern void load_idt(unsigned long *idt_ptr);
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr14(void);
extern void isr13(void);
extern void isr32(void);
extern void isr33(void);
extern void isr128(void);
extern void pusha(void);
extern void popa(void);

void timer_init();

extern unsigned char kbdus[128];

static char char_buf[1024];
//static char char_err_buf[1024];

static volatile int buf_idx = 0;
static volatile int buf_err_idx = 0;
static volatile int new_line = 0;
int SHIFT_ON = 0;
int CTRL_ON = 0;
int is_bg = 0;
/*struct gate_str{
  uint16_t offset_low;     // offset bits 0...15
  uint16_t selector;       // a code segment selector in GDT or LDT 16...31
  uint8_t zero;           // unused set to 0, set at 96...127
  uint8_t type_attr;       // type and attributes 40...43, Gate Type 0...3
  uint16_t offset_mid;  // offset bits 16..31, set at 48...63
  uint32_t offset_high;  // offset bits 16..31, set at 48...63
  uint32_t ist;           // unused set to 0, set at 96...127
  }  __attribute__((packed));
 */
struct gate_str
{
	uint16_t offset_low;
	uint16_t selector;
	unsigned ist : 3;
	unsigned reserved0 : 5;
	unsigned type_attr : 4;
	unsigned zero : 1;
	unsigned dpl : 2;
	unsigned p : 1;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved1;
} __attribute__((packed));

struct gate_str IDT[MAX_IDT];
int ticks = 0;
int sec = -1;
struct idtr_t
{
	uint16_t size;
	uint64_t addr;
} __attribute__((packed));

struct idtr_t idtr = {((sizeof(struct gate_str))*MAX_IDT), (uint64_t)&IDT};

void _x86_64_asm_lidt(struct idtr_t *idtr);

/*
0  0  0 - Supervisory process tried to read a non-present page entry
0  0  1 - Supervisory process tried to read a page and caused a protection fault
0  1  0 - Supervisory process tried to write to a non-present page entry
0  1  1 - Supervisory process tried to write a page and caused a protection fault
1  0  0 - User process tried to read a non-present page entry
1  0  1 - User process tried to read a page and caused a protection fault
1  1  0 - User process tried to write to a non-present page entry
1  1  1 - User process tried to write a page and caused a protection fault
*/
void page_fault_handler(uint64_t err_code, uint64_t err_rip) {
	uint64_t pf_addr, curr_cr3;
	__asm__ volatile ("movq %%cr2, %0":"=r"(pf_addr));
	__asm__ volatile ("movq %%cr3, %0":"=r"(curr_cr3));
//	kprintf("Page Fault encountered: %p %p Code: %d\n", pf_addr, err_rip, err_code);

	if (err_code == 4 || err_code == 6){
		struct page *pp = (struct page *)kmalloc(1);	
		init_map_virt_phys_addr(pf_addr, (uint64_t)PADDR(pp), 1, (uint64_t *)VADDR(curr_cr3), 1);
		kprintf("(Segmentation Fault)\n");
	}
	else if (err_code == 5 || err_code == 7){
		init_map_virt_phys_addr(pf_addr, (uint64_t)PADDR(pf_addr), 1, (uint64_t *)VADDR(curr_cr3), 1);
		kprintf("(Segmentation Fault)\n");
	}
	else if(err_code == 0 || err_code == 2){
		struct page *pp = (struct page *)kmalloc(1);	
		init_map_virt_phys_addr(pf_addr, (uint64_t)PADDR(pp), 1, (uint64_t *)VADDR(curr_cr3), 1);
		kprintf("(Segmentation Fault)\n");
	}
	else if(err_code == 1 || err_code == 3){
		init_map_virt_phys_addr(pf_addr, (uint64_t)PADDR(pf_addr), 1, (uint64_t *)VADDR(curr_cr3), 1);
		kprintf("(Segmentation Fault)\n");
	}
	else {
		struct page *pp = (struct page *)kmalloc(1);	
		init_map_virt_phys_addr(pf_addr, (uint64_t)PADDR(pp), 1, (uint64_t *)VADDR(curr_cr3), 1);
		kprintf("(Segmentation Fault)\n");
	}
//	__asm__ volatile ("movq %0, %%cr3"::"r"(ker_cr3));
//	kprintf("Pid: %d", get_running_task()->pid);
	//delete_curr_from_task_list();
//	kprintf("delete done");
	load_sbush();
//	kprintf("load");
	delete_curr_from_task_list();
	schedule(1);
}

void general_protection_fault_handler(uint64_t err_code, uint64_t err_rip) {
//	uint64_t pf_addr;
//	__asm__ volatile ("movq %%cr2, %0":"=r"(pf_addr));
	kprintf("GPF Encountered\n");
	__asm__ volatile ("hlt;");
	while(1);
}

void irq_timer_handler(void){
	outb(0x20, 0x20);
	outb(0xa0, 0x20);
	if (ticks % 100 == 0){
		int new_sec = ticks/100;
		if (new_sec != sec){
			sec = new_sec;
			kprintf_boott(">Time Since Boot: ", sec);
		}
		dec_sleep_count();
	}
	ticks++;
}

void syscall_handler(void) {
		uint64_t syscall_num = 0;
		uint64_t buf, third, fourth;
		//Save register values
		__asm volatile("movq %%rax, %0;"
						"movq %%rbx, %1;"
						"movq %%rcx, %2;"
						"movq %%rdx, %3;"
						: "=g"(syscall_num),"=g"(buf), "=g"(third), "=g"(fourth)
						:
						:"rax","rsi","rcx", "rdx"
					  );
		//	__asm__ volatile("movq %%rax, %0;"
		///					"movq %%rbx, %1;"
		//		"movq %%rcx, %2;"
		//			"movq %%rdx, %3;"
		//			: "=g"(syscall_num),"=g"(buf), "=g"(third), "=g"(fourth)
		//			:
		//			:"rbx", "rcx", "rdx");

		if (syscall_num == 1){ // WRITE
				int written = 0;
				char sub[str_len((char*)third)];
				if (fourth >= str_len((char*)third)){
						str_cpy(sub, (char*)third);
						written = str_len((char*)third);
				}
				else{
						str_substr((char*)third, 0, fourth, sub);
						written = str_len((char*)sub);
				}
				if (buf == 1){
						kprintf("%s", sub);
				}
				else{
						kprintf("STDERR: %s", sub);
				}
				__asm__ volatile (
								"movq %0, %%rax;"
								:
								:"a" ((uint64_t)written)
								:"cc", "memory"
								); 
		}
		else if (syscall_num == 2){ //READ
				__asm__("sti");
				kprintf(PS1);
				char *buf_cpy = (char *)third;
				int line = 0, idx = 0;
				while(line == 0 && idx < fourth){
						//poll
						line = new_line;
						idx = buf_idx;
				}
				memcpy(buf_cpy, char_buf, str_len(char_buf));
				memset(char_buf, 0, 1024);
				buf_idx = 0;
				new_line = 0;
				__asm__ volatile (
								"movq %0, %%rax;"
								:
								:"a" ((uint64_t)str_len(char_buf))
								:"cc", "memory"
								); 
				//		__asm__("cli");
				//				__asm__ volatile("movq %0, %%rax;"::"r"(str_len(char_buf)));
		}
		else if (syscall_num == 4) { //fork
		sys_fork();
	}
	else if (syscall_num == 6){ //yield
		schedule(0);
	}
	else if (syscall_num == 7 || syscall_num == 8){ //exec
//		kprintf("execvp called for %s %s\n", buf, ((char*)third));
		char cmd[50];
		str_concat(PATH, (char*)buf, cmd);
		file* fd = open((char *)cmd);
		if (fd == NULL){
				kprintf("oxTerm error: Command %s not found\n", buf);
				sys_exit(1);	
		}
/*		if (fd == NULL){ // Executes Scripts
			file *fd_sc = (file*)open((char*)buf);
			if (fd_sc == NULL){
				kprintf("oxTerm error: Command %s not found\n", buf);
				sys_exit(1);
			}
			else{
					char shebang[50];
					if (readline(fd_sc, shebang, 50) > 0){
							if (str_cmp("!#bin/oxTerm", shebang) > 0){
									kprintf("shebang");
									task_struct *parsed[MAX_SCRIPT_CMDS];
									task_struct *parent = get_running_task();
									int idx = 0;
									while(readline(fd_sc, shebang, 50) > 0){
											char task[50] = {0};
											str_concat(PATH, shebang, task);
											file* cmd = (file*)open(task);
											if (cmd == NULL){
													kprintf("oxTerm error: Command %s not found\n", buf);
													sys_exit(1);
											}
											else{
													file *tfd = open(task);
													char *argv[1] = {""};
													parsed[idx++] = elf_parse(tfd->addr+512, (file*)tfd->addr, 0, argv);
													parsed[idx]->pid = parent->pid;
													parsed[idx]->ppid = parent->ppid;	
											}
											for(int i = 0; i < 50; i++){
												shebang[i] = 0;
											}
									}
									delete_curr_from_task_list();
									for(int i = 0; i < idx; i++){
											add_to_task_list(parsed[i]);
									}
									close(fd_sc);
							}
							else{
								kprintf("oxTerm err: Trying to execute invalid oxTerm script\n");
								sys_exit(1);
							}
					}
			}
		}
*/		else{
			task_struct *parent = get_running_task();
			PID--; // Since pcb_exec increases PID by one on assignment
			char argument[50];
			str_cpy(argument, (char*)third);
//			while(1);
			char *argv[1] = {(char*)argument};
			task_struct *pcb_exec;
//			char argument[10];
//			memcpy(argument, (char*)third, str_len((char*)third));
//			kprintf("\nargs:%s %s\n", argument, buf);
			if (str_len((char*)argument) > 0 && str_cmp((char*)argument, (char*)buf) < 0 ){	
					pcb_exec = elf_parse(fd->addr+512,(file *)fd->addr, 1, argv);
				}
			else
				pcb_exec = elf_parse(fd->addr+512,(file *)fd->addr, 0, argv);
			pcb_exec->pid = parent->pid;
			pcb_exec->ppid = parent->ppid;
//			kprintf("%d", parent->pid);
			delete_curr_from_task_list();
			add_to_task_list(pcb_exec);
//			kprintf("name %s %d", pcb_exec->tname, parent->pid);
			schedule(1);
		}
	} 
	else if (syscall_num == 10) { //exit
		//while(1);
		//kprintf("Exit called");
		sys_exit(buf);
	} 
	else if (syscall_num == 12){
		list_dir();
	}
	else if (syscall_num == 14){ 
			char cmd_args[2][FMT_LEN];

			str_split_delim((char*)buf, ' ', cmd_args);
//			kprintf("args:%s", cmd_args[0]);
			char *filename = (char *)cmd_args[0];
			file *fd = open(filename);
			if (fd != NULL){
					int bytes = 60;
					int bread = 2048;
					char readbuf[bytes];
					while(bread >= bytes){
							bread = read_file(fd, readbuf, bytes);
							kprintf("%s", readbuf);
					}
					close(fd);
			}
	}
	else if (syscall_num == 16){
		kprintf("%s\n", buf); //Echo
	}
	else if (syscall_num == 18){
		print_task_list(); //ps
	}
	else if (syscall_num == 20) { //waitpid
		pid_t pid = sys_waitpid(buf, is_bg);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)pid)
				:"cc", "memory"
				); 
		//kprintf("pid %d", pid);
		//is_bg = 0;
	} else if (syscall_num == 22) { //sleep
		char cmd_args[2][FMT_LEN];
		str_split_delim((char*)buf, ' ', cmd_args);
//		kprintf("args:%d", atoi(cmd_args[0]));
		if (str_contains((char *)buf, "&") != -1){
			get_running_task()->is_bg = 1;
			is_bg = 1;
		} else {
			is_bg = 0;
		}
		sys_sleep(atoi(cmd_args[0]));
	} else if (syscall_num == 24) { //kill
			char cmd_args[3][FMT_LEN];
			char out[3];
			str_split_delim((char*)buf, ' ', cmd_args);
			str_substr((char*)cmd_args[1], 1, str_len(cmd_args[1])-1, out);
		if (str_len(out) == 0)
			kprintf("oxTerm: Usage: kill -9 PID\n");
		else
			sys_kill(atoi(out));
	} else if (syscall_num == 26) { //getpid
                 uint32_t pid = get_running_task()->pid;
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)pid)
				:"cc", "memory"
				); 
         } else if (syscall_num == 28) { //getppid
                 pid_t ppid = get_running_task()->ppid;
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)ppid)
				:"cc", "memory"
				); 
         } else if (syscall_num == 30) { //gets
                 __asm__("sti");
                 kprintf(PS1);
                 char *buf_cpy = (char *)buf;
                 //while(1);
                 int line = 0;
                 while(line == 0){
                 //poll
                         line = new_line;
                         //idx = buf_idx;
                 }
                 memcpy(buf_cpy, char_buf, str_len(char_buf));
                 memset(char_buf, 0, 1024);
                 buf_idx = 0;
                 new_line = 0;
         } else if (syscall_num == 32) { //wait
	 	//char *status = (char *)buf;
		pid_t pid = sys_waitpid(0, 0);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)pid)
				:"cc", "memory"
				); 
		//kprintf("pid %d", pid);
	 } else if (syscall_num == 34) { //open
	 	file *fd = open((char *)buf);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)fd)
				:"cc", "memory"
				); 

	 } else if (syscall_num == 36) { //close
	 	int status = close((file *)buf);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)status)
				:"cc", "memory"
				); 

	 } else if (syscall_num == 38) { //read-file
	 	size_t size = read_file((file *)buf, (void *)third, (size_t)fourth);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)size)
				:"cc", "memory"
				); 
	 } else if (syscall_num == 40) { //opendir
		uint64_t ret = opendir((char *)buf);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)ret)
				:"cc", "memory"
				); 
	 	
	 } else if (syscall_num == 42) { //read_dir
	 	uint64_t ret = read_dir((uint64_t)buf);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)ret)
				:"cc", "memory"
				); 
	 } else if (syscall_num == 44) { //closedir
	 	uint64_t ret = closedir((uint64_t)buf);
		__asm__ volatile (
				"movq %0, %%rax;"
				:
				:"a" ((uint64_t)ret)
				:"cc", "memory"
				); 
	 } 
	 else if (syscall_num == 46) { //clear
	 	clear_screen();
	 }
	

}
void irq_kb_handler(void){
		outb(0x20, 0x20);
	outb(0xa0, 0x20);
	
	char status = inb(KB_STATUS);
	if (status & 0x01){
		//		outb(KB_STATUS, 0x20);
		unsigned int key = inb(KB_DATA);
//		kprintf("%p\n", key);
//		return;
		if (key < 0){
			kprintf("OOPS");
			return;
		}
		if (key == 0x2a){
			SHIFT_ON = 1;
			return;
		}
		else if (key == 0xaa){
			SHIFT_ON = 0;
			return;
		}
		if (key == 0x1d){
			CTRL_ON = 1;
			SHIFT_ON = 1; // should be ^C and not ^c
			kprintf("%c", '^');
			char_buf[buf_idx++] = '^';
			return;
		}
		else if (key == 0x9d){
			CTRL_ON = 0;
			SHIFT_ON = 0;
			return;
		}
		else if (key == 0x0e){
			if (buf_idx > 0){
				char_buf[buf_idx] = 0;
				X -= 2;
				kprintf(" ");
				X -= 2;
				buf_idx--;
			}
			move_csr();
			return;
		
		}
		if (key == 0x1c){
			new_line = 1;
			kprintf("\n");
			char_buf[buf_idx++] = '\n';
			return;
		}
		if (key < 0x52){ // Keys over SSH
//			if (CTRL_ON == 0)
//				kprintf("%c", ' ');
			if (SHIFT_ON == 1){
				switch (key){
					case 2:
						kprintf("%c", '!');
						char_buf[buf_idx++] = '!';
						break;
					case 3:
						kprintf("%c", '@');
						char_buf[buf_idx++] = '@';
						break;
					case 4:
						kprintf("%c", '#');
						char_buf[buf_idx++] = '#';
						break;
					case 5:
						kprintf("%c", '$');
						char_buf[buf_idx++] = '$';
						break;
					case 6:
						kprintf("%c", '%');
						char_buf[buf_idx++] = '%';
						break;
					case 7:
						kprintf("%c",  '^');
						char_buf[buf_idx++] = '^';
						break;
					case 8:
						kprintf("%c", '&');
						char_buf[buf_idx++] = '&';
						break;
					case 9:
						kprintf("%c", '*');
						char_buf[buf_idx++] = '*';
						break;
					case 10:
						kprintf("%c", '(');
						char_buf[buf_idx++] = '(';
						break;
					case 11:
						kprintf("%c", ')');
						char_buf[buf_idx++] = ')';
						break;
					case 12:
						kprintf("%c", '_');
						char_buf[buf_idx++] = '_';
						break;
					case 13:
						kprintf("%c", '+');
						char_buf[buf_idx++] = '+';
						break;
					case 26:
						kprintf("%c", '{');
						char_buf[buf_idx++] = '{';
						break;
					case 27:
						kprintf("%c", '}');
						char_buf[buf_idx++] = '}';
						break;
					case 39:
						kprintf("%c", ':');
						char_buf[buf_idx++] = ':';
						break;
					case 40:
						kprintf("%c", '"');
						char_buf[buf_idx++] = '"';
						break;
					case 41:
						kprintf("%c", '~');
						char_buf[buf_idx++] = '~';
						break;
					case 43:
						kprintf("%c", '|');
						char_buf[buf_idx++] = '|';
						break;
					case 51:
						kprintf("%c", '<');
						char_buf[buf_idx++] = '<';
						break;
					case 52:
						kprintf("%c", '>');
						char_buf[buf_idx++] = '>';
						break;
					case 53:
						kprintf("%c", '?');
						char_buf[buf_idx++] = '?';
						break;
					default:
						kprintf("%c", kbdus[key] - 32);	
						char_buf[buf_idx++] = kbdus[key] - 32;
						break;
				}
			}
			else {
				kprintf("%c", kbdus[key]);
				char_buf[buf_idx++] = kbdus[key];
			}
		}
	}
}


void pic_init(void){ // SETUP Master and Slave PICS
	outb(0x20, 0x11);
	outb(0xA0, 0x11); 
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0xff);
	outb(0xA1, 0xff);
}
void setup_gate(int32_t num, uint64_t handler_addr, unsigned dpl){
	IDT[num].offset_low = (handler_addr & 0xFFFF);
	IDT[num].offset_mid = ((handler_addr >> 16) & 0xFFFF);
	IDT[num].offset_high = ((handler_addr >> 32) & 0xFFFFFFFF);
	IDT[num].selector = 0x08;
	IDT[num].type_attr = 0x0e;
	IDT[num].zero = 0x0;
	IDT[num].ist = 0x0;
	IDT[num].reserved0 = 0;
	IDT[num].p = 1;
	IDT[num].dpl = dpl;

}
void idt_init(void)
{
	setup_gate(0, (uint64_t)isr0, 0);
	setup_gate(1, (uint64_t)isr1, 0);
	setup_gate(3, (uint64_t)isr3, 0);
	setup_gate(4, (uint64_t)isr4, 0);
	setup_gate(5, (uint64_t)isr5, 0);
	setup_gate(6, (uint64_t)isr6, 0);
	setup_gate(7, (uint64_t)isr7, 0);
	setup_gate(8, (uint64_t)isr8, 0);
	setup_gate(9, (uint64_t)isr9, 0);
	setup_gate(10, (uint64_t)isr10, 0);
	setup_gate(11, (uint64_t)isr11, 0);
	setup_gate(12, (uint64_t)isr12, 0);
	setup_gate(13, (uint64_t)isr13, 0);
	setup_gate(14, (uint64_t)isr14, 0);
	setup_gate(16, (uint64_t)isr16, 0);
	setup_gate(17, (uint64_t)isr17, 0);
	setup_gate(18, (uint64_t)isr18, 0);
	setup_gate(19, (uint64_t)isr19, 0);
	setup_gate(20, (uint64_t)isr20, 0);
	setup_gate(32, (uint64_t)isr32, 0);
	setup_gate(33, (uint64_t)isr33, 0);
	setup_gate(128, (uint64_t)isr128, 3);
//	setup_gate(128, (uint64_t)syscall_handler, 3);
	_x86_64_asm_lidt(&idtr);
}

void mask_init(void){
		outb(0x21 , 0xFC); //11111100

//		outb(0x3D4, 0x0A);  // Disable ugly cursors
//		outb(0x3D5, 0x20);
}

void kmain(void){

	pic_init();
	idt_init();
	mask_init();
	timer_init();
}
void timer_init(){
 
	uint32_t divisor = 1193180;
   outb(0x43, 0x36);

   uint8_t l = (uint8_t)(divisor & 0xFF);
   uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

   outb(0x40, l);
   outb(0x40, h);
}
