#ifndef VEG_META_INT_FIX_HPP_7S9Y48TFS
#define VEG_META_INT_FIX_HPP_7S9Y48TFS

#include "veg/type_traits/tags.hpp"
#include "veg/internal/std.hpp"
#include "veg/internal/fmt.hpp"
#include "veg/util/compare.hpp"
#include "veg/internal/prologue.hpp"

namespace veg {
struct Dyn;
template <i64 N>
struct Fix;

namespace internal {
template <typename L, typename R>
struct binary_traits {
	using add = void;
	using sub = void;
	using mul = void;
	using div = void;

	using cmp_eq = void;
	using cmp_neq = void;
	using cmp_lt = void;
	using cmp_le = void;
	using cmp_gt = void;
	using cmp_ge = void;
};

namespace idx {
namespace adl {
template <typename T>
struct IdxBase {};
} // namespace adl
} // namespace idx
namespace meta_ {
template <typename T>
struct is_fix : false_type {};
template <i64 N>
struct is_fix<Fix<N>> : true_type {};
} // namespace meta_
} // namespace internal

namespace concepts {
VEG_DEF_CONCEPT(
		typename T,
		index,
		VEG_CONCEPT(same<T, Dyn>) || internal::meta_::is_fix<T>::value);
} // namespace concepts

enum struct Ternary : unsigned char {
	no,
	maybe,
	yes,
};

constexpr auto no = Ternary::no;
constexpr auto maybe = Ternary::maybe;
constexpr auto yes = Ternary::yes;
using no_c = meta::constant<Ternary, Ternary::no>;
using maybe_c = meta::constant<Ternary, Ternary::maybe>;
using yes_c = meta::constant<Ternary, Ternary::yes>;

template <Ternary T>
struct Boolean;

template <Ternary T>
struct Boolean {
	constexpr Boolean() VEG_NOEXCEPT = default;
	using type = meta::constant<Ternary, T>;

	VEG_INLINE constexpr Boolean(Boolean<maybe> /*b*/, Unsafe /*tag*/)
			VEG_NOEXCEPT;
	VEG_INLINE constexpr Boolean // NOLINT(hicpp-explicit-conversions)
			(Boolean<maybe> b) VEG_NOEXCEPT;

	VEG_NODISCARD VEG_INLINE constexpr friend auto
	operator!(Boolean /*arg*/) VEG_NOEXCEPT -> Boolean<T == yes ? no : yes> {
		return {};
	}
	VEG_NODISCARD VEG_INLINE explicit constexpr
	operator bool() const VEG_NOEXCEPT {
		return T == yes;
	}

private:
	void print(fmt::Buffer& out) const {
		constexpr auto const& yes_str = "maybe(true)";
		constexpr auto const& no_str = "maybe(false)";
		char const* str = (T == yes) ? yes_str : no_str;
		auto len = i64((T == yes) ? sizeof(yes_str) : sizeof(no_str)) - 1;
		out.insert(out.size(), str, len);
	}
};

template <i64 N>
struct Fix : internal::idx::adl::IdxBase<Fix<N>> {
	constexpr Fix() VEG_NOEXCEPT = default;
	VEG_INLINE constexpr Fix(Dyn /*arg*/, Unsafe /*tag*/) VEG_NOEXCEPT;
	VEG_INLINE constexpr Fix // NOLINT(hicpp-explicit-conversions)
			(Dyn arg) VEG_NOEXCEPT;
	VEG_TEMPLATE((i64 M), requires((M != N)), constexpr Fix, (/*arg*/, Fix<M>)) =
			delete;

	VEG_NODISCARD VEG_INLINE explicit constexpr
	operator i64() const VEG_NOEXCEPT {
		return N;
	}
	VEG_NODISCARD VEG_INLINE constexpr auto operator+() const VEG_NOEXCEPT
			-> Fix {
		return {};
	}
	VEG_NODISCARD VEG_INLINE constexpr auto operator-() const VEG_NOEXCEPT
			-> Fix<-N> {
		return {};
	}

	VEG_TEMPLATE(
			(typename R),
			requires(VEG_CONCEPT(index<R>)),
			VEG_NODISCARD VEG_INLINE constexpr auto
			operator+,
			(b, R))
	const VEG_DEDUCE_RET(internal::binary_traits<Fix, R>::add_fn(*this, b));

