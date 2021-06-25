/*

Experiment in C
Studying inline asm, absolute adddress / relative jmps and memory pages
Thanks https://github.com/rafaelrc7 for the help!

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

unsigned long long int r1;

unsigned char *cmd; // address of a function (to stored in main())
char * string = "Now, i jumped to a absolute address!\n";
char * jto_message = "Wow, i jumped to a offset!\n";

void jump_with_offset(){
    asm("mov $1, %rax\n"
	"mov $1, %rdi\n"
	"mov jto_message, %rsi\n"
	"mov $27, %rdx\n"
	"syscall\n"
	"movq cmd, %rax\n" // moving 64 bit address (cmd) to rax register
        "jmp *%rax"); 		 // jumping to absolute address of rax (*rax) (jmp with absolute address)
}

void prepare_jump(){
	__asm__ __volatile__("jmp jump_with_offset"); // jump to jump_with_offset (jmp with offset)
}

void jump_with_absolute_address(){
    asm("xor %%rax, %%rax\n" 
        "mov $1, %%rax \n" // Write syscall to rax
	"mov $1, %%rdi\n"	 // Stdout (write argument)
	"mov string, %%rsi\n" // char pointer (write argument)
	"mov $37, %%rdx\n"  // string lenght (write argument)
        "syscall\n"	// call write syscall

	"mov $60, %%rax\n"	// exit syscall
	"mov $0, %%rdi\n"		// return (exit argument)
	"syscall\n"					// call exit syscall
	: "=&bits" (r1));		// set 64 bits mode with an unsigned long long int (64 bits) variable
}

int main(void){
	size_t size = ((intptr_t)&main - (intptr_t)&jump_with_absolute_address); // Size of jump_with_absolute_address function
		// main() address if the end of jump_with_absolute_address()
		// jump_with_absolute_address() address is the start of jump_with_absolute_address()
		// main() - jump_with_absolute_address() = size of jump_with_absolute_address() function
	printf("Size of jump_with_absolute_address() function :: 0x%zx\n", size); 
	intptr_t pagesize = sysconf(_SC_PAGE_SIZE);
	printf("Page size 0x%zx\n", pagesize);
	cmd = (unsigned char *)aligned_alloc(pagesize, size);
	if (!cmd) {
		perror("malloc - cmd");
		exit(1);
	}


#define PAGE_START(P) ((intptr_t)(P) & ~(pagesize - 1))
#define PAGE_END(P) (((intptr_t)(P) + pagesize - 1) & ~(pagesize - 1))

		// make allocated page executable
	if (mprotect((void *)PAGE_START(cmd),vPAGE_END(cmd + size) - PAGE_START(cmd), PROT_READ | PROT_WRITE | PROT_EXEC) == -1){
		perror("mprotect");
		exit(1);
	}
	memcpy(cmd, (unsigned char *)&jump_with_absolute_address, size); // copy jump_with_absolute_address to cmd
	prepare_jump(); // jump with offset address
	return 0;
}
