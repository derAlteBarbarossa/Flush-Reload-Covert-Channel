#include "util.h"

int main(int argc, char** argv)
{
	int threshold;
	char string[100] = "Hello";
	char* channel = (char*) malloc(sizeof(char));

	threshold = find_threshold(string);
	printf("Threshold: %d\n", threshold);
	bool bit = true;

	// demo(string, channel, threshold);
	demo_child_parent(string, channel, threshold);
}