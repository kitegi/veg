#ifndef __VEG_STORAGE_HPP_X0B4XDKES
#define __VEG_STORAGE_HPP_X0B4XDKES

#include "veg/internal/type_traits.hpp"
#include "veg/internal/memory.hpp"
#include "veg/assert.hpp"

namespace veg {
namespace internal {
namespace storage {

struct delete_copy {
  delete_copy() = default;
  ~delete_copy() = default;
  delete_copy(delete_copy const&) = delete;
  delete_copy(delete_copy&&) noexcept = default;
  auto operator=(delete_copy const&) -> delete_copy& = delete;
  auto operator=(delete_copy&&) noexcept -> delete_copy& = default;
};

template <typename T, typename Arg>
struct arg_ctor_to_fn {
  Arg&& arg;
  HEDLEY_ALWAYS_INLINE constexpr auto operator()() const
      noexcept(meta::nothrow_constructible<T, Arg&&>::value) -> T {
    return T{VEG_FWD(arg)};
  }
};
template <typename T>
using copy_ctor_fn = arg_ctor_to_fn<T, T const&>;
template <typename T>
using move_ctor_fn = arg_ctor_to_fn<T, T&&>;

template <typename T, bool = meta::constructible<T>::value>
struct storage_base {
  T inner_val = {};
  constexpr storage_base() = default;
  template <typename Fn>
  HEDLEY_ALWAYS_INLINE constexpr storage_base(int /*unused*/, Fn&& fn) noexcept(
      meta::nothrow_invocable<Fn&&>::value)
      : inner_val(VEG_FWD(fn)()) {}
  HEDLEY_ALWAYS_INLINE explicit constexpr storage_base(T&& arg) noexcept(
      meta::nothrow_move_constructible<T>::value)
      : inner_val{static_cast<T&&>(arg)} {}
  HEDLEY_ALWAYS_INLINE explicit constexpr storage_base(T const& arg) noexcept(
      meta::nothrow_constructible<T, T const&>::value)
      : inner_val{arg} {}
};

template <typename T>
struct storage_base<T, false> {
  T inner_val;
  template <typename Fn>
  HEDLEY_ALWAYS_INLINE constexpr storage_base(int /*unused*/, Fn&& fn) noexcept(
      meta::nothrow_invocable<Fn&&>::value)
      : inner_val(VEG_FWD(fn)()) {}
  HEDLEY_ALWAYS_INLINE explicit constexpr storage_base(T&& arg)
      : inner_val{static_cast<T&&>(arg)} {}
  HEDLEY_ALWAYS_INLINE explicit constexpr storage_base(T const& arg)
      : inner_val{arg} {}
};

template <typename T>
struct storage : storage_base<T> {
  using storage_base<T>::storage_base;
  using storage_base<T>::inner_val;

  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto
  operator=(T const& val) noexcept(meta::nothrow_assignable<T, T const&>::value)
      -> storage& {
    inner_val = val;
    return *this;
  }
  __VEG_CPP14(constexpr)
  HEDLEY_ALWAYS_INLINE auto
  operator=(T&& val) noexcept(meta::nothrow_assignable<T, T&&>::value)
      -> storage& {
    inner_val = VEG_FWD(val);
    return *this;
  }

  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto _get() const noexcept
      -> T const& {
    return inner_val;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mut() noexcept -> T& {
    return inner_val;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mov_ref() && noexcept
      -> T&& {
    return static_cast<T&&>(inner_val);
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mov() && noexcept -> T {
    return static_cast<T&&>(inner_val);
  }

  template <typename U>
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) void assign(U&& rhs) noexcept(
      meta::nothrow_assignable<T&, U&&>::value) {
    inner_val = VEG_FWD(rhs);
  }

  VEG_TEMPLATE(
      (typename V),
      requires(
          meta::swappable<T&, V&>::value && //
          !meta::reference<V>::value),
      HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) void swap,
      (v, storage<V>&))
  noexcept(meta::nothrow_swappable<T&, V&>::value) {
    fn::swap{}(get_mut(), v.get_mut());
  }
};

template <typename T>
HEDLEY_ALWAYS_INLINE auto as_lvalue(T&& arg) noexcept -> T& {
  return arg;
}

struct null_key {};
template <typename T>
struct storage<T&> {
  T* inner_ptr = nullptr;

