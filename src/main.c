#include <stdio.h>
#include "server.h"

int main(int argc, char **argv)
{
	printf("hello eldr\n");

	server_start(5555);
}
