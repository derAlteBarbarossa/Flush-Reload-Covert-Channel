#include "util.h"

int compare_function(const void* a, const void*b)
{
	return (*(int*)a - *(int*)b);
}

int find_threshold(char* addr)
{
	volatile char* probing_address = addr;
	int times[ROUNDS] = {0};
	uint64_t start;
	int aux = 0;

	*probing_address;
	for (int i = 0; i < ROUNDS; i++)
	{
		start = __rdtscp(&aux);
		*probing_address;
		_mm_lfence();
		times[i] = __rdtscp(&aux) - start;
	}

	qsort(times, ROUNDS, sizeof(int), compare_function);

	
	return times[ROUNDS>>1] + 2*(times[ROUNDS>>1] - times[0]);
}

int time_access(char* addr)
{
	int time = 0;
	int aux = 0;
	uint64_t start;

	volatile char* probing_address = addr;

	start = __rdtscp(&aux);
	*probing_address;
	_mm_lfence();
	time = __rdtscp(&aux) - start;


	return time;
}

char* allocate_set()
{
	char* eviction_set = mmap(NULL, L3_CACHE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (eviction_set == MAP_FAILED)
	{
		printf("Mapping Failed\n");
		exit(-1);
	}

	memset((void*) eviction_set, 'A', L3_CACHE_SIZE);
	return eviction_set;
}

inline void send_bit(char* channel, bool bit)
{
	volatile char* covert_channel = channel;
	
	if(bit)
		_mm_clflush((char*)covert_channel);


	_mm_mfence();
}

inline bool receive_bit(char* channel, int threshold)
{
	volatile char* probing_address = channel;

	int time = 0;
	_mm_mfence();
	time = time_access(channel);

	if (time > threshold)
		return true;

	return false;
	
}

inline bool* char_to_binary(char character, bool* binary_value)
{
	binary_value[7] = (character & 0b10000000) ? true: false;
	binary_value[6] = (character & 0b01000000) ? true: false;
	binary_value[5] = (character & 0b00100000) ? true: false;
	binary_value[4] = (character & 0b00010000) ? true: false;
	binary_value[3] = (character & 0b00001000) ? true: false;
	binary_value[2] = (character & 0b00000100) ? true: false;
	binary_value[1] = (character & 0b00000010) ? true: false;
	binary_value[0] = (character & 0b00000001) ? true: false;

	return binary_value;
}

inline char binary_to_char(bool* binary_value)
{
	int result = 0;

	for (int i = 0; i < BITS; i++)
	{
		if(binary_value[i])
			result |= (1<<i);
	}
	return (char)result;
}

void demo(char* string, char* channel, int threshold)
{
	bool* send_binary_value; 
	send_binary_value = (bool*) malloc(sizeof(bool) * BITS);

	bool* coded_binary_value;
	coded_binary_value = (bool*) malloc(sizeof(bool) * CODE_SIZE);

	bool* recv_binary_value; 
	recv_binary_value = (bool*) malloc(sizeof(bool) * CODE_SIZE);

	bool* decoded_binary_value;
	decoded_binary_value = (bool*) malloc(sizeof(bool) * BITS);

	bool received_bit = true;
	int index = 0;
	char received_char;
	int count;

	while(*(string + index) != '\0')
	{
		send_binary_value = char_to_binary(*(string + index), send_binary_value);

		coded_binary_value = encode(send_binary_value);

		for (int i = 0; i < CODE_SIZE; i++)
		{
			count = 0;
			received_bit = true;
			for (int j = 0; j < TRANSMIT_ROUNDS; ++j)
			{

				send_bit(channel, *(coded_binary_value + i));
				if(!receive_bit(channel, threshold))
					count++;

				if(count >= 10)
					received_bit = false;
				
			}
			*(recv_binary_value + i) = received_bit;
		}
		decoded_binary_value = decode(recv_binary_value);

		received_char = binary_to_char(decoded_binary_value);
		printf("%c", received_char);
		index++;
	}
	printf("\n");
}

void dump_bitset(bool* bitset, int size)
{
	for (int i = size - 1; i >=0; i--)
		printf("%d", *(bitset + i));
	
	printf("\n");
}

uint64_t synchronise()
{
	int aux = 0;
	while(__rdtscp(&aux) & TIME_MASK > JITTER);
	return __rdtscp(&aux);
}

inline void send_preamble(char* channel)
{
	bool* send_binary_value; 
	send_binary_value = (bool*) malloc(sizeof(bool) * BITS);

	bool* coded_binary_value;
	coded_binary_value = (bool*) malloc(sizeof(bool) * CODE_SIZE);

	uint64_t start;
	int aux = 0;


	send_binary_value = char_to_binary((char)PREAMBLE, send_binary_value);
	coded_binary_value = encode(send_binary_value);
	// dump_bitset(coded_binary_value, CODE_SIZE);

	for (int i = CODE_SIZE - 1; i >= 0; i--)
	{
		start = synchronise();
		while(__rdtscp(&aux) - start < INTERVAL)
		{
			send_bit(channel, *(coded_binary_value + i));
		}
	}


	return;
}

void demo_child_parent(char* string, char* channel, int threshold)
{
	int inFile = open(SHARED_FILE, O_RDONLY);
	void *mapaddr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, inFile, 0);
	channel = (char*) mapaddr;

	pid_t child_pid;
	child_pid = fork();

	if(child_pid == 0)
	{
		int sending = 1;
		int index;
		int aux = 0;

		bool* send_binary_value; 
		send_binary_value = (bool*) malloc(sizeof(bool) * BITS);

		bool* coded_binary_value;
		coded_binary_value = (bool*) malloc(sizeof(bool) * CODE_SIZE);

		uint64_t time;

		
		
		send_preamble(channel);

		index = 0;
		while(*(string + index) != '\0')
		{
			send_binary_value = char_to_binary(*(string + index), send_binary_value);
			coded_binary_value = encode(send_binary_value);

			for (int i = 0; i < CODE_SIZE; i++)
			{
				time = synchronise();
				while((__rdtscp(&aux) - time) < INTERVAL)
					send_bit(channel, *(coded_binary_value + i));
			}
			index++;
		}


		exit(0);
	}

	else
	{
		uint64_t time, start;
		int aux = 0;
		int count = 0;
		int index = 0;
		int strikes = 0;
		bool receiving = true;

		bool bitReceived;
		bool received_bit;
		uint32_t bitSequence = 0;
		uint32_t sequenceMask = ((uint32_t) 1<<15) - 1;
		uint32_t expSequence = 0b10100101011001;

		bool* recv_binary_value; 
		recv_binary_value = (bool*) malloc(sizeof(bool) * CODE_SIZE);

		bool* decoded_binary_value;
		decoded_binary_value = (bool*) malloc(sizeof(bool) * BITS);

		fflush(stdout);
		while (receiving) 
		{
			start = synchronise();
			while((__rdtscp(&aux) - start) < INTERVAL)
				bitReceived = receive_bit(channel, threshold);

			bitSequence = ((uint32_t) bitSequence<<1) | bitReceived;

			if ((bitSequence & sequenceMask) == expSequence) 
			{
				printf("Preamble received\n");
				char received_char = (char) 27;

				while(received_char != '\0')
				{
					index = 0;
					strikes = 0;

					while(index != 14)
					{
						received_bit = false;
						count = 0;
						start = synchronise();

						while((__rdtscp(&aux) - start) < INTERVAL)
						{
							if(receive_bit(channel, threshold))
								count++;
							else
								count--;
						}
						if(count > 0)
						{
							strikes++;
							received_bit = true;
						}

						else
							strikes = 0;

						*(recv_binary_value + index) = received_bit;						
						if(index == 13 || strikes >= 14)
							break;

						index++;
					}
					decoded_binary_value = decode(recv_binary_value);
					received_char = binary_to_char(decoded_binary_value);

					if(strikes != 14)
					{
						printf("%c", received_char);
					}
					if(received_char == '\0')
						receiving = false;
					
				}
				
			}
		}
		printf("\n");
		wait(NULL);

	}
	return;
}

