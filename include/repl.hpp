#pragma once
#include <iostream>
#include "lexer.hpp"


struct repl {
	static void start(std::istream& in, std::ostream& out) {
		for (;;) {
			std::string input;
			out << ">> ";
			// in >> input; BUG: by not only '\n'
			std::getline(in, input);
			lexer::Lexer l{input};
			for (auto t = l.NextToken();
					t.token_type != token::END_OF_FILE;
					t = l.NextToken()) {
				out << t.token_type << "\t\t" << t.literal << '\n';
			}
		}
	}
};
