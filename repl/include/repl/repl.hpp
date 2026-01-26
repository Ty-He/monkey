#pragma once
#include <iostream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"


struct repl {
	static void start(std::istream& in, std::ostream& out) {
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

			out << program->to_string() << '\n';
		}
	}
};
