/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright (c) 2024 Stefan Kerkmann, Pengutronix */

#ifndef __ASM_SOPHGO_RISCV_HEAD_H
#define __ASM_SOPHGO_RISCV_HEAD_H

#include <linux/kernel.h>
#include <asm/image.h>

// TODO: taken from fsbl/build/cvi_board_memmap.h
// offset 2.0MiB
#define CONFIG_SYS_TEXT_BASE 0x80200000

#define ____barebox_sophgo_riscv_header(load_offset, version, magic1, magic2)    	\
	__asm__ __volatile__ (                                              			\
		/* Header start */															\
		"j 1f\n"                    	/* code0 */                     			\
		".balign 4\n"                          										\
		".ascii \"BL33\"\n"  			/* Magic */									\
		".ascii \"CKSM\"\n"  			/* Checksum */								\
		".ascii \"SIZE\"\n"  			/* Size */ 									\
		".dword " #load_offset "\n"		/* Run address */							\
		".ascii \"RSV1\"\n" 			/* Reserved1 */								\
		".ascii \"RSV2\"\n" 			/* Reserved2 */								\
		".balign 4\n" 																\
		/* Header end */															\
		"j 1f\n" 						/* code0 */									\
		".balign 4\n"																\
		".dword " #load_offset "\n"    	/* Image load offset from RAM start */		\
		".dword _barebox_image_size\n" 	/* Effective Image size */             		\
		".dword 0\n"                   	/* Kernel flags */                     		\
		".word " #version "\n"         	/* version */                          		\
		".word 0\n"                    	/* reserved */                         		\
		".dword 0\n"                   	/* reserved */                         		\
		".asciz \"" magic1 "\"\n"      	/* magic 1 */                          		\
		".balign 8\n"                                                         		\
		".ascii \"" magic2 "\"\n"      	/* magic 2 */                          		\
		".word 0\n"                    	/* reserved (PE-COFF offset) */        		\
		"1:\n"                                                                		\
		"li fp, 0\n"                                                          		\
	)

#define __barebox_sophgo_riscv_header(load_offset, version, magic1, magic2) \
        ____barebox_sophgo_riscv_header(load_offset, version, magic1, magic2)

#define __barebox_riscv_head() \
	__barebox_sophgo_riscv_header(CONFIG_SYS_TEXT_BASE, 0x0, "barebox", "RSCV")

#define ENTRY_FUNCTION_SOPHGO(name, arg0, arg1, arg2) \
	ENTRY_FUNCTION(name, arg0, arg1, arg2)

#endif /* __ASM_SOPHGO_RISCV_HEAD_H */
