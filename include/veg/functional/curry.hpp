#ifndef VEG_CURRY_HPP_KV8QZWDES
#define VEG_CURRY_HPP_KV8QZWDES

#include "veg/type_traits/invocable.hpp"
#include "veg/type_traits/constructible.hpp"
#include "veg/internal/tuple_generic.hpp"
#include "veg/internal/prologue.hpp"

namespace veg {
namespace fn {

template <typename Fn, typename... StoredArgs>
struct Curried {
	Fn fn;
	Tuple<StoredArgs...> stored_args;

	VEG_TEMPLATE(
			typename... Args,
			requires(
					VEG_CONCEPT(move_constructible<Fn>) &&
					VEG_ALL_OF(VEG_CONCEPT(move_constructible<Args>))),
			VEG_INLINE constexpr auto push_fwd,
			(... args, Args&&))
	&&VEG_NOEXCEPT_IF(
				VEG_CONCEPT(nothrow_move_constructible<Fn>) &&
				VEG_ALL_OF(VEG_CONCEPT(nothrow_move_constructible<Args>)))
				->Curried<Fn, Args..., StoredArgs...> {
		return push_fwd_impl(VEG_FWD(args)..., VEG_FWD(stored_args));
	}

	VEG_TEMPLATE(
			typename... Args,
			requires(VEG_CONCEPT(invocable<Fn, Args&&..., StoredArgs&&...>)),
			VEG_INLINE constexpr auto
			operator(),
			(... args, Args&&)) &&
			VEG_NOEXCEPT_IF(
					VEG_CONCEPT(nothrow_invocable<Fn, Args&&..., StoredArgs&&...>))
					-> meta::invoke_result_t<Fn, Args&&..., StoredArgs&&...> {
		return Curried::template call<
				(VEG_CONCEPT(nothrow_invocable<Fn, Args&&..., StoredArgs&&...>)),
				meta::invoke_result_t<Fn, Args&&..., StoredArgs&&...>>(
				meta::make_index_sequence<sizeof...(StoredArgs)>{},
				static_cast<Curried&&>(*this),
				VEG_FWD(args)...);
	}

	VEG_TEMPLATE(
			typename... Args,
			requires(
					VEG_CONCEPT(trivially_copy_constructible<Fn>) &&
					VEG_ALL_OF(VEG_CONCEPT(trivially_copy_constructible<StoredArgs>)) &&
					VEG_CONCEPT(invocable<Fn, Args&&..., StoredArgs&&...>)),
			VEG_INLINE constexpr auto
			operator(),
			(... args, Args&&))
	const& VEG_NOEXCEPT_IF(
			VEG_CONCEPT(nothrow_invocable<Fn, Args&&..., StoredArgs&&...>))
			->meta::invoke_result_t<Fn, Args&&..., StoredArgs&&...> {
		return static_cast<Curried>(*this)(VEG_FWD(args)...);
	}

private:
	template <
			bool NoExcept,
			typename Ret,
			typename Self,
			typename... Args,
			usize... Is>
	VEG_INLINE static constexpr auto
	call(meta::index_sequence<Is...> /*iseq*/, Self&& self, Args&&... args)
			VEG_NOEXCEPT_IF(NoExcept) -> Ret {
		return (
				VEG_FWD(self)
						.fn)(VEG_FWD(args)..., static_cast<StoredArgs&&>(internal::tup_::get_impl<Is>(self.stored_args))...);
	}

	template <typename... Args, usize... Is, typename... Ts>
			VEG_INLINE constexpr auto push_fwd_impl(
					Args&&... args,
					internal::tup_::IndexedTuple<meta::index_sequence<Is...>, Ts...>&&
							stored) //
			&& VEG_NOEXCEPT_IF(
						 VEG_CONCEPT(nothrow_move_constructible<Fn>) &&
						 VEG_ALL_OF(VEG_CONCEPT(nothrow_move_constructible<Args>)))
						 -> Curried<Fn, Args..., StoredArgs...> {
		return {
				VEG_FWD(fn),
				{VEG_FWD(args)...,
		     static_cast<Ts&&>(internal::tup_::get_impl<Is>(stored))...}};
	}
};

template <typename Fn, typename... StoredArgs>
struct RCurried {
	Fn fn;
	Tuple<StoredArgs...> stored_args;

	VEG_TEMPLATE(
			typename... Args,
			requires(
					VEG_CONCEPT(move_constructible<Fn>) &&
					VEG_ALL_OF(VEG_CONCEPT(move_constructible<Args>))),
			VEG_INLINE constexpr auto push_fwd,
			(... args, Args&&))
	&&VEG_NOEXCEPT_IF(
				VEG_CONCEPT(nothrow_move_constructible<Fn>) &&
				VEG_ALL_OF(VEG_CONCEPT(nothrow_move_constructible<Args>)))
				->RCurried<Fn, Args..., StoredArgs...> {
		return push_fwd_impl(VEG_FWD(args)..., VEG_FWD(stored_args));
	}

