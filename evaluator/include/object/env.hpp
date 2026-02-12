#pragma once
#include <unordered_map>

#include "object.hpp"
#include "ast/ast.hpp"

namespace obj {

template<class Base, class Derived = void>
struct _clone {
	static Derived* on_success(const Derived* p) { return new Derived(*p); }
	static Base* on_fail(const Base* p) { return const_cast<Base*>(p); }
};


template<template<typename B, typename D = void> class Clone, class Base>
	Base* clone(const Base* ptr)
	{
		return Clone<Base>::on_fail(ptr);
	}

template<template<typename B, typename D = void> class Clone,
	class Base, class Derived, class ...More>
	Base* clone(const Base* ptr)
	{
		auto* p = dynamic_cast<const Derived*>(ptr);
		return p ? Clone<Base, Derived>::on_success(p) :
			clone<Clone, Base, More...>(ptr);
	}

class environment {
public:
	environment() = default;
	environment(std::shared_ptr<environment> upper): store_(), upper_(upper) {}

	std::pair<object_ptr, bool> get(std::string const& key)
	{
		if (store_.contains(key))
			return std::pair{object_ptr{clone_obj(store_[key].get())}, true};
		if (upper_)
			return upper_->get(key);
		return {nullptr, false};
	}

	void set(std::string key, const object* val)
	{
		// deep clone?
		store_.emplace(key, clone_obj(val));
	}

	static inline auto clone_obj = clone<_clone, obj::object,
							obj::integer,
							obj::function<ast::FunctionLiteral, environment>,
							obj::string,
							obj::array,
							obj::hashtable>;
private:
	std::unordered_map<std::string, object_ptr> store_;
	std::shared_ptr<environment> upper_;
};
}
