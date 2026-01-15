#pragma once

#include <concepts>
#include <memory>
#include "../ast/ast.hpp"
#include "lexer/token.hpp"

namespace parser {

template <class T>
concept LEXER = requires {
	{ T().next_token() } -> std::same_as<token::Token>;
};

template <LEXER Lexer>
class Parser {
public:
	Parser(Lexer* lx): lx_(lx) 
	{
		next_token();
		next_token();
	}

	// ast::Program* parse() 
	std::pair<std::unique_ptr<ast::Program>, std::vector<std::string>>
	parse()
	{
		ast::Program* program = new ast::Program;
		
		int i = 0;
		while (cur_token_.token_type != token::END_OF_FILE) {
			auto stmt = parse_stmt();
			if (stmt) 
				program->statements.emplace_back(stmt);
			next_token();
		}
		return {std::unique_ptr<ast::Program>{ program }, errors_};
		// return program;
	}

private:
	void next_token()
	{
		cur_token_ = peek_token_;
		peek_token_ = lx_->next_token();
	}
	
	ast::Statement* parse_stmt()
	{
		if (cur_token_is(token::LET))
			return parse_let_stmt();
		if (cur_token_is(token::RETURN))
			return parse_return_stmt();

		return nullptr;
	}

	ast::LetStmt* parse_let_stmt()
	{
		auto stmt = new ast::LetStmt();
		stmt->token_ = cur_token_;

		if (!expect_peek(token::IDENT)) {
			delete stmt;
			return nullptr;
		}

		stmt->name_ = ast::IdentifierPtr{ new ast::Identifier(cur_token_, cur_token_.literal) };

		if (!expect_peek(token::ASSIGN)) {
			delete stmt;
			return nullptr;
		}

		// TODO skip expression
		
		while (!cur_token_is(token::SEMICOLON))
			next_token();

		return stmt;
	}
	
	ast::ReturnStmt* parse_return_stmt()
	{
		auto stmt = new ast::ReturnStmt;
		stmt->token_ = cur_token_;

		while (!cur_token_is(token::SEMICOLON))
			next_token();

		return stmt;
	}

	bool cur_token_is(token::TokenType t) const noexcept
	{
		return cur_token_.token_type == t;
	}

	bool peek_token_is(token::TokenType t) const noexcept
	{
		return peek_token_.token_type == t;
	}

	// if peek_token_ type is expected, then advance
	// else return false
	bool expect_peek(token::TokenType t)
	{
		if (peek_token_.token_type == t) {
			next_token();
			return true;
		}

		peek_error(t);
		return false;
	}

	// collect error when parsing,
	// due to unknown pragram, errors hold by parser.
	void peek_error(token::TokenType expected)
	{
		std::string err(64, 0);
		std::sprintf(err.data(), "expected next token to be %s, got %s instead",
				expected.c_str(), peek_token_.token_type.c_str());
		errors_.push_back(err);
	}


	using LexerPtr = std::unique_ptr<Lexer>;
	LexerPtr lx_;
	token::Token cur_token_;
	token::Token peek_token_;
	std::vector<std::string> errors_;
};

}
