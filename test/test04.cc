// for more support test
#include <iostream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "eval/eval.hpp"


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

template <typename StmtType>
void testStmt(std::vector<std::string> const& inputs)
{
	for (auto const& input: inputs) {
		printLexer(input);
		parser::Parser<lexer::Lexer> p(new lexer::Lexer(input));
		auto [program, errors] = p.parse();
		if (!errors.empty()) {
			std::cerr << "Fail: parse errors:\n";
			for (auto const& err: errors) std::cout << err << std::endl;
			exit(1);
		}

		if (auto sz = program->statements.size(); sz != 1) {
			std::cerr << "Fail: unexpected size: " << sz << '\n';
			std::cerr << program->to_string() << '\n';
			exit(1);
		}

		auto* stmt = dynamic_cast<StmtType*>(program->statements[0].get());
		if (!stmt) {
			std::cerr << "Fail: cannot dynamic_cast: " << '\n';
			exit(1);
		}

		std::cout << stmt->to_string() << std::endl;
	}
}

template <class ExpressionImpl>
void testExpr(std::string const& input, bool isPrintToken = false)
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

template<typename ObjectImpl>
void testEval(std::string const& input)
{
	std::cout << "\ntest: " << input << '\n';
	// testExpr<ast::CallExpression>(input, true);
	parser::Parser<lexer::Lexer> p(new lexer::Lexer(input));
	auto [program, errors] = p.parse();
	auto b = evaluator::eval(program.get());
	if (!b) {
		std::cout << "eval: nullptr\n";
		return;
	}
	std::cout << "after eval\n";
	std::cout << "type: " <<b->type() << ", inspect: " << b->inspect() << '\n';
	auto i = dynamic_cast<ObjectImpl*>(b.get());
	if (!i) throw std::runtime_error{"fail: dynamic_cast ObjectImpl: " + b->inspect()};
	std::cout << "pass!\n";
}

void testBuiltin()
{
	std::vector inputs {
		R"a(len("");)a",
		R"a(len("four");)a",
		R"a(len("hello world");)a",
		// R"a("len(1)")a",
		// R"a("len("one", "two")")a",
	};

	for (auto input: inputs)
		testEval<obj::integer>(input);
}

void testBuiltinError()
{
	std::vector inputs {
		R"a(len(1);)a",
		R"a(len("one", "two");)a",
	};

	for (auto input: inputs)
		testEval<obj::error>(input);
}

void testString()
{
	// printLexer(R"(
	// "foo bar"
	// )");
	// testExpr<ast::IntegerLiteral>("1234;");
	// testExpr<ast::StringLiteral>(R"a("";)a");
	// testEval<obj::string>(R"("hello world";)");
	// testEval<obj::string>(R"("hello" + " " + "world";)");
	// testEval<obj::boolean>(R"("hello" == "world";)");
	// testBuiltin();
	// testBuiltinError();
}

void testArray()
{
	// printLexer("[1, 2, 3];");
	testExpr<ast::ArrayLiteral>("[];");
	// testExpr<ast::ArrayLiteral>("[1, 2 * 2, 3 + 3, fn(x) {x+1;}];");
	// testExpr<ast::IndexExpression>("[1, 2, 3][2]");
	// testEval<obj::array>("[];");
	// testEval<obj::array>("[1, 2, 3];");
	// testEval<obj::array>("[1, 2 + 3 * 4, fn(x) {x * x;}];");
	testEval<obj::integer>("let arr = [1, 2 + 3 * 4, fn(x) {x * x;}]; arr[0];");
}

void testHashTable()
{
	// printLexer(R"({key: value, "str": "value", 1: 1, true: "true"})");
	// testExpr<ast::HashTableLiteral>(R"({})");
	// testExpr<ast::HashTableLiteral>(R"({key: value, "str": "value", 1: 1, true: "true"})");

	// testEval<obj::hashtable>(R"({})");
	testEval<evaluator::eval_handler::func>(R"(
	let key = "key";
	let value = fn(x) {return x + 10;};
	let ht = {key: value, "str": "value", 1: 1, true: "true"};
	ht[key];
	)");
	// testEval<obj::string>(R"(
	// let ht = {"str": "value", 1: 1, true: "true"};
	// ht["str"];
	// )");
}

int main()
{
	testHashTable();
	return 0;
}
