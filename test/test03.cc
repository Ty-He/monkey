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
	if (!b) {
		std::cout << "nullptr\n";
		return;
	}
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

void testIfElse()
{
	std::vector inputs {
		"if (true) {10}",
		"if (false) {10}",
		"if (1) {10}",
		"if (1 < 2) {10}",
		"if (1 > 2) {10}",
		"if (1 < 2) {10} else {20}",
		"if (1 > 2) {10} else {20}",
		"foobar",
	};
	for (auto input: inputs)
		testAnObejct<obj::integer>(input);
}

void testReturn()
{
	std::vector inputs {
		"return 10;",
		"return 10; 9;",
		"return 2 * 5; 9;",
		"9; return 2 * 5; 9",
		"if (10 > 1) { if(10 > 1) { return 10; } return 1;}"
	};
	for (auto input: inputs)
		testAnObejct<obj::integer>(input);
}

void testError()
{
	std::vector inputs {
		"5 + true;",
		"5 + true; 5;",
		"-true",
		"true + false",
		"2; true + false; 2",
		"if (10 > 1) { true + false; }",
	};
	for (auto input: inputs)
		testAnObejct<obj::error>(input);
}

void testLet()
{
	std::vector inputs {
		"let a = 5; a;",
		"let a = 5 * 5; a",
		"let a = 5; let b = a; b;",
		"let let a = 5; let b = a; let c = a + b + 5; c;",
	};
	for (auto input: inputs)
		testAnObejct<obj::integer>(input);
}

void testFn()
{
	// std::string input{"fn(x) {x + 2;}"};
	// testAnObejct<typename evaluator::eval_handler::func>(input);
	std::vector inputs {
		"let identity = fn(x) {x;}; identity(5);",
		"let identity = fn(x) { return x;}; identity(5);",
		"let double = fn(x) { return x * 2;}; double(5);",
		"let add = fn(x, y) { x + y;}; add(5, 5);",
		"let add = fn(x, y) { x + y;}; add(5 + 5, add(5, 5));",
	};
	for (auto input: inputs)
		testAnObejct<obj::integer>(input);
}


int main()
{
	try {
		// testAnObejct<obj::integer>("5");
		// testBoolean();
		// testBangOp();
		// testMinuxPrefix();
		// testIntInfix();
		// testIfElse();
		// testReturn();
		// testError();
		// testLet();
		testFn();
	} catch (std::exception& e) {
		std::cerr << e.what() << '\n';
	}
	return 0;
}
