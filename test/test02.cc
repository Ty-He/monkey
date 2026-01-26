#include <iostream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

void printLexer(std::string const& input);

template <typename StmtType>
void test_stmt(std::vector<std::string> const& inputs)
{
	for (auto const& input: inputs) {
		printLexer(input);
		parser::Parser<lexer::Lexer> p(new lexer::Lexer(input));
		auto [program, errors] = p.parse();
		if (!errors.empty()) {
			std::cerr << "Fail: parse errors:\n";
			for (auto const& err: errors) std::cout << err << std::endl;
			exit(0);
		}

		if (auto sz = program->statements.size(); sz != 1) {
			std::cerr << "Fail: unexpected size: " << sz << '\n';
			std::cerr << program->to_string() << '\n';
			exit(0);
		}

		auto* stmt = dynamic_cast<StmtType*>(program->statements[0].get());
		if (!stmt) {
			std::cerr << "Fail: cannot dynamic_cast: " << '\n';
			exit(0);
		}

		std::cout << stmt->to_string() << std::endl;
	}
}

void test_return_stmt()
{
	std::vector<std::string> inputs {"return x + a * b;", "return;", "return fn(){};"};
	test_stmt<ast::ReturnStmt>(inputs);
	std::cout << "Pass all\n";
}

void test_let_stmt()
{
	std::vector<std::string> inputs {"let x = 5;", "let y = true;", "let foo = y;"};
	test_stmt<ast::LetStmt>(inputs);
	std::cout << "Pass all\n";
}

void printLexer(std::string const& input)
{
	// auto l = new lexer::Lexer(R"(
	// let x  5;
	// let  = 10;
	// let 23 2323;
	// )");
	auto l = new lexer::Lexer(input);

	for (auto t = l->next_token(); t.token_type != token::END_OF_FILE; t = l->next_token())
	{
		std::cout << t.token_type << ' ' << t.literal << std::endl;
	}
	std::cout << "==================\n";

	delete l;
}

void testOpeartorPrecedence()
{
	using TS = std::pair<std::string, std::string>;
	std::vector<TS> inputs{
		{ "1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)" },
		{"(5 + 5) * 2", "((5 + 5) * 2)"},
		{"2 / (5 + 5)", "(2 / (5 + 5))"},
		{"-(5 + 5)", "(-(5 + 5))"},
		{"!(true == true)", "(!(true == true))"},
	};

	for (const auto& [input, expected]: inputs) {
		parser::Parser<lexer::Lexer> p(new lexer::Lexer(input));
		auto [program, errors] = p.parse();
		if (!errors.empty()) {
			std::cerr << "Fail: parse errors:\n";
			for (auto const& err: errors) std::cout << err << std::endl;
			exit(0);
		}

		if (auto actual = program->to_string(); actual != expected) {
			std::cerr << "Fail: expected: " << expected << " got = " << actual << '\n';
			exit(0);
		}
	}
}

template <class ExpressionImpl>
void testOne(std::string const& input, bool isPrintToken = false)
{
	if (isPrintToken) {
		printLexer(input);
	}
	auto l = new lexer::Lexer(input);

	parser::Parser<lexer::Lexer> p(l);
	auto [program, errors] = p.parse();
	if (!errors.empty()) {
		std::cerr << "Fail: parse errors:\n";
		for (auto const& err: errors) std::cout << err << std::endl;
		return;
	}

	if (auto sz = program->statements.size(); sz != 1) {
		std::cerr << "Fail: unexpected size: " << sz << '\n';
		std::cerr << program->statements[1].get() << '\n';
		return;
	}

	auto* stmt = dynamic_cast<ast::ExpressionStmt*>(program->statements[0].get());
	if (!stmt) {
		std::cerr << "Fail: dynamic_cast: not a expression\n";
		return;
	} 

	auto* node = dynamic_cast<ExpressionImpl*>(stmt->expression_.get());
	if (!node) {
		std::cerr << "Fail: dynamic_cast: not target impl\n";
		return;
	}

	std::cout << node->to_string() << std::endl;
}

