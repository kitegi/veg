#ifndef VEG_SLICE_HPP_GKSTE2JDS
#define VEG_SLICE_HPP_GKSTE2JDS

#include "veg/util/assert.hpp"
#include "veg/option.hpp"
#include "veg/util/get.hpp"
#include "veg/internal/prologue.hpp"

namespace veg {
template <typename T, i64 N>
struct Array {
	static_assert(N > 0, ".");
	T self[static_cast<usize>(N)];

	VEG_NODISCARD VEG_INLINE auto data() VEG_NOEXCEPT -> T* { return self; }
	VEG_NODISCARD VEG_INLINE auto data() const VEG_NOEXCEPT -> T const* {
		return self;
	}
	VEG_NODISCARD VEG_INLINE auto size() const VEG_NOEXCEPT -> i64 { return N; }

	VEG_NODISCARD VEG_INLINE auto operator[](i64 i) && VEG_NOEXCEPT
			-> T&& = delete;
	VEG_NODISCARD VEG_INLINE auto operator[](i64 i) & VEG_NOEXCEPT -> T& {
		VEG_INTERNAL_ASSERT_PRECONDITIONS(i >= i64(0), i < N);
		return self[i];
	}
	VEG_NODISCARD VEG_INLINE auto operator[](i64 i) const & VEG_NOEXCEPT
																													-> T const& {
		VEG_INTERNAL_ASSERT_PRECONDITIONS(i >= i64(0), i < N);
		return self[i];
	}

	VEG_NODISCARD auto at(i64 i) && VEG_NOEXCEPT -> Option<T&&> = delete;
	VEG_NODISCARD VEG_INLINE auto at(i64 i) & VEG_NOEXCEPT -> Option<T&> {
		if (i >= 0 && i < N) {
			return {some, self[i]};
		}
		return none;
	}
	VEG_NODISCARD VEG_INLINE auto at(i64 i) const & VEG_NOEXCEPT
																									-> Option<T const&> {
		if (i >= 0 && i < N) {
			return {some, self[i]};
		}
		return none;
	}
};

namespace internal {

struct member_fn_data_size {
	template <typename R, typename T>
	using dtype_r = decltype(void(static_cast<R* const*>(
			static_cast<decltype(VEG_DECLVAL(T&).data()) const*>(nullptr))));

	template <typename T>
	using dtype = decltype(void(VEG_DECLVAL(T&).data()));

	template <typename T>
	using stype = decltype(void(static_cast<i64>(VEG_DECLVAL(T&).size())));

	template <typename T>
	VEG_INLINE static constexpr auto d(T& arg) VEG_NOEXCEPT
			-> decltype(arg.data()) {
		return arg.data();
	}
	template <typename T>
	VEG_INLINE static constexpr auto s(T& arg) VEG_NOEXCEPT -> i64 {
		return i64(arg.size());
	}
};
template <typename R, typename T>
struct has_members_r : meta::conjunction<
													 meta::bool_constant<VEG_CONCEPT(
															 detected<member_fn_data_size::dtype_r, R, T&>)>,
													 meta::bool_constant<VEG_CONCEPT(
															 detected<member_fn_data_size::stype, T&>)>>,
											 member_fn_data_size {};
template <typename T>
struct has_members : meta::conjunction<
												 meta::bool_constant<VEG_CONCEPT(
														 detected<member_fn_data_size::dtype, T&>)>,
												 meta::bool_constant<VEG_CONCEPT(
														 detected<member_fn_data_size::stype, T&>)>>,
										 member_fn_data_size {};

struct array_data_size {
	template <typename T>
	VEG_INLINE static constexpr auto d(T& arg) VEG_NOEXCEPT -> decltype(+arg) {
		return +arg;
	}
	template <typename T, usize N>
	VEG_INLINE static constexpr auto s(T (&/*arg*/)[N]) VEG_NOEXCEPT -> i64 {
		return i64(N);
	}
};

template <typename R, typename T>
struct has_array_data2
		: meta::bool_constant<VEG_CONCEPT(
					convertible<
							T&,
							R (&)[sizeof(T) / sizeof(decltype(VEG_DECLVAL(T &&)[0]))]>)> {};

template <typename R, typename T>
struct has_array_data_r
		: meta::conjunction<meta::is_bounded_array<T>, has_array_data2<R, T>>,
			array_data_size {};

template <typename T>
struct has_array_data : meta::is_bounded_array<T>, array_data_size {};

template <typename R, typename T>
struct has_data_r
		: meta::disjunction<has_members_r<R, T&>, has_array_data_r<R, T>> {};
template <typename T>
struct has_data : meta::disjunction<has_members<T&>, has_array_data<T>> {};

// prevents forwarding ctor from hiding copy/move ctors
// disables implicitly generated deduction guides
template <typename T>
struct SliceCommon {
	SliceCommon() VEG_NOEXCEPT = default;

	VEG_INLINE
	constexpr SliceCommon(T* data, i64 count, Unsafe /* tag */) VEG_NOEXCEPT
			: m_begin{data},
				m_count{count} {}

