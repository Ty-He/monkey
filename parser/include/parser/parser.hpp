#pragma once

#include <concepts>
#include <memory>
#include <unordered_map>
#include <functional>
// #include <iostream>
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

		prefix_parse_fns_.emplace(token::IDENT, [this] {
				return this->parse_identifier();
				});
		prefix_parse_fns_.emplace(token::INT, [this] {
				return this->parse_integer_literal();
				});
		// 
		// prefix_parse_fns_.emplace(token::SEMICOLON, [this] {
		//     std::cout << "Parse SEMICOLON\n";
		//     return nullptr;
		//     });
		prefix_parse_fns_.emplace(token::BANG, [this] {
				return this->parse_prefix_expr();
				});
		prefix_parse_fns_.emplace(token::MINUS, [this] {
				return this->parse_prefix_expr();
				});

		static auto parse_infix_expr = [this](ast::Expression* left) {
			// cur_token_ is infix operator_
			auto expr = new ast::InfixExpression{cur_token_, cur_token_.literal, left};

			auto precedence = t2p(cur_token_.token_type);
			next_token();
			expr->right_.reset(parse_expr(precedence));

			return expr;
		};

		for (const auto&[infixOp, _]: t2p_maps)
			infix_parse_fns_.emplace(infixOp, parse_infix_expr);

	} // Parse()

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
			// std::cout << cur_token_.token_type << ' ' << cur_token_.literal << "-->"
			//   << peek_token_.token_type << ' ' << peek_token_.literal << std::endl;
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

		return parse_expr_stmt();
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

	enum class Precedence {
		lowest,
		equals, // ==
		less_greater, // > <
		sum, // +
		product, // *
		prefix, // -x, !x
		call, // fn()
	};

	static inline std::unordered_map<token::TokenType, Precedence> t2p_maps{
		{token::EQ, Precedence::equals},
		{token::NEQ, Precedence::equals},
		{token::LT, Precedence::less_greater},
		{token::GT, Precedence::less_greater},
		{token::PLUS, Precedence::sum},
		{token::MINUS, Precedence::sum},
		{token::SLASH, Precedence::product},
		{token::ASTERISK, Precedence::product},
	};

	static Precedence t2p(token::TokenType t) 
	{
		return t2p_maps.contains(t) ? t2p_maps[t] : Precedence::lowest;
	}

	ast::ExpressionStmt* parse_expr_stmt()
	{
		auto* stmt = new ast::ExpressionStmt{};
		stmt->token_ = cur_token_;
		stmt->expression_ = ast::ExpressionPtr{parse_expr(Precedence::lowest)};
	
		if (peek_token_is(token::SEMICOLON)) next_token();

		return stmt;
	}

	void no_prefix_fn_error(token::TokenType expected)
	{
		std::string err(64, 0);
		std::sprintf(err.data(), "no prefix parse function for [%s] found", expected.c_str());
		errors_.push_back(err);
	}

	ast::Expression* parse_expr(Precedence precedence)
	{
		// prefix: IDENT, INT, BANG, MINUS
		auto* expr =  prefix_parse_fns_.contains(cur_token_.token_type) ?
			prefix_parse_fns_[cur_token_.token_type]() :
			(no_prefix_fn_error(cur_token_.token_type), nullptr);

		// infix
		while (!peek_token_is(token::SEMICOLON) && precedence < t2p(peek_token_.token_type)) {
			if  (!infix_parse_fns_.contains(peek_token_.token_type))
				return expr;
			next_token();
			expr = infix_parse_fns_[cur_token_.token_type](expr);
		}

		return expr;
	}

	ast::Expression* parse_identifier()
	{
		return new ast::Identifier{
			token::Token{this->cur_token_},
			this->cur_token_.literal};
	}

	ast::Expression* parse_integer_literal()
	{
		std::int64_t val {};
		try {
			val = std::stoll(cur_token_.literal);
		} catch (std::exception& e) {
			errors_.emplace_back(e.what());
			return nullptr;
		}
		return new ast::IntegerLiteral(cur_token_, val);
	}

	ast::Expression* parse_prefix_expr()
	{
		auto expr = new ast::PrefixExpression{cur_token_, cur_token_.literal}; 

		// skip current prefix operator_
		next_token();

		// clangd connot find this
		// expr->right_ = parse_expr(Precedence::prefix);
		expr->right_.reset(parse_expr(Precedence::prefix));
		return expr;
	}

	using LexerPtr = std::unique_ptr<Lexer>;
	LexerPtr lx_;
	token::Token cur_token_;
	token::Token peek_token_;
	std::vector<std::string> errors_;

	using prefixParseFn = std::function<ast::Expression*()>; 
	using infixParseFn = std::function<ast::Expression*(ast::Expression*)>;
	std::unordered_map<token::TokenType, prefixParseFn> prefix_parse_fns_;
	std::unordered_map<token::TokenType, infixParseFn> infix_parse_fns_;
};

}