void testCallExpr()
{
	std::vector<std::string> inputs{
		"add(1, 2 * 3, 4 + 5);"
	};

	for (const auto& input: inputs) {
		testOne<ast::CallExpression>(input, true);
	}
}

// specialize for debugging
// template <>
// void testOne<ast::FunctionLiteral>(std::string const& input)
// {
//   auto l = new lexer::Lexer(input);
//
//   parser::Parser<lexer::Lexer> p(l);
//   auto [program, errors] = p.parse();
//   if (!errors.empty()) {
//     std::cerr << "Fail: parse errors:\n";
//     for (auto const& err: errors) std::cout << err << std::endl;
//     return;
//   }
//
//   if (auto sz = program->statements.size(); sz != 1) {
//     std::cerr << "Fail: unexpected size: " << sz << '\n';
//     std::cerr << program->statements[1].get() << '\n';
//     return;
//   }
//
//   auto* stmt = dynamic_cast<ast::ExpressionStmt*>(program->statements[0].get());
//   if (!stmt) {
//     std::cerr << "Fail: dynamic_cast: not a expression\n";
//     return;
//   }
//
//   auto* node = dynamic_cast<ast::FunctionLiteral*>(stmt->expression_.get());
//   if (!node) {
//     std::cerr << "Fail: dynamic_cast: not target impl\n";
//     return;
//   }
//
//   std::cout << node->to_string() << std::endl;
// }

void testFnLiteral()
{
	std::vector<std::string> inputs{
		"fn() {}",
		"fn(x) {}",
		"fn(x, y) {x + y;}",
	};
	for (auto const& input: inputs) {
		testOne<ast::FunctionLiteral>(input);
		std::cout << "after " << input << std::endl;
	}
}

void testIf()
{
	std::vector<std::string> inputs{
		"if (x < y) {x}", 
		"if (x < y) {x} else {y}", 
	};
	for (auto const& input: inputs) {
		testOne<ast::IfExpression>(input);
	}
	std::cout << "Pass all!" <<  std::endl;
}

void testBoolean()
{
	std::vector<std::string> inputs{
		"true;", "false;"
	};
	for (const auto& input: inputs) {
		testOne<ast::Boolean>(input);
		std::cout << "Pass test: " << input << std::endl;
	}

}

void testInfixMore()
{
	struct TestCase {
		std::string input;
		std::string expected;
	};
	std::vector<TestCase> tests {
		{
				"-a * b",
				"((-a) * b)",
		},
		{
				"!-a",
				"(!(-a))",
		},
		{
				"a + b + c",
				"((a + b) + c)",
		},
		{
				"a + b - c",
				"((a + b) - c)",
		},
		{
				"a * b * c",
				"((a * b) * c)",
		},
		{
				"a * b / c",
				"((a * b) / c)",
		},
		{
				"a + b / c",
				"(a + (b / c))",
		},
		{
				"a + b * c + d / e - f",
				"(((a + (b * c)) + (d / e)) - f)",
		},
		{
				"3 + 4; -5 * 5",
				"(3 + 4)((-5) * 5)",
		},
		{
				"5 > 4 == 3 < 4",
				"((5 > 4) == (3 < 4))",
		},
		{
				"5 < 4 != 3 > 4",
				"((5 < 4) != (3 > 4))",
		},
		{
				"3 + 4 * 5 == 3 * 1 + 4 * 5",
				"((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))",
		},
	};

	for (const auto& [input, expected]: tests) {
		parser::Parser<lexer::Lexer> p(new lexer::Lexer(input));
		auto [program, errors] = p.parse();
		if (!errors.empty()) {
			std::cerr << "Fail: parse errors:\n";
			for (auto const& err: errors) std::cout << err << std::endl;
			exit(0);
		}

		if (auto actual = program->to_string(); actual != expected) {
			std::cerr << "Fail: expected: " << expected << " got = " << actual << '\n';
			exit(0);
		}
	}
}