	VEG_INLINE
	constexpr SliceCommon(T* data, i64 count, Safe /* tag */ = {}) VEG_NOEXCEPT :
#if __cplusplus >= 201402L

			m_begin{data},
			m_count{count} {
		VEG_INTERNAL_ASSERT_PRECONDITION(count >= i64(0));
		if (count > 0) {
			VEG_INTERNAL_ASSERT_PRECONDITION(data);
		}
	}

#else

			m_begin{
					(VEG_INTERNAL_ASSERT_PRECONDITION(count >= i64(0)),
	         (count > 0 ? VEG_INTERNAL_ASSERT_PRECONDITION(data) : void(0)),
	         data)},
			m_count{count} {
	}

#endif

	// COMPAT: check if slice_ctor_common is a base of Rng
	VEG_TEMPLATE(
			(typename Rng),
			requires(internal::has_data_r<T, meta::uncvref_t<Rng>>::value),
			VEG_INLINE constexpr SliceCommon,
			(rng, Rng&&))
	VEG_NOEXCEPT
			: SliceCommon(
						static_cast<T*>(
								internal::has_data_r<T, meta::uncvref_t<Rng>>::d(rng)),
						static_cast<i64>(has_data_r<T, meta::uncvref_t<Rng>>::s(rng))) {}

	T* m_begin = nullptr;
	i64 m_count = 0;
};

template <typename T>
struct SliceCtor : SliceCommon<T> {
	using SliceCommon<T>::SliceCommon;
};

template <typename T>
struct SliceCtor<T const> : SliceCommon<T const> {
	using SliceCommon<T const>::SliceCommon;

	constexpr SliceCtor(std::initializer_list<T> lst) VEG_NOEXCEPT
			: SliceCommon<T const>{lst.begin(), static_cast<i64>(lst.size())} {}
};
} // namespace internal

template <typename T>
struct Slice : private internal::SliceCtor<T> {
	using internal::SliceCtor<T>::SliceCtor;
	VEG_NODISCARD
	VEG_INLINE
	constexpr auto data() const VEG_NOEXCEPT -> T* {
		return internal::SliceCommon<T>::m_begin;
	}
	VEG_NODISCARD
	VEG_INLINE
	constexpr auto size() const VEG_NOEXCEPT -> i64 {
		return internal::SliceCommon<T>::m_count;
	}
	VEG_NODISCARD
	VEG_INLINE
	constexpr auto operator[](i64 i) const VEG_NOEXCEPT -> T& {
		return VEG_INTERNAL_ASSERT_PRECONDITIONS( //
							 (i >= i64(0)),
							 (i < size())),
		       *(data() + i);
	}
	VEG_NODISCARD
	VEG_INLINE
	VEG_CPP14(constexpr) auto at(i64 i) const VEG_NOEXCEPT -> Option<T&> {
		if (i > 0 || i <= size()) {
			return {some, *(data() + i)};
		}
		return none;
	}
};

template <>
struct Slice<void> : Slice<unsigned char> {
	using Slice<unsigned char>::Slice;

	VEG_TEMPLATE(
			(typename T),
			requires(
					!VEG_CONCEPT(const_type<T>) && //
					VEG_CONCEPT(trivially_copyable<T>)),
			VEG_INLINE Slice,
			(s, Slice<T>))
	VEG_NOEXCEPT : Slice(
										 reinterpret_cast<unsigned char*>(s.data()),
										 s.size() * static_cast<i64>(sizeof(T))) {}
};
template <>
struct Slice<void const> : Slice<unsigned char const> {
	using Slice<unsigned char const>::Slice;
	VEG_TEMPLATE(
			(typename T),
			requires VEG_CONCEPT(trivially_copyable<T>),
			VEG_INLINE Slice,
			(s, Slice<T>))
	VEG_NOEXCEPT : Slice{
										 reinterpret_cast<unsigned char const*>(s.data()),
										 s.size() * static_cast<i64>(sizeof(T))} {}
};

namespace nb {
struct slice {
	VEG_TEMPLATE(
			(typename Rng),
			requires(
					VEG_CONCEPT(constructible< //
											veg::Slice<meta::unptr_t<
													decltype(internal::has_data<meta::uncvref_t<Rng>>::d(
															VEG_DECLVAL(Rng&)))>>,
											Rng&&>)),
			VEG_INLINE auto
			operator(),
			(rng, Rng&&))
	const VEG_NOEXCEPT->veg::Slice<meta::unptr_t<
			decltype(internal::has_data<meta::uncvref_t<Rng>>::d(rng))>> {
		return {VEG_FWD(rng)};
	}
};
} // namespace nb
VEG_NIEBLOID(slice);

namespace cpo {
template <typename T>
struct is_trivially_constructible<Slice<T>> : meta::bool_constant<true> {};
} // namespace cpo
} // namespace veg

#include "veg/internal/epilogue.hpp"
#endif /* end of include guard VEG_SLICE_HPP_GKSTE2JDS */
