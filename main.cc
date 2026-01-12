#include <iostream>
#include <unistd.h>

#include "repl.hpp"


int main()
{
	std::printf("Hello %s! This is the Monkey programming language!\n", getlogin());
	std::printf("Feel free to type in commands\n");
	repl::start(std::cin, std::cout);
	return 0;
}