void testIfxExpr()
{
	std::vector<std::string> inputs{
		"5 + 5;", "5 - 5", "5 * 5 ", "5 / 5", "1 > 1", "2 < 2", "3 == 3", "4 != 4",
	};
	for (const auto& input: inputs) {
		testOne<ast::InfixExpression>(input);
		std::cout << "Pass test: " << input << std::endl;
	}

}

void testPreExpr()
{
	std::vector<std::string> inputs{"!5;", "-15"};
	for (const auto& input: inputs) {
		testOne<ast::PrefixExpression>(input);
	}
}

void testIntExpr()
{
	auto l = new lexer::Lexer("5;");

	parser::Parser<lexer::Lexer> p(l);
	auto [program, errors] = p.parse();
	if (!errors.empty()) {
		std::cerr << "Fail: parse errors:\n";
		for (auto const& err: errors) std::cout << err << std::endl;
		return;
	}

	if (program->statements.size() != 1) {
		std::cerr << "Fail: unexpected size\n";
		return;
	}

	auto* stmt = dynamic_cast<ast::ExpressionStmt*>(program->statements[0].get());
	if (!stmt) {
		std::cerr << "Fail: dynamic_cast\n";
		return;
	} 

	auto* ident = dynamic_cast<ast::IntegerLiteral*>(stmt->expression_.get());
	if (!ident) {
		std::cerr << "Fail: dynamic_cast\n";
		return;
	}

	std::cout << ident->to_string() << std::endl;
}

void testIdentExpr()
{
	auto l = new lexer::Lexer("foobar");

	parser::Parser<lexer::Lexer> p(l);
	auto [program, errors] = p.parse();
	if (!errors.empty()) {
		std::cerr << "Fail: parse errors:\n";
		for (auto const& err: errors) std::cout << err << std::endl;
		return;
	}

	if (program->statements.size() != 1) {
		std::cerr << "Fail: unexpected size\n";
		return;
	}

	auto* stmt = dynamic_cast<ast::ExpressionStmt*>(program->statements[0].get());
	if (!stmt) {
		std::cerr << "Fail: dynamic_cast\n";
		return;
	} 

	auto* ident = dynamic_cast<ast::Identifier*>(stmt->expression_.get());
	if (!ident) {
		std::cerr << "Fail: dynamic_cast\n";
		return;
	}

	std::cout << ident->to_string() << std::endl;
}

void testLexer()
{
	// auto l = new lexer::Lexer(R"(
	// let x  5;
	// let  = 10;
	// let 23 2323;
	// )");
	auto l = new lexer::Lexer(R"(
	3 == 3;
	4 != 4;
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

void testToString()
{
	ast::LetStmt ls;
	ls.token_ = token::Token{token::LET, "let"};

	auto i = new ast::Identifier{};
	i->token_ = token::Token{token::IDENT, "myVar"};
	i->value_ = "myVar";
	ls.name_.reset(i);

	auto v = new ast::Identifier{};
	v->token_ = token::Token{token::IDENT, "anotherVar"};
	v->value_ = "anotherVar";
	ls.value_.reset(v);

	std::cout << ls.to_string() << std::endl;
}


int main() 
{
	// testLexer();
	// std::cout << "testLexer pass.\n";
	// testLetStmt();
	// std::cout << "testLetStat pass.\n";
	// testReturnStmt();
	// std::cout << "testReturnStat pass.\n";
	
	// testToString();

	// testIdentExpr();
	// std::cout << "==== testIdentExpr pass.\n\n";
	//
	// testIntExpr();
	// std::cout << "==== testIntExpr pass.\n\n";

	// testLexer();
	// // testPreExpr();
	// testIfxExpr();

	// testInfixMore();
	// std::cout << "==== testInfixMore pass.\n";


	// testBoolean();
	// testOpeartorPrecedence();
	// std::cout << "testOpeartorPrecedence pass\n";

	// testIf();
	
	// testFnLiteral();

	// testCallExpr();

	// test_let_stmt();
	test_return_stmt();

	// char ch = 0;
	// std::cout << isdigit(static_cast<unsigned char>(0)) << std::endl;
	return 0;
}
