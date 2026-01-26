#pragma once

#include <concepts>
#include <memory>
#include <unordered_map>
#include <functional>
#include <optional>
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
		prefix_parse_fns_.emplace(token::TRUE, [this] {
				return this->parse_boolean();
				});
		prefix_parse_fns_.emplace(token::FALSE, [this] {
				return this->parse_boolean();
				});
		prefix_parse_fns_.emplace(token::LPAREN, [this] {
				return this->parse_grouped_expr();
				});
		prefix_parse_fns_.emplace(token::IF, [this] {
				return this->parse_if_expr();
				});
		prefix_parse_fns_.emplace(token::FUNCTION, [this] {
				return this->parse_fn_literal();
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

		infix_parse_fns_[token::LPAREN] = [this](ast::Expression* fn) {
				return this->parse_call_expr(fn);
				};

	} // Parse()

	// ast::Program* parse() 
	std::pair<std::unique_ptr<ast::Program>, std::vector<std::string>>
	parse()
	{
		static auto pred = [](token::Token const& token) -> bool
		{
			return token.token_type == token::END_OF_FILE;
		};
		
		return { std::unique_ptr<ast::Program>{ new ast::Program(get_stmts(pred)) }, errors_ };
	}

private:

	using Pred = std::function<bool(token::Token const&)>;

	// if pred() then exit parse
	std::vector<ast::StmtPtr> get_stmts(Pred&& pred)
	{
		std::vector<ast::StmtPtr> statements;
		while (!pred(cur_token_))
		{
			auto stmt = parse_stmt();
			if (stmt) 
				statements.emplace_back(stmt);

			// skip delimiter, such as ';', '}' ...
			next_token();
		}
		return statements;
	}


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
		// cur_token_ '=', to parse_expr for value, next_token()
		next_token();

		// cur_token_is expr
		stmt->value_.reset(parse_expr(Precedence::lowest));
		
		if (peek_token_is(token::SEMICOLON))
			next_token();

		return stmt;
	}
	
	ast::ReturnStmt* parse_return_stmt()
	{
		auto stmt = new ast::ReturnStmt{};
		stmt->token_ = cur_token_;
		
		next_token();

		// return;
		if (cur_token_is(token::SEMICOLON)) {
			return stmt;
		}

		stmt->return_value_.reset(parse_expr(Precedence::lowest));

		if (peek_token_is(token::SEMICOLON))
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
		// operator_, such as +, -, *, callback is parse_infix_expr
		{token::EQ, Precedence::equals},
		{token::NEQ, Precedence::equals},
		{token::LT, Precedence::less_greater},
		{token::GT, Precedence::less_greater},
		{token::PLUS, Precedence::sum},
		{token::MINUS, Precedence::sum},
		{token::SLASH, Precedence::product},
		{token::ASTERISK, Precedence::product},
		// '(' for call function, callback is parse_call_expr
		{token::LPAREN, Precedence::call},
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
		// prefix: IDENT, INT, BANG, MINUS, Boolean
		auto* expr =  prefix_parse_fns_.contains(cur_token_.token_type) ?
			prefix_parse_fns_[cur_token_.token_type]() :
			(no_prefix_fn_error(cur_token_.token_type), nullptr);

		// in prefix parse, don't call next_token()

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


	ast::Expression* parse_boolean()
	{
		return new ast::Boolean{cur_token_, cur_token_is(token::TRUE)};
	}

	ast::Expression* parse_grouped_expr()
	{
		next_token();

		auto* expr = parse_expr(Precedence::lowest);

		if (!expect_peek(token::RPAREN)) {
			delete expr;
			return nullptr;
		}

		return expr;
	}

	ast::Expression* parse_if_expr()
	{
		auto* expr = new ast::IfExpression();
		expr->token_ = cur_token_;

		// expect '('
		if (!expect_peek(token::LPAREN)) {
			delete expr;
			return nullptr;
		}
		// cur_token_ is (, skip to the next
		next_token();

		expr->cond_.reset(parse_expr(Precedence::lowest));

		if (!expect_peek(token::RPAREN)) {
			delete expr;
			return nullptr;
		}

		if (!expect_peek(token::LBRACE)) {
			delete expr;
			return nullptr;
		}

		expr->consequence_.reset(parse_block_stmt());
		// cur_token_ is '}'

		if (!peek_token_is(token::ELSE))
			return expr;

		next_token();
		// cur_token_ is 'ELSE'

		if (!expect_peek(token::LBRACE)) {
			delete expr;
			return nullptr;
		}

		// cur_token_ is '{'
		expr->alternative_.reset(parse_block_stmt());
		// cur_token_ is '}'
		// ~~this delimiter is skipped by this->parse():next_token()

		return expr;
	}

	ast::BlockStmt* parse_block_stmt()
	{
		auto* block = new ast::BlockStmt;
		block->token_ = cur_token_;

		// cur_token_ is '{', to next
		next_token();

		static auto pred = [](token::Token const& token)
		{
			return token.token_type == token::RBRACE ||
				token.token_type == token::END_OF_FILE;
		};
		block->statements_ = get_stmts(pred);

		return block;
	}

	ast::Expression* parse_fn_literal() {
		auto fnToken = cur_token_;

		if (!expect_peek(token::LPAREN)) {
			return nullptr;
		}
		// cur_token_ is '('

		auto ops = parse_fn_param();
		if (!ops) {
			return nullptr;
		}

		// cur_token_ is ')'
		if (!expect_peek(token::LBRACE)) {
			return nullptr;
		}
		// cur_token_ is '{'
		auto* body = parse_block_stmt();

		return new ast::FunctionLiteral{fnToken, move(*ops), body};
		// cur_token_ is '}'
	}

	using _param = ast::FunctionLiteral::Parameters;
	std::optional<_param> parse_fn_param()
	{
		_param ps;

		// void parmeter, not error
		if (peek_token_is(token::RPAREN)) {
			// skip '('
			next_token();
			return ps;
		}

		next_token(); // skip '('
		auto* p = new ast::Identifier{cur_token_, cur_token_.literal};
		ps.emplace_back(p);

		while (peek_token_is(token::COMMA)) {
			next_token(); // skip current parameter
			next_token(); // skip ','
			auto* p = new ast::Identifier{cur_token_, cur_token_.literal};
			ps.emplace_back(p);
		}

		if (!expect_peek(token::RPAREN)) {
			// error
			return std::nullopt;
		}
		return ps;
	}

	ast::Expression* parse_call_expr(ast::Expression* fn)
	{
		// cur_token_is '(', fn is function
		auto callToken = cur_token_;
		auto args = parse_list<ast::CallExpression::ArgType>([this] {
				return this->parse_expr(Precedence::lowest);
				});
		if (!args)
			return nullptr;
		return new ast::CallExpression{callToken, fn, std::move(args.value())};
	}

	// parse parameter list or args list
	template<typename T>
	requires std::same_as<T, ast::FunctionLiteral::ParamType> 
	|| std::same_as<T, ast::CallExpression::ArgType>
	std::optional<std::vector<std::unique_ptr<T>>> parse_list(std::function<T*()> get_nx)
	{
		std::vector<std::unique_ptr<T>> ps;

		// void parmeter, not error
		if (peek_token_is(token::RPAREN)) {
			// skip '('
			next_token();
			return ps;
		}

		next_token(); // skip '('
		auto* p = get_nx();
		ps.emplace_back(p);

		while (peek_token_is(token::COMMA)) {
			next_token(); // skip current parameter
			next_token(); // skip ','
			auto* p = get_nx();
			ps.emplace_back(p);
		}

		if (!expect_peek(token::RPAREN)) {
			// error
			return std::nullopt;
		}
		return ps;
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