	VEG_TEMPLATE(
			(typename R),
			requires(VEG_CONCEPT(index<R>)),
			VEG_NODISCARD VEG_INLINE constexpr auto
			operator-,
			(b, R))
	const VEG_DEDUCE_RET(internal::binary_traits<Fix, R>::sub_fn(*this, b));

	VEG_TEMPLATE(
			(typename R),
			requires(VEG_CONCEPT(index<R>)),
			VEG_NODISCARD VEG_INLINE constexpr auto
			operator*,
			(b, R))
	const VEG_DEDUCE_RET(internal::binary_traits<Fix, R>::mul_fn(*this, b));

	VEG_TEMPLATE(
			(typename R),
			requires(
					VEG_CONCEPT(index<R>) &&
					VEG_CONCEPT(index<typename internal::binary_traits<Fix, R>::div>)),
			VEG_NODISCARD VEG_INLINE constexpr auto
			operator/,
			(b, R))
	const VEG_DEDUCE_RET(internal::binary_traits<Fix, R>::div_fn(*this, b));

	VEG_TEMPLATE(
			(typename R),
			requires(
					VEG_CONCEPT(index<R>) &&
					VEG_CONCEPT(index<typename internal::binary_traits<Fix, R>::mod>)),
			VEG_NODISCARD VEG_INLINE constexpr auto
			operator%,
			(b, R))
	const VEG_DEDUCE_RET(internal::binary_traits<Fix, R>::mod_fn(*this, b));

#define VEG_CMP(Name, Op)                                                      \
	VEG_TEMPLATE(                                                                \
			(typename R),                                                            \
			requires(VEG_CONCEPT(index<R>)),                                         \
			VEG_NODISCARD VEG_INLINE constexpr auto                                  \
			operator Op,                                                             \
			(b, R))                                                                  \
	const VEG_DEDUCE_RET(                                                        \
			internal::binary_traits<Fix, R>::cmp_##Name##_fn(*this, b))

	VEG_CMP(eq, ==);
	VEG_CMP(neq, !=);
	VEG_CMP(lt, <);
	VEG_CMP(le, <=);
	VEG_CMP(gt, >);
	VEG_CMP(ge, >=);

#undef VEG_CMP
};

namespace internal {
struct Error {
	constexpr auto operator()(u64 const* fail = nullptr) const VEG_NOEXCEPT
			-> u64 {
		return *fail;
	}
};

using parser = auto (*)(char, Error) -> u64;
constexpr auto parse_digit_2(char c, Error e) VEG_NOEXCEPT -> u64 {
	return (c == '0') ? 0 : (c == '1' ? 1 : e());
}
constexpr auto parse_digit_8(char c, Error e) VEG_NOEXCEPT -> u64 {
	return (c >= '0' && c <= '7') ? u64(c - '0') : e();
}
constexpr auto parse_digit_10(char c, Error e) VEG_NOEXCEPT -> u64 {
	return (c >= '0' && c <= '9') ? u64(c - '0') : e();
}
constexpr auto parse_digit_16(char c, Error e) VEG_NOEXCEPT -> u64 {
	return (c >= '0' && c <= '9') //
	           ? u64(c - '0')
	           : (c >= 'a' && c <= 'f') //
	                 ? u64(c - 'a')
	                 : (c >= 'A' && c <= 'F') //
	                       ? u64(c - 'A')
	                       : e();
}

constexpr auto parse_digit(u64 radix) VEG_NOEXCEPT -> parser {
	return radix == 2
	           ? parse_digit_2
	           : (radix == 8
	                  ? parse_digit_8
	                  : (radix == 10 ? parse_digit_10
	                                 : (radix == 16 ? parse_digit_16 : nullptr)));
}

constexpr auto
parse_num(char const* str, u64 len, u64 radix, Error e) VEG_NOEXCEPT -> u64 {
	return (len == 0) ? 0
	                  : radix * parse_num(str, len - 1, radix, e) +
	                        (parse_digit(radix)(str[len - 1], e));
}

constexpr auto parse_int(char const* str, u64 len, Error e) VEG_NOEXCEPT
		-> u64 {
	return (len == 0) //
	           ? e()
	           : ((str[0] == '0')   //
	                  ? ((len == 1) //
	                         ? 0
	                         : (str[1] == 'b' || str[1] == 'B') //
	                               ? parse_num(str + 2, len - 2, 2, e)
	                               : (str[1] == 'x' || str[1] == 'X') //
	                                     ? parse_num(str + 2, len - 2, 16, e)
	                                     : parse_num(str + 1, len - 1, 8, e))
	                  : parse_num(str, len, 10, e));
}

template <char... Chars>
struct char_seq {
	static constexpr char value[] = {Chars...};
};

template <i64 N, i64 M>
struct binary_traits<Fix<N>, Fix<M>> {

#define VEG_OP(Name, Op)                                                       \
	using Name /* NOLINT(bugprone-macro-parentheses) */ = Fix<N Op M>;           \
	VEG_NODISCARD VEG_INLINE static constexpr auto Name##_fn(Fix<N>, Fix<M>)     \
			VEG_NOEXCEPT->Name {                                                     \
		return {};                                                                 \
	}                                                                            \
	static_assert(true, "")

#define VEG_CMP(Name, Op)                                                      \
	using Name /* NOLINT(bugprone-macro-parentheses) */ =                        \
			Boolean<(N Op M) ? yes : no>;                                            \
	VEG_NODISCARD VEG_INLINE static constexpr auto Name##_fn(Fix<N>, Fix<M>)     \
			VEG_NOEXCEPT->Name {                                                     \
		return {};                                                                 \
	}                                                                            \
	static_assert(true, "")

