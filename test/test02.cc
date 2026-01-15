#include <iostream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


void testLexer()
{
	auto l = new lexer::Lexer(R"(
	let x  5;
	let  = 10;
	let 23 2323;
	)");

	for (auto t = l->next_token(); t.token_type != token::END_OF_FILE; t = l->next_token())
	{
		std::cout << t.token_type << ' ' << t.literal << std::endl;
	}

	delete l;
}

void testLetStmt()
{
	// BUG: double free
	// lexer::Lexer l(R"(
	// let x = 5;
	// let y = 10;
	// let foobar = 232323;
	// )");

	auto l = new lexer::Lexer(R"(
	let x  5;
	let  = 10;
	let 23 2323;
	)");
	parser::Parser<lexer::Lexer> p(l);
	auto [program, errors] = p.parse();
	if (errors.size() > 0) {
		std::cerr << "Fail: parse errors:\n";
		for (auto const& err: errors) std::cout << err << std::endl;
		return;
	}
	for (auto& stmt: program->statements) {
		if (auto let = dynamic_cast<ast::LetStmt*>(stmt.get()); let) {
			std::cout << let->token_literal() << ' '
				<< let->name_->token_literal() << '\n';
		} else {
			std::cerr << "Fail: dynamic_cast error\n";
		}
	}
}

void testReturnStmt()
{
	auto l = new lexer::Lexer(R"(
	return x;
	return 123;
	)");
	parser::Parser<lexer::Lexer> p(l);
	auto [program, errors] = p.parse();
	if (errors.size() > 0) {
		std::cerr << "Fail: parse errors:\n";
		for (auto const& err: errors) std::cout << err << std::endl;
		return;
	}
	for (auto& stmt: program->statements) {
		if (auto let = dynamic_cast<ast::ReturnStmt*>(stmt.get()); let) {
			std::cout << let->token_literal() << ' '
				<< let->token_literal() << '\n';
		} else {
			std::cerr << "Fail: dynamic_cast error\n";
		}
	}
}

int main() 
{
	testLexer();
	std::cout << "testLexer pass.\n";
	testLetStmt();
	std::cout << "testLetStat pass.\n";
	testReturnStmt();
	std::cout << "testReturnStat pass.\n";
	// char ch = 0;
	// std::cout << isdigit(static_cast<unsigned char>(0)) << std::endl;
	return 0;
}
