#pragma once
#include <cstdint>
#include <string>
#include <memory>

namespace obj {

using Type = std::size_t;

constexpr Type INTEGER = 1;
constexpr Type BOOLEAN = 2;
constexpr Type NIL = 2;

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

		if (o->type() == INTEGER) delete o;
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

#define M_NIL nil::make()
#define M_TRUE boolean::make(true)
#define M_FALSE boolean::make(false)

}