  HEDLEY_ALWAYS_INLINE explicit constexpr storage(T& arg) noexcept
      : inner_ptr{mem::addressof(arg)} {}
  template <typename Fn>
  HEDLEY_ALWAYS_INLINE constexpr storage(int /*unused*/, Fn&& fn) noexcept(
      meta::nothrow_invocable<Fn&&>::value)
      : inner_ptr(mem::addressof(VEG_FWD(fn)())) {}

  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto _get() const noexcept -> T& {
    return *inner_ptr;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mut() noexcept -> T& {
    return *inner_ptr;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mov_ref() && noexcept
      -> T& {
    return *inner_ptr;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mov() && noexcept -> T& {
    return *inner_ptr;
  }

  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) void swap(storage& v) noexcept {
    swap_::mov_fn_swap::apply(inner_ptr, v.inner_ptr);
  }

private:
  HEDLEY_ALWAYS_INLINE explicit constexpr storage(
      null_key /*unused*/) noexcept {};
  HEDLEY_ALWAYS_INLINE static constexpr auto null() noexcept -> storage {
    return storage{null_key{}};
  }
  template <typename U>
  friend struct meta::value_sentinel_for;
};

template <typename T>
struct storage<T&&> : delete_copy {
  T* inner_ptr = nullptr;

  storage() = default;
  HEDLEY_ALWAYS_INLINE explicit constexpr storage(T&& arg) noexcept
      : inner_ptr{mem::addressof(arg)} {}
  template <typename Fn>
  HEDLEY_ALWAYS_INLINE constexpr storage(int /*unused*/, Fn&& fn) noexcept(
      meta::nothrow_invocable<Fn&&>::value)
      : inner_ptr(mem::addressof(internal::storage::as_lvalue(VEG_FWD(fn)()))) {
  }

  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto _get() const noexcept -> T& {
    return *inner_ptr;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mut() noexcept -> T& {
    return *inner_ptr;
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mov_ref() && noexcept
      -> T&& {
    return static_cast<T&&>(*inner_ptr);
  }
  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) auto get_mov() && noexcept
      -> T&& {
    return static_cast<T&&>(*inner_ptr);
  }

  HEDLEY_ALWAYS_INLINE __VEG_CPP14(constexpr) void swap(storage& v) noexcept {
    swap_::mov_fn_swap::apply(inner_ptr, v.inner_ptr);
  }

private:
  HEDLEY_ALWAYS_INLINE explicit constexpr storage(
      null_key /*unused*/) noexcept {};
  HEDLEY_ALWAYS_INLINE static constexpr auto null() noexcept -> storage {
    return storage{null_key{}};
  }
  template <typename U>
  friend struct meta::value_sentinel_for;
};

// intentionally unsupported. please don't
template <typename T>
struct storage<T const&&>;

template <meta::category_e C>
struct get_inner;

template <>
struct get_inner<meta::category_e::own> {
  template <typename T>
  HEDLEY_ALWAYS_INLINE static constexpr auto apply(storage<T>&& arg)
      __VEG_DEDUCE_RET(static_cast<storage<T>&&>(arg).get_mov());

  template <typename Idx, template <Idx, typename> class Indexed>
  struct with_idx {
    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto
    get_type(Indexed<I, T> const& arg) noexcept -> T;
    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto apply(Indexed<I, T>&& arg)
        __VEG_DEDUCE_RET(static_cast<storage<T>&&>(arg).get_mov());
  };
};

template <>
struct get_inner<meta::category_e::ref> {
  template <typename T>
  HEDLEY_ALWAYS_INLINE static constexpr auto
  apply(storage<T> const& arg) noexcept -> T const& {
    return arg._get();
  }

  template <typename Idx, template <Idx, typename> class Indexed>
  struct with_idx {
    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto
    apply(Indexed<I, T> const& arg) noexcept -> T const& {
      return arg._get();
    }

    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto
    impl(Indexed<I, T> const& arg) noexcept -> T const& {
      return arg;
    }
  };
};
template <>
struct get_inner<meta::category_e::ref_mut> {
  template <typename T>
  HEDLEY_ALWAYS_INLINE static constexpr auto apply(storage<T>& arg) noexcept
      -> T& {
    return arg.get_mut();
  }

  template <typename Idx, template <Idx, typename> class Indexed>
  struct with_idx {
    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto
    apply(Indexed<I, T>& arg) noexcept -> T& {
      return arg.get_mut();
    }

    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto impl(Indexed<I, T>& arg) noexcept
        -> Indexed<I, T>& {
      return arg;
    }
  };
};
template <>
struct get_inner<meta::category_e::ref_mov> {
  template <typename T>
  static constexpr auto apply(storage<T>&& arg) noexcept -> T&& {
    return static_cast<storage<T>&&>(arg).get_mov_ref();
  }

  template <typename Idx, template <Idx, typename> class Indexed>
  struct with_idx {
    template <Idx I, typename T>
    HEDLEY_ALWAYS_INLINE static constexpr auto
    apply(Indexed<I, T>&& arg) noexcept -> T&& {
      return static_cast<storage<T>&&>(arg).get_mov_ref();
    }
  };
};

} // namespace storage
} // namespace internal

template <typename T>
struct meta::value_sentinel_for<veg::internal::storage::storage<T&>>
    : std::integral_constant<i64, 1> {
  HEDLEY_ALWAYS_INLINE static constexpr auto invalid(i64 i) noexcept
      -> ::veg::internal::storage::storage<T&> {
    return VEG_ASSERT(i == 0), ::veg::internal::storage::storage<T&>::null();
  }
  HEDLEY_ALWAYS_INLINE static constexpr auto
  id(::veg::internal::storage::storage<T&> const& arg) noexcept -> i64 {
    return arg.inner_ptr == nullptr ? 0 : -1;
  }
};

template <typename T>
struct meta::value_sentinel_for<veg::internal::storage::storage<T&&>>
    : std::integral_constant<i64, 1> {
  HEDLEY_ALWAYS_INLINE static constexpr auto invalid(i64 i) noexcept
      -> ::veg::internal::storage::storage<T&&> {
    return VEG_ASSERT(i == 0), ::veg::internal::storage::storage<T&&>::null();
  }
  HEDLEY_ALWAYS_INLINE static constexpr auto
  id(::veg::internal::storage::storage<T&&> const& arg) noexcept -> i64 {
    return arg.inner_ptr == nullptr ? 0 : -1;
  }
};

template <typename T>
struct meta::value_sentinel_for<veg::internal::storage::storage<T>>
    : std::integral_constant<i64, meta::value_sentinel_for<T>::value> {
  HEDLEY_ALWAYS_INLINE static constexpr auto invalid(i64 i) noexcept
      -> ::veg::internal::storage::storage<T> {
    return ::veg::internal::storage::storage<T>{
        meta::value_sentinel_for<T>::invalid(i)};
  }
  HEDLEY_ALWAYS_INLINE static constexpr auto
  id(::veg::internal::storage::storage<T> const& arg) noexcept -> i64 {
    return meta::value_sentinel_for<T>::id(arg.inner_val);
  }
};

} // namespace veg

#endif /* end of include guard __VEG_STORAGE_HPP_X0B4XDKES */
