#pragma once

// #include <iostream>

#include "ast/ast.hpp"
#include "object/object.hpp"
#include "object/env.hpp"

namespace evaluator {
inline namespace v_0_1 {

template<typename Eval, typename Base>
	Eval::Ret dispatch(Base* b, typename Eval::EnvPtr)
	{
		throw std::runtime_error{"dispatch<> is failed"};
	}

template<typename Eval, typename Base, typename Impl, typename... RestImpls>
	Eval::Ret dispatch(Base* b, typename Eval::EnvPtr env)
	{
		if (auto* impl = dynamic_cast<Impl*>(b); impl) {
			return Eval::eval(impl, env);
		}
		return dispatch<Eval, Base, RestImpls...>(b, env);
	}

template<class EvalHandler>
static auto stmt_dispatch = dispatch<EvalHandler, ast::Statement,
						ast::ExpressionStmt,
						ast::BlockStmt,
						ast::ReturnStmt,
						ast::LetStmt>;

template<class EvalHandler>
static auto expr_dispatch = dispatch<EvalHandler, ast::Expression,
						ast::IntegerLiteral,
						ast::Boolean,
						ast::PrefixExpression,
						ast::InfixExpression,
						ast::IfExpression,
						ast::Identifier,
						ast::FunctionLiteral,
						ast::CallExpression,
						ast::StringLiteral,
						ast::ArrayLiteral,
						ast::IndexExpression,
						ast::HashTableLiteral>;

// uptr is a unique_ptr returned by eval or dispatch
#define CheckEvalErr(uptr) if (uptr->type() == obj::ERROR) return uptr
// impl struct for evaluator
class eval_handler {
public:
	using Ret = obj::object_ptr;
	using Env = obj::environment;
	using EnvPtr = std::shared_ptr<Env>;

	using e = obj::eval_errc;
	using err = obj::error;
	using func = obj::function<ast::FunctionLiteral, obj::environment>;
	// eval for program is an interface for caller
	static obj::object_ptr eval(const ast::Program* program, EnvPtr env)
	{
		obj::object_ptr res {};
		for (auto const& stmt: program->statements) {
			res = stmt_dispatch<eval_handler>(stmt.get(), env);
			if (res->type() == obj::ERROR) return res;
			if (auto* rv = dynamic_cast<obj::return_value*>(res.get()); rv) {
				// return value for program
				return obj::object_ptr{std::move(rv->value_)};
			}
			// res.release();
		}
		return res;
	}

	static obj::object_ptr eval(const ast::ExpressionStmt* e, EnvPtr env)
	{
		return expr_dispatch<eval_handler>(e->expression_.get(), env);
	}

	static obj::object_ptr eval(const ast::IntegerLiteral* i, EnvPtr)
	{
		return obj::object_ptr{ new obj::integer{i->value_} };
	}

	static obj::object_ptr eval(const ast::Boolean* b, EnvPtr)
	{
		return obj::object_ptr{ obj::boolean::make(b->value_) };
	}

	static obj::object_ptr eval(const ast::PrefixExpression* pe, EnvPtr env)
	{
		auto _right = expr_dispatch<eval_handler>(pe->right_.get(), env);
		CheckEvalErr(_right);
		if (_right->type() == obj::ERROR) return _right;
		auto* right = _right.get();

		if (pe->operator_ == "!") {
			if (right == obj::boolean::make(false) || right == obj::nil::make())
				return obj::M_TRUE;
			else return obj::M_FALSE;
		}

		if (pe->operator_ == "-") {
			if (right->type() != obj::INTEGER)
				return err::make(e::unknown_operator, join({pe->operator_, right->inspect()}));
			return obj::object_ptr{ new obj::integer(-static_cast<obj::integer*>(right)->value_) };
		}

		// unsupported operator
		// return obj::M_NIL;
		return err::make(obj::eval_errc::unknown_operator, pe->operator_ + right->inspect());
	}

	static obj::object_ptr eval(const ast::InfixExpression* ie, EnvPtr env) {
		auto _left = expr_dispatch<eval_handler>(ie->left_.get(), env);
		CheckEvalErr(_left);
		auto _right = expr_dispatch<eval_handler>(ie->right_.get(), env);
		CheckEvalErr(_right);
		auto *left = _left.get(), *right = _right.get();

		if (left->type() == obj::INTEGER && right->type() == obj::INTEGER) {
			auto* li = static_cast<obj::integer*>(left);
			auto* ri = static_cast<obj::integer*>(right);
			return obj::object_ptr{eval_int_infix(ie->operator_, li, ri)};
		}

		if (left->type() == obj::STRING && right->type() == obj::STRING) {
			auto* ls = static_cast<obj::string*>(left);
			auto* rs = static_cast<obj::string*>(right);
			return obj::object_ptr{eval_string_expr(ie->operator_, ls, rs)};
		}

		if (left->type() != right->type())
			return err::make(obj::eval_errc::type_mismatch, join({left->inspect(), ie->operator_, right->inspect()}));

		// bool compare
		if (ie->operator_ == "==")
			return obj::object_ptr{obj::boolean::make(left == right)};
		if (ie->operator_ == "!=")
			return obj::object_ptr{obj::boolean::make(left != right)};

		return err::make(obj::eval_errc::unknown_operator, join({left->inspect(), ie->operator_, right->inspect()}));
	}

	static obj::object_ptr eval(const ast::BlockStmt* bs, EnvPtr env)
	{
		return eval_stmt(bs->statements_, env);
	}

	static obj::object_ptr eval(const ast::IfExpression* i, EnvPtr env)
	{
		auto cond = expr_dispatch<eval_handler>(i->cond_.get(), env);
		CheckEvalErr(cond);
		if (is_truthy(cond.get())) {
			return eval(i->consequence_.get(), env);
		} else if (i->alternative_) {
			return eval(i->alternative_.get(), env);
		}

		return nullptr;
	}

	static obj::object_ptr eval(const ast::ReturnStmt* r, EnvPtr env)
	{
		auto v = expr_dispatch<eval_handler>(r->return_value_.get(), env);
		CheckEvalErr(v);
		return obj::object_ptr{
			new obj::return_value(std::move(v))
		};
	}

	static obj::object_ptr eval(const ast::LetStmt* l, EnvPtr env)
	{
		auto v = expr_dispatch<eval_handler>(l->value_.get(), env);
		CheckEvalErr(v);
		env->set(l->name_->value_, v.get());
		return v;
	}

	static obj::object_ptr eval(const ast::Identifier* i, EnvPtr env)
	{
		auto [val, ok] = env->get(i->value_);
		// return ok ? std::move(val) : err::make(e::identifier_not_defined, i->value_);
		if (ok) return std::move(val);
		return builtins.contains(i->value_) ? 
			obj::object_ptr{ &builtins[i->value_] } :
			err::make(e::identifier_not_defined, i->value_);
	}

	static obj::object_ptr eval(const ast::FunctionLiteral* f, EnvPtr env)
	{
		auto* fn = new func{*f, env};
		return obj::object_ptr{ fn };
	}

	static obj::object_ptr eval(const ast::CallExpression* c, EnvPtr env)
	{
		// fn
		auto fn = expr_dispatch<eval_handler>(c->fn_.get(), env);
		CheckEvalErr(fn);

		std::vector<obj::object_ptr> args;
		// args
		for (auto& arg: c->args_) {
			auto a = expr_dispatch<eval_handler>(arg.get(), env); 
			CheckEvalErr(a);
			args.push_back(std::move(a));
		}

		func* f = dynamic_cast<func*>(fn.get());
		if (f) return call(f, std::move(args));
		obj::builtin* b = dynamic_cast<obj::builtin*>(fn.get());
		if (b) return call(b, std::move(args));
		return err::make(e::not_a_function, fn->inspect());
	}

	static obj::object_ptr eval(const ast::StringLiteral* i, EnvPtr)
	{
		return obj::object_ptr{ new obj::string{i->value_} };
	}

	static obj::object_ptr eval(const ast::ArrayLiteral* a, EnvPtr env)
	{
		obj::array::Elements elems;
		for (auto const& elem: a->elements_) {
			auto e = expr_dispatch<eval_handler>(elem.get(), env);
			CheckEvalErr(e);
			elems.push_back(std::move(e));
		}
		return obj::object_ptr{ new obj::array(std::move(elems), a->to_string())};
	}

	// index of array shoule return elem ref
	// but now all var is readonly, so return the deep copy
	static obj::object_ptr eval(const ast::IndexExpression* i, EnvPtr env)
	{
		auto set = expr_dispatch<eval_handler>(i->left_.get(), env);
		CheckEvalErr(set);
		auto index = expr_dispatch<eval_handler>(i->index_.get(), env);
		CheckEvalErr(index);

		if (set->type() == obj::ARRAY && index->type() == obj::INTEGER)
			return eval_index_arr(set.get(), index.get());
		if (set->type() == obj::HASHTABLE) {
			return eval_index_ht(set.get(), index);
		}
		return err::make(e::type_mismatch, "cannot index");
	}

	static obj::object_ptr eval(const ast::HashTableLiteral* h, EnvPtr env)
	{
		obj::hashtable::HashTable ht;
		for (auto const& [ke, ve]: h->pairs_) {
			auto k = expr_dispatch<eval_handler>(ke.get(), env);
			CheckEvalErr(k);
			auto v = expr_dispatch<eval_handler>(ve.get(), env);
			CheckEvalErr(v);
			ht.emplace(move(k), move(v));
		}
		return obj::object_ptr{new obj::hashtable{move(ht)}};
	}
private:
	// used for cond in if
	static bool is_truthy(obj::object* cond)
	{
		return !(cond == obj::nil::make() || cond == obj::boolean::make(false));
	}

	static obj::object_ptr eval_stmt(std::vector<ast::StmtPtr> const& stmts, EnvPtr env)
	{
		obj::object_ptr res{};
		for (auto const& stmt: stmts) {
			res = stmt_dispatch<eval_handler>(stmt.get(), env);
			if (res->type() == obj::RETURN_VALUE || res->type() == obj::ERROR) {
				// return return_value object for upper block
				return res;
			}
		}
		return res;
	}

	// TODO use macro
	static std::string join(std::initializer_list<std::string> list, char cat = ' ')
	{
		std::ostringstream out;
		std::for_each(list.begin(), list.end() - 1, [&out, cat](std::string const& s) {
				out << s << cat;
				});
		out << *(list.end() - 1);
		return out.str();
	}

	static obj::object* eval_int_infix(std::string const& op, const obj::integer* li, const obj::integer* ri)
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
		// return obj::nil::make();
		return new err{e::unknown_operator, join({li->inspect(), op, ri->inspect()})};
	}

	static obj::object* eval_string_expr(std::string const& op, const obj::string* ls, const obj::string* rs)
	{
		if (op == "+")
			return new obj::string{ls->value_ + rs->value_};
		if (op == "==")
			return obj::boolean::make(ls->value_ == rs->value_);
		
		return new err{e::unknown_operator, join({ls->inspect(), op, rs->inspect()})};
	}
	// obj::environment env_;
	static std::unordered_map<std::string, obj::builtin> builtins;

	static obj::object_ptr call(func* f, std::vector<obj::object_ptr>&& args)
	{
		// Env extendEnv(f->env_);
		auto extendEnv = std::make_shared<Env>(f->env_);
		for (int i = 0; i < args.size(); ++i)
			extendEnv->set((*f->parameters_)[i]->value_, args[i].get());

		auto res = eval(f->body_.get(), extendEnv);
		CheckEvalErr(res);
		return res->type() == obj::RETURN_VALUE ?
			obj::object_ptr{ dynamic_cast<obj::return_value*>(res.get())->value_.release() }:
			std::move(res);
	}

	static obj::object_ptr call(obj::builtin* b, std::vector<obj::object_ptr>&& args)
	{
		return b->fn_(std::move(args));
	}

	static obj::object_ptr eval_index_arr(const obj::object* arr, const obj::object* index)
	{
		obj::array::Elements const& elems = *static_cast<const obj::array*>(arr)->elements_;
		std::int64_t idx = static_cast<const obj::integer*>(index)->value_;
		return idx >= 0 && idx < elems.size() ?
			obj::object_ptr{Env::clone_obj(elems[idx].get())} :
			err::make(e::array, "out of range");
	}

	static obj::object_ptr eval_index_ht(const obj::object* ht, const obj::object_ptr& key)
	{
		obj::hashtable::HashTable const& h = *static_cast<const obj::hashtable*>(ht)->ht_;
		if (!obj::hashtable::hashable(key->type())) {
			return err::make(e::hashtable, "key is not hashable " + key->inspect());
		}
		if (!h.contains(key)) {
			return obj::object_ptr{ obj::nil::make() };
		}
		// return obj::object_ptr{ obj::nil::make() };
		return obj::object_ptr{ Env::clone_obj(h.at(key).get()) };
	}
}; // struct eval_handler

// TODO move to source file
std::unordered_map<std::string, obj::builtin> eval_handler::builtins = obj::builtin::create_builtins();

template <typename EvalHandler = eval_handler>
obj::object_ptr eval(ast::Program* program, std::shared_ptr<obj::environment> env)
{
	return EvalHandler::eval(program, env);
}

template <typename EvalHandler = eval_handler>
obj::object_ptr eval(ast::Program* program)
{
	auto env = std::make_shared<obj::environment>();
	return EvalHandler::eval(program, env);
}

} // v_0_1
}
