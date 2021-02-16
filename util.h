#ifndef _UTIL_H
#define _UTIL_H

#include "ECC.h"

#include <stdio.h>
#include <sys/mman.h>	// For mmap
#include <string.h>		// For memset
#include <stdlib.h>
#include <x86intrin.h>	// For _mm_clflush
#include <stdbool.h>
#include <stdint.h>		// For uint
#include <sys/wait.h> 	// For wait()
#include <unistd.h>		// For fork
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define L3_CACHE_SIZE		4096*1024
#define ROUNDS				10000
#define BITS				8
#define TRANSMIT_ROUNDS		1000
#define INTERVAL			0xFF000
#define TIME_MASK			0xFFFFF
#define JITTER				0x100
#define TERMINATE_ROUNDS	10
#define PREAMBLE			0b10101011
#define PREAMBLE_ROUNDS		10
#define SHARED_FILE			"/bin/ls"
#define MAX_BUFFER_LEN	1024

int compare_function(const void* a, const void*b);
char* allocate_set();

int time_access(char* addr);
int find_threshold(char* addr);

extern void send_bit(char* channel, bool bit);
extern bool receive_bit(char* channel, int threshold);
extern void send_preamble(char* channel);

extern bool* char_to_binary(char character, bool* binary_value);
extern char binary_to_char(bool* binary_value);
uint64_t synchronise();


void dump_bitset(bool* bitset, int size);
void demo(char* string, char* channel, int threshold);
void demo_child_parent(char* string, char* channel, int threshold);
#endif