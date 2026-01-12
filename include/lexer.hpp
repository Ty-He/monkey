#pragma once
#include <string>
#include <cctype>
#include "token.hpp"

namespace lexer {

class Lexer {
public:
	Lexer() = default;

	Lexer(std::string const& _input): input(_input) {
		position = 0;
		read_position = 0;
		// read first char
		read();
	}

	token::Token NextToken() {
		token::Token t;

		skip_whitespace();
		
		switch (ch) {
			case '=': 
				if (peek() == '=') 
					t = token::Token{token::EQ, "=="};
				else t = make_token(token::ASSIGN, ch);
				break;
			case ';': 
				t = make_token(token::SEMICOLON, ch);
				break;
			case '(': 
				t = make_token(token::LPAREN, ch);
				break;
			case ')': 
				t = make_token(token::RPAREN, ch);
				break;
			case ',': 
				t = make_token(token::COMMA, ch);
				break;
			case '+': 
				t = make_token(token::PLUS, ch);
				break;
			case '-':
				t = make_token(token::MINUS, ch);
				break;
			case '!':
				if (peek() == '=')
					t = token::Token{token::NEQ, "!="};
				else t = make_token(token::BANG, ch);
				break;
			case '/':
				t = make_token(token::SLASH, ch);
				break;
			case '*':
				t = make_token(token::ASTERISK, ch);
				break;
			case '<':
				t = make_token(token::LT, ch);
				break;
			case '>':
				t = make_token(token::GT, ch);
				break;
			case '{': 
				t = make_token(token::LBARACE, ch);
				break;
			case '}': 
				t = make_token(token::RBARACE, ch);
				break;
			case 0:
				t = make_token(token::END_OF_FILE, ch);
				break;
			default:
				if (is_letter(ch)) {
					// NOTE return from there, avoid read() repeatedly
					auto literal = read_ident_or_num(is_letter);
					return token::Token{token::lookup_ident(literal), literal};
				} else if (isdigit(static_cast<unsigned char>(ch))) {
					return token::Token{token::INT, read_ident_or_num([](char ch) -> bool {
							return isdigit(static_cast<unsigned char>(ch));
							})};
				}
				else {
					t = make_token(token::ILLEGAL, ch);
				}
		}

		read();
		return t;
	}


private:
	void read() noexcept {
		if (read_position == input.length()) ch = 0;
		else ch = input[read_position];

		position = read_position;
		++read_position;
	}

	// construct token by a single char
	token::Token make_token(token::TokenType const& tt, char c) const noexcept 
	{
		std::string s(1, c);
		return token::Token{tt, s};
	}

	// is a valid letter for indetifier or not
	static bool is_letter(char ch) noexcept {
		return (ch >= 'A' && ch <= 'Z') 
			|| (ch >= 'a' && ch <= 'z')
			|| ch == '_';
	}

	std::string read_ident_or_num(bool (*check)(char)) {
		int beg = position;
		do {
			read();
		} while (check(ch));
		return input.substr(beg, position - beg);
	}

	void skip_whitespace() {
		while (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
			read();
	}

	char peek() const noexcept {
		if (read_position == input.length()) return 0;
		return input[read_position];
	}

	std::string input;
	// current position of input
	int position;
	int read_position;
	// TODO more type, using template and concept
	char ch;
};


}
