#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <system_error>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>
// #include <functional>

namespace obj {

using Type = std::size_t;

constexpr Type INTEGER = 1;
constexpr Type BOOLEAN = 2;
constexpr Type NIL = 3;
constexpr Type RETURN_VALUE = 4;
constexpr Type ERROR = 5;
constexpr Type FUNCTION = 6;
constexpr Type STRING = 7;
constexpr Type BUILTIN = 8;
constexpr Type ARRAY = 9;
constexpr Type HASHTABLE = 10;

static std::string looktype(Type x)
{
	static std::vector<std::string> types {
		"unknown",
		"int",
		"bool",
		"nil",
		"ret",
		"error",
		"fn",
		"str",
		"builtin",
		"array",
		"hashtable",
	};
	return types[x];
}

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

		switch (o->type()) {
			case BOOLEAN: [[fallthrough]];
			case NIL: [[fallthrough]];
			case BUILTIN:
				return;
			default: break;
		}

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
	builtin,
	array,
	hashtable,
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
					case eval_errc::builtin: return "builtin";	
					case eval_errc::array: return "array";	
					case eval_errc::hashtable: return "hashable";	
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

struct string: object {
	std::string value_;

	string() = default;
	string(std::string_view sv): value_(sv) {}
	
	Type type() const override { return STRING; }
	std::string inspect() const override { return value_; }
};

struct array: object {
	using Elements= std::vector<object_ptr>;
	std::shared_ptr<Elements> elements_;
	// I am lazy.
	std::string ins_cache_;

	array() = default;
	array(Elements&& es, std::string_view ins = "array")
		:elements_(std::make_shared<Elements>(std::move(es))), ins_cache_(ins) {}

	Type type() const override { return ARRAY; }
	std::string inspect() const override { return ins_cache_; }
};

struct hashtable: object {
	static bool hashable(Type t) {
		return t == INTEGER || t == BOOLEAN || t == STRING;
	}
	struct hash {
		std::size_t operator()(const object_ptr& o) const noexcept
		{
			switch (o->type()) {
				case INTEGER:
					return std::hash<std::int64_t>{}(static_cast<const integer*>(o.get())->value_);
				case BOOLEAN:
					return std::hash<bool>{}(static_cast<const boolean*>(o.get())->value_);
				case STRING:
					return std::hash<std::string>{}(static_cast<const string*>(o.get())->value_);
				default: return -1;
			}
		}
	};
	struct hash_key_eq {
		template<class T>
			bool compare_value(object_ptr const& x, const object_ptr& y) const
			{
				return static_cast<const T*>(x.get())->value_ == 
				static_cast<const T*>(y.get())->value_;
			}
		bool operator()(const object_ptr& x, const object_ptr& y) const
		{
			if (x.get() == y.get()) return true;
			if (!x || !y) return false;
			// x and y not null
			if (x->type() != y->type()) return false;
			switch (x->type()) {
				case INTEGER:
					return compare_value<integer>(x, y);
				case BOOLEAN:
						// for BOOLEAN, the addr is equal
					return compare_value<boolean>(x, y);
				case STRING:
					return compare_value<string>(x, y);
				default: return false;
			}
		}
	};

	using HashTable = std::unordered_map<object_ptr, object_ptr, hash, hash_key_eq>;
	std::shared_ptr<HashTable> ht_;

	hashtable() = default;
	hashtable(HashTable&& ht): ht_(std::make_shared<HashTable>(move(ht))) {}

	Type type() const override { return HASHTABLE; }
	std::string inspect() const override
	{
		std::ostringstream out;
		out << "{";
		if (ht_) {
			for (auto const&[k, v]: *ht_) {
				out << "\n  " << k->inspect() << ": " << looktype(k->type()) << " -> "
					<< v->inspect() << " : " << looktype(v->type());
			}
			out << "\n";
		}
		out << "}";
		return out.str();
	}
};

struct builtin: object {
	using builtinFuncArg = std::vector<object_ptr>;
	using builtinFunc = object_ptr(*)(builtinFuncArg);
	builtinFunc fn_;
	
	builtin() = default;
	builtin(builtinFunc fn): fn_(fn) {}


	Type type() const override { return BUILTIN; }
	std::string inspect() const override { return ""; }

	static std::unordered_map<std::string, builtin> create_builtins()
	{
		std::unordered_map<std::string, builtin> builtins;
		builtins.emplace("len", len);
		builtins.emplace("append", append);
		builtins.emplace("println", println);
		return builtins;
	}

	static object_ptr len(builtinFuncArg args)
	{
		if (args.size() != 1)
			return error::make(eval_errc::builtin, "len: wrong arg size: " + std::to_string(args.size()));
		switch (args[0]->type()) {
			case STRING: return object_ptr{new integer(
											 static_cast<string*>(args[0].get())->value_.length()
											 )};
			case ARRAY: return object_ptr{new integer(
											static_cast<array*>(args[0].get())->elements_->size()
											)};
			default: return error::make(eval_errc::builtin, "len: not supported type " + std::to_string(args[0]->type()));
		}
	}

	static object_ptr append(builtinFuncArg args)
	{
		if (args.size() != 2)
			return error::make(eval_errc::builtin, "append: wrong arg size: " + std::to_string(args.size()));
		if (auto* arr = dynamic_cast<array*>(args[0].get()); arr) {
			arr->elements_->push_back(std::move(args[1]));
			return std::move(args[0]);
		}
		return error::make(eval_errc::builtin, "append: not an array"  + args[0]->inspect());
	}

	static object_ptr println(builtinFuncArg args)
	{
		std::cout << "[monkey]";
		for (auto const& arg: args)
			std::cout << arg->inspect() << ' ';
		std::cout << '\n';
		return object_ptr{ nil::make() };
	}
}; // struct builtin


}