	VEG_TEMPLATE(
			typename... Args,
			requires(VEG_CONCEPT(invocable<Fn, StoredArgs&&..., Args&&...>)),
			VEG_INLINE constexpr auto
			operator(),
			(... args, Args&&)) &&
			VEG_NOEXCEPT_IF(
					VEG_CONCEPT(nothrow_invocable<Fn, StoredArgs&&..., Args&&...>))
					-> meta::invoke_result_t<Fn, StoredArgs&&..., Args&&...> {
		return RCurried::template call<
				(VEG_CONCEPT(nothrow_invocable<Fn, StoredArgs&&..., Args&&...>)),
				meta::invoke_result_t<Fn, StoredArgs&&..., Args&&...>>(
				meta::make_index_sequence<sizeof...(StoredArgs)>{},
				static_cast<RCurried&&>(*this),
				VEG_FWD(args)...);
	}

	VEG_TEMPLATE(
			typename... Args,
			requires(
					VEG_CONCEPT(trivially_copy_constructible<Fn>) &&
					VEG_ALL_OF(VEG_CONCEPT(trivially_copy_constructible<StoredArgs>)) &&
					VEG_CONCEPT(invocable<Fn, StoredArgs&&..., Args&&...>)),
			VEG_INLINE constexpr auto
			operator(),
			(... args, Args&&))
	const& VEG_NOEXCEPT_IF(
			VEG_CONCEPT(nothrow_invocable<Fn, StoredArgs&&..., Args&&...>))
			->meta::invoke_result_t<Fn, StoredArgs&&..., Args&&...> {
		return static_cast<RCurried>(*this)(VEG_FWD(args)...);
	}

private:
	template <
			bool NoExcept,
			typename Ret,
			typename Self,
			typename... Args,
			usize... Is>
	VEG_INLINE static constexpr auto
	call(meta::index_sequence<Is...> /*iseq*/, Self&& self, Args&&... args)
			VEG_NOEXCEPT_IF(NoExcept) -> Ret {
		return (
				VEG_FWD(self)
						.fn)(static_cast<StoredArgs&&>(internal::tup_::get_impl<Is>(self.stored_args))..., VEG_FWD(args)...);
	}

	template <typename... Args, usize... Is, typename... Ts>
			VEG_INLINE constexpr auto push_fwd_impl(
					Args&&... args,
					internal::tup_::IndexedTuple<meta::index_sequence<Is...>, Ts...>&&
							stored) //
			&& VEG_NOEXCEPT_IF(
						 VEG_CONCEPT(nothrow_move_constructible<Fn>) &&
						 VEG_ALL_OF(VEG_CONCEPT(nothrow_move_constructible<Args>)))
						 -> RCurried<Fn, Args..., StoredArgs...> {
		return {
				VEG_FWD(fn),
				{VEG_FWD(args)...,
		     static_cast<Ts&&>(internal::tup_::get_impl<Is>(stored))...}};
	}
};

namespace nb {
struct curry_fwd {
	VEG_TEMPLATE(
			(typename Fn, typename... Args),
			requires(
					VEG_CONCEPT(move_constructible<Fn>) &&
					VEG_ALL_OF(VEG_CONCEPT(move_constructible<Args>))),
			VEG_INLINE constexpr auto
			operator(),
			(fn, Fn&&),
			(... args, Args&&))
	const VEG_NOEXCEPT_IF(
			VEG_CONCEPT(nothrow_move_constructible<Fn>) &&
			VEG_ALL_OF(VEG_CONCEPT(nothrow_move_constructible<Args>)))
			->Curried<Fn, Args...> {
		return {VEG_FWD(fn), {VEG_FWD(args)...}};
	}
};

struct rcurry_fwd {
	VEG_TEMPLATE(
			(typename Fn, typename... Args),
			requires(
					VEG_CONCEPT(move_constructible<Fn>) &&
					VEG_ALL_OF(VEG_CONCEPT(move_constructible<Args>))),
			VEG_INLINE constexpr auto
			operator(),
			(fn, Fn&&),
			(... args, Args&&))
	const VEG_NOEXCEPT_IF(
			VEG_CONCEPT(nothrow_move_constructible<Fn>) &&
			VEG_ALL_OF(VEG_CONCEPT(nothrow_move_constructible<Args>)))
			->RCurried<Fn, Args...> {
		return {VEG_FWD(fn), {VEG_FWD(args)...}};
	}
};
} // namespace nb
VEG_NIEBLOID(curry_fwd);
VEG_NIEBLOID(rcurry_fwd);
} // namespace fn
} // namespace veg

#include "veg/internal/epilogue.hpp"
#endif /* end of include guard VEG_CURRY_HPP_KV8QZWDES */