	VEG_OP(add, +);
	VEG_OP(sub, -);
	VEG_OP(mul, *);
	VEG_CMP(cmp_eq, ==);
	VEG_CMP(cmp_neq, !=);
	VEG_CMP(cmp_lt, <);
	VEG_CMP(cmp_le, <=);
	VEG_CMP(cmp_gt, >);
	VEG_CMP(cmp_ge, >=);

	using div = meta::conditional_t<M == 0, void, Fix<N / (M != 0 ? M : 1)>>;
	using mod = meta::conditional_t<M == 0, void, Fix<N % (M != 0 ? M : 1)>>;

	VEG_NODISCARD VEG_INLINE static constexpr auto
	div_fn(Fix<N> /*a*/, Fix<M> /*b*/) VEG_NOEXCEPT -> div {
		return div();
	}
	VEG_NODISCARD VEG_INLINE static constexpr auto
	mod_fn(Fix<N> /*a*/, Fix<M> /*b*/) VEG_NOEXCEPT -> mod {
		return mod();
	}

#undef VEG_OP
#undef VEG_CMP
};
namespace idx {
namespace adl {} // namespace adl
} // namespace idx
} // namespace internal

inline namespace literals {
template <char... Chars>
VEG_INLINE constexpr auto
operator"" _c() VEG_NOEXCEPT -> Fix<internal::parse_int(
		internal::char_seq<Chars...>::value, sizeof...(Chars), internal::Error{})> {
	return {};
}
} // namespace literals

template <>
struct fmt::Debug<Boolean<yes>> {
	static void to_string(fmt::Buffer& out, Ref<Boolean<yes>> /*val*/) {
		out.insert(out.size(), "yes", 3);
	}
};
template <>
struct fmt::Debug<Boolean<no>> {
	static void to_string(fmt::Buffer& out, Ref<Boolean<no>> /*val*/) {
		out.insert(out.size(), "no", 2);
	}
};

template <i64 N>
struct fmt::Debug<Fix<N>> {
	static void to_string(fmt::Buffer& out, Ref<Fix<N>> /*val*/) {
		out.insert(out.size(), "Fix[", 4);
		Debug<i64>::to_string(out, ref(N));
		out.insert(out.size(), "]", 1);
	}
};

template <i64 N>
struct cmp::is_eq<Fix<N>> : meta::true_type {};
template <i64 N>
struct cmp::is_ord<Fix<N>> : meta::true_type {};
template <>
struct cmp::is_eq<Boolean<yes>> : meta::true_type {};
template <>
struct cmp::is_ord<Boolean<yes>> : meta::true_type {};
template <>
struct cmp::is_eq<Boolean<no>> : meta::true_type {};
template <>
struct cmp::is_ord<Boolean<no>> : meta::true_type {};
} // namespace veg

#include "veg/internal/epilogue.hpp"
#endif /* end of include guard VEG_META_INT_FIX_HPP_7S9Y48TFS */
