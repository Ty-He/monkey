#pragma once
#include <iostream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "eval/eval.hpp"


struct repl {
	static void start(std::istream& in, std::ostream& out) {
		auto env = std::make_shared<obj::environment>();
		for (;;) {
			std::string input;
			out << ">> ";
			// in >> input; BUG: by not only '\n'
			std::getline(in, input);
			parser::Parser<lexer::Lexer> p(new lexer::Lexer{input});
			auto [program, errors] = p.parse();
			if (!errors.empty()) {
				for (auto const& err: errors) out << err << '\n';
				continue;
			}

			// out << "Program:" << program->to_string() << '\n';
			auto evaluated = evaluator::eval<evaluator::eval_handler>(program.get(), env);
			// out << "end of eval\n";
			if (evaluated) {
				out << evaluated->inspect() << '\n';
			}
		}
	}
};
