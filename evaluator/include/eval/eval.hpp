#pragma once

#include "ast/ast.hpp"
#include "object/object.hpp"

namespace evaluator {
inline namespace v_0_1 {

template<typename EvalHandler, typename Base>
	obj::object* dispatch(Base* b)
	{
		return nullptr;
	}

template<typename EvalHandler, typename Base, typename Impl, typename... RestImpls>
	obj::object* dispatch(Base* b)
	{
		if (auto* impl = dynamic_cast<Impl*>(b); impl) {
			return EvalHandler::eval(impl);
		}
		return dispatch<EvalHandler, Base, RestImpls...>(b);
	}

template<class EvalHandler>
static auto stmt_dispatch = dispatch<EvalHandler, ast::Statement,
						ast::ExpressionStmt>;

template<class EvalHandler>
static auto expr_dispatch = dispatch<EvalHandler, ast::Expression,
						ast::IntegerLiteral,
						ast::Boolean,
						ast::PrefixExpression,
						ast::InfixExpression>;

struct eval_handler {
	static obj::object* eval(ast::ExpressionStmt* e)
	{
		return expr_dispatch<eval_handler>(e->expression_.get());
	}

	static obj::object* eval(ast::IntegerLiteral* i)
	{
		return new obj::integer{i->value_};
	}

	static obj::object* eval(ast::Boolean* b)
	{
		return obj::boolean::make(b->value_);
	}

	static obj::object* eval(ast::PrefixExpression* pe)
	{
		auto* right = expr_dispatch<eval_handler>(pe->right_.get());

		if (pe->operator_ == "!") {
			if (right == obj::boolean::make(false) || right == obj::nil::make())
				return obj::boolean::make(true);
			else return obj::boolean::make(false);
		}

		if (pe->operator_ == "-") {
			if (right->type() != obj::INTEGER)
				return obj::nil::make();
			return new obj::integer(-static_cast<obj::integer*>(right)->value_);
		}

		// unsupported operator
		return obj::nil::make();
	}

	static obj::object* eval_int_infix(std::string const& op, obj::integer* li, obj::integer* ri)
	{
		if (op == "+")
			return new obj::integer{li->value_ + ri->value_};
		if (op == "-")
			return new obj::integer{li->value_ - ri->value_};
		if (op == "*")
			return new obj::integer{li->value_ * ri->value_};
		if (op == "/")
			return new obj::integer{li->value_ / ri->value_};
		if (op == "<")
			return obj::boolean::make(li->value_ < ri->value_);
		if (op == ">")
			return obj::boolean::make(li->value_ > ri->value_);
		if (op == "==")
			return obj::boolean::make(li->value_ == ri->value_);
		if (op == "!=")
			return obj::boolean::make(li->value_ != ri->value_);

		// unsupported operator
		return obj::M_NIL;
	}
	static obj::object* eval(ast::InfixExpression* ie) {
		auto* left = expr_dispatch<eval_handler>(ie->left_.get());
		auto* right = expr_dispatch<eval_handler>(ie->right_.get());

		if (left->type() == obj::INTEGER && right->type() == obj::INTEGER) {
			auto* li = static_cast<obj::integer*>(left);
			auto* ri = static_cast<obj::integer*>(right);
			return eval_int_infix(ie->operator_, li, ri);
		}

		// bool compare
		if (ie->operator_ == "==")
			return obj::boolean::make(left == right);
		if (ie->operator_ == "!=")
			return obj::boolean::make(left != right);

		return obj::M_NIL;
	}
};

template <typename EvalHandler = eval_handler>
obj::object_ptr eval(ast::Program* program)
{
	obj::object* res = nullptr;
	for (auto const& stmt: program->statements) {
		res = stmt_dispatch<EvalHandler>(stmt.get());
	}
	return obj::object_ptr(res);
}

} // v_0_1
}
