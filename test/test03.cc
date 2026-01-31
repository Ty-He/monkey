#include <iostream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "eval/eval.hpp"
#include <stdexcept>

template<typename ObjectImpl>
void testAnObejct(std::string const& input)
{
	std::cout << "\ntest: " << input << '\n';
	parser::Parser<lexer::Lexer> p(new lexer::Lexer(input));
	auto [program, errors] = p.parse();
	auto b = evaluator::eval(program.get());
	std::cout << "type: " <<b->type() << ", inspect: " << b->inspect() << '\n';
	auto i = dynamic_cast<ObjectImpl*>(b.get());
	if (!i) throw std::runtime_error{"fail: dynamic_cast ObjectImpl"};
	std::cout << "pass!\n";
}

void testBoolean()
{
	testAnObejct<obj::boolean>("true");
	testAnObejct<obj::boolean>("false");
}

void testBangOp()
{
	std::vector inputs {
		"!true", 
		"!false",
		"!5",
		"!!true", 
		"!!false",
		"!!5",
	};
	for (auto input: inputs)
		testAnObejct<obj::boolean>(input);
}

void testMinuxPrefix()
{
	std::vector inputs {
		"5", 
		"10",
		"-5",
		"-10", 
	};
	for (auto input: inputs)
		testAnObejct<obj::integer>(input);
}

void testIntInfix()
{
	std::vector inputs {
		// "5",
		// "10",
		// "-5",
		// "-10",

		// "5 + 5 + 5 + 5 - 10", // 10
		// "2 * 2 * 2 * 2 * 2", // 32
		// "-100 + 200 - 100", //0

		"1 < 2",
		"1 > 2",
		"1 < 1",
		"1 > 1",
		"1 == 1",
		"1 != 1",
		"1 != 2",

		"true == true",
		"false == false",
		"true == false",
		"true != false",
		"false != true",
		"(1 < 2) == true",
		"(1 < 2) == false",
		"(1 > 2) == true",
		"(1 > 2) == false",
	};
	for (auto input: inputs)
		testAnObejct<obj::boolean>(input);
}


int main()
{
	try {
		// testAnObejct<obj::integer>("5");
		// testBoolean();
		// testBangOp();
		// testMinuxPrefix();
		testIntInfix();
	} catch (std::exception& e) {
		std::cerr << e.what() << '\n';
	}
	return 0;
}
