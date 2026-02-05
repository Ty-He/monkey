#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <system_error>

namespace obj {

using Type = std::size_t;

constexpr Type INTEGER = 1;
constexpr Type BOOLEAN = 2;
constexpr Type NIL = 3;
constexpr Type RETURN_VALUE = 4;
constexpr Type ERROR = 5;
constexpr Type FUNCTION = 6;

// template<typename T>
// concept Object = requires(T* t) {
//   {t->type()} -> std::same_as<Type>;
// };

struct object {
	virtual Type type() const = 0;
	virtual std::string inspect() const = 0;
	virtual ~object() = default;
};

struct object_deleter {
	void operator()(object* o) const
	{
		if (!o) return;

		if (o->type() == INTEGER || o->type() == RETURN_VALUE)
			delete o;
	}
};

using object_ptr = std::unique_ptr<object, object_deleter>;

struct integer: object {
	std::int64_t value_;

	integer() = default;
	integer(std::int64_t v): value_(v) {}
	Type type() const override
	{
		return INTEGER;
	}

	std::string inspect() const override
	{
		return std::to_string(value_);
	}
};

struct boolean: object {
	bool value_;

	boolean() = default;
	boolean(bool v): value_(v) {}
	Type type() const override
	{
		return BOOLEAN;
	}

	std::string inspect() const override
	{
		return value_ ? "true" : "false";
	}
	
	static boolean* make(bool v)
	{
		static boolean t{true}, f{false};
		return v ? &t : &f;
	}
};

struct nil: object {
	Type type() const override { return NIL; }
	std::string inspect() const override { return "null"; }

	static nil* make()
	{
		static nil n{};
		return &n;
	}
};

#define M_NIL object_ptr{ obj::nil::make() }
#define M_TRUE object_ptr{ obj::boolean::make(true) }
#define M_FALSE object_ptr{ obj::boolean::make(false) }

struct return_value: object {
	object_ptr value_;

	return_value() = default;
	return_value(object_ptr&& v): value_(std::move(v)) {}
	Type type() const override { return RETURN_VALUE; }
	std::string inspect() const override { return value_->inspect(); }
};

enum class eval_errc {
	type_mismatch,
	unknown_operator,
	identifier_not_defined,
	not_a_function,
};

struct error: object, std::system_error {
	static auto const& eval_category()
	{
		static const struct: std::error_category {
			virtual std::string message(int val) const override
			{
				switch (static_cast<eval_errc>(val)) {
					case eval_errc::type_mismatch: return "type mismatch";
					case eval_errc::unknown_operator: return "unknown operator";
					case eval_errc::identifier_not_defined: return "identifier not defined";
					case eval_errc::not_a_function: return "not a function";
					default: return "other error";
				}
			}
			virtual const char* name() const noexcept override { return "eval"; }
		} instance;
		return instance;
	}

	// std::error_code ec;
	// std::system_error err;
	std::string info_;
	mutable std::string cache_;

	error() = default;
	error(eval_errc e) : std::system_error(static_cast<int>(e), eval_category()) {}
	// error(eval_errc e, const char* what_arg): std::system_error(static_cast<int>(e), eval_category(), what_arg) {}
	error(eval_errc e, std::string const& detail):
		std::system_error(static_cast<int>(e), eval_category()), info_(detail) {}
	Type type() const override { return ERROR; }
	std::string inspect() const override { return this->what(); }
	const char* what() const noexcept override
	{
		if (!info_.empty())
			cache_ = this->code().message() + ": " + info_;
		else
			cache_ = this->code().message();
		return cache_.c_str();
	}

	static object_ptr make(eval_errc e, std::string const& what_arg)
	{
		return object_ptr{new error(e, what_arg)};	
	}
};

template<class T>
concept FunctionLiteral = requires(T const& t) {
	typename T::Parameters;
	typename T::Body;
	{ t.parameters_ } -> std::same_as<const typename T::Parameters&>;
	{ t.body_ } -> std::same_as<const typename T::Body&>;
	{ t.to_string() } noexcept -> std::convertible_to<std::string>;
};

template<FunctionLiteral Func, typename Env>
struct function: object {
	// std::shared_ptr<Func> fn_;
	Func::Parameters parameters_;
	Func::Body body_;
	std::shared_ptr<Env> env_;
	std::string ins_;

	function() = default;
	function(Func const& f, std::shared_ptr<Env> env):
		parameters_(f.parameters_), body_(f.body_), env_(env), ins_(f.to_string())
	{}

	Type type() const override { return FUNCTION; }
	std::string inspect() const override
	{
		return ins_;
	}
};

}
