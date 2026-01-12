#include <iostream>

#include "token.hpp"
#include "lexer.hpp"
using namespace lexer;
using namespace token;

void println(lexer::Lexer& l) {
	for (auto t = l.NextToken(); t.token_type != END_OF_FILE; t = l.NextToken()) {
		if (t.token_type == token::ILLEGAL) {
			std::cerr << "ILLEGAL: " << t.literal << std::endl;  
			std::terminate();
		}
		std::cout << t.token_type << '\t' << t.literal << '\n';
	}
}

void test01() 
{
	Lexer l("=+(){},;");
	println(l);
}

void test02()
{
	Lexer l(R"(let five = 5;
	let ten = 10;

	let add = fn(x, y) {
		x + y;
	};

	let result = add(five, ten);

	!-/5;
	3 < 9 > 3;
	if (4 < 8) {
		return true;
	} else {
		return false;
	}

	10 == 10;
	10 != 9;
	)");
	println(l);
}

int main()
{
	test02();
	std::cout << "test01\n";
	return 0;
}
