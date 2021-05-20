#ifndef VEG_INVOCABLE_HPP_GVSWRKAYS
#define VEG_INVOCABLE_HPP_GVSWRKAYS

#include "veg/type_traits/core.hpp"
#include "veg/type_traits/constructible.hpp"
#include "veg/internal/prologue.hpp"

namespace veg {
namespace internal {
namespace meta_ {
template <typename Fn, typename... Args>
using call_expr = decltype(VEG_DECLVAL(Fn &&)(VEG_DECLVAL(Args &&)...));
} // namespace meta_
} // namespace internal
namespace meta {
template <typename Fn, typename... Args>
using invoke_result_t =
		meta::detected_t<internal::meta_::call_expr, Fn&&, Args&&...>;
} // namespace meta

namespace concepts {

namespace aux {
VEG_DEF_CONCEPT_DISJUNCTION(
		(typename T, typename U),
		matches_if_not_void,
		((concepts::, same<void, U>), (concepts::, same<T, U>)));

VEG_DEF_CONCEPT(
		(typename Fn, typename... Args),
		nothrow_invocable_pre,
		VEG_IS_NOEXCEPT(
				VEG_DECLVAL_NOEXCEPT(Fn&&)(VEG_DECLVAL_NOEXCEPT(Args&&)...)));
} // namespace aux

#if __cplusplus >= 202002L

template <typename Fn, typename... Args>
concept invocable = requires(Fn&& fn, Args&&... args) {
	static_cast<Fn&&>(fn)(static_cast<Args&&>(args)...);
};
template <typename Fn, typename... Args>
concept nothrow_invocable = requires(Fn&& fn, Args&&... args) {
	{ static_cast<Fn&&>(fn)(static_cast<Args&&>(args)...) }
	noexcept;
};

#else
VEG_DEF_CONCEPT(
		(typename Fn, typename... Args),
		invocable,
		VEG_CONCEPT(detected<internal::meta_::call_expr, Fn, Args&&...>));

VEG_DEF_CONCEPT_CONJUNCTION(
		(typename Fn, typename... Args),
		nothrow_invocable,
		((, invocable<Fn, Args&&...>),
     (aux::, nothrow_invocable_pre<Fn, Args&&...>)));
#endif

#if __cplusplus >= 201703L
VEG_DEF_CONCEPT_CONJUNCTION(
		(typename Fn, typename Ret, typename... Args),
		invocable_r,
		((, invocable<Fn, Args&&...>),
     (aux::, matches_if_not_void<meta::invoke_result_t<Fn, Args&&...>, Ret>)));
#else
VEG_DEF_CONCEPT_CONJUNCTION(
		(typename Fn, typename Ret, typename... Args),
		invocable_r,
		((, invocable<Fn, Args&&...>),
     (aux::, matches_if_not_void<meta::invoke_result_t<Fn, Args&&...>, Ret>),
     (concepts::,
      move_constructible<meta::conditional_t<
					VEG_CONCEPT(same<meta::invoke_result_t<Fn, Args&&...>, void>),
					decltype(nullptr),
					meta::invoke_result_t<Fn, Args&&...>>>)));
#endif

VEG_DEF_CONCEPT_CONJUNCTION(
		(typename Fn, typename Ret, typename... Args),
		nothrow_invocable_r,
		((, invocable_r<Fn, Ret, Args&&...>),
     (, nothrow_invocable<Fn, Args&&...>)));

} // namespace concepts
} // namespace veg

#include "veg/internal/epilogue.hpp"
#endif /* end of include guard VEG_INVOCABLE_HPP_GVSWRKAYS */
