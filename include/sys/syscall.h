#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <sys/kprintf.h>
#include <sys/defs.h>

void syscall_init();

#define DECL_SYSCALL0(fn) int syscall_##fn();
#define DECL_SYSCALL1(fn,p1) int syscall_##fn(p1);

#define DEFN_SYSCALL0(fn, num) \
int syscall_##fn() \
{ \
	int a; \
	__asm__ volatile("int $0x80" : "=a" (a) : "0" (num)); \
	return a; \
}

#define DEFN_SYSCALL1(fn, num, P1) \
int syscall_##fn(P1 p1) \
{ \
	int a; \
	__asm__ volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((long)p1)); \
	return a; \
}


DECL_SYSCALL1(kprintf, const char*)
//DECL_SYSCALL1(write_hex, const char*)

//////////////////////////////////////////

#define _syscall0(type,name) \
		type name() \
	{ \
			long __res; \
			__asm__ volatile ("int $0x80" \
							: "=a" (__res) \
							: "a" (__NR_##name) \
							: "memory"); \
			return (type) __res; \
	}
#define _syscall1(type,name,type1,arg1) \
	type name(type1 arg1) \
	{ \
			long __res; \
			__asm__ volatile ("int $0x80" \
							: "=a" (__res) \
							: "a" (__NR_##name),"b" ((long)(arg1)) \
							: "memory"); \
			return (type) __res; \
	}
#define _syscall2(type,name,type1,arg1,type2,arg2) \
		type name(type1 arg1,type2 arg2) \
{ \
		long __res; \
		__asm__ volatile (  "int $0x80" \
						: "=a" (__res) \
						: "a" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)) \
						: "memory"); \
		return (type)__res; \
}
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
		type name(type1 arg1,type2 arg2,type3 arg3) \
{ \
		long __res; \
		__asm__ volatile (  "int $0x80" \
						: "=a" (__res) \
						: "a" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
						"d" ((long)(arg3)) \
						: "memory"); \
		return (type)__res; \
}
#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3, type4, arg4) \
		type name(type1 arg1,type2 arg2,type3 arg3, type4, arg4) \
{ \
		long __res; \
		__asm__ volatile (  "int $0x80" \
						: "=a" (__res) \
						: "a" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
						"d" ((long)(arg3)), "s" ((long)(arg4)) \
						: "memory"); \
		return (type)__res; \
}

#endif
