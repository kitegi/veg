#ifndef VEG_OPTION_HPP_8NVLXES2S
#define VEG_OPTION_HPP_8NVLXES2S

#include "veg/assert.hpp"
#include "veg/internal/memory.hpp"
#include "veg/internal/storage.hpp"

namespace veg {

template <typename T>
struct option;
namespace meta {
template <typename T>
struct is_option : std::false_type {};
template <typename T>
struct is_option<veg::option<T>> : std::true_type {};

template <typename T>
struct option_inner {};
template <typename T>
struct option_inner<option<T>> {
  using type = T;
};
} // namespace meta
struct none_t {
  friend constexpr auto operator==(none_t /*lhs*/, none_t /*rhs*/) -> bool {
    return true;
  }
  friend constexpr auto operator!=(none_t /*lhs*/, none_t /*rhs*/) -> bool {
    return false;
  }

private:
  constexpr none_t() = default;
  constexpr explicit none_t(none_t* /*unused*/) {}
  template <typename T>
  friend struct meta::internal::static_const;
};
VEG_ODR_VAR(none, none_t);
struct some_t {
  VEG_TEMPLATE(
      (typename T),
      requires(meta::constructible<meta::remove_cvref_t<T>, T&&>),
      constexpr auto
      operator(),
      (arg, T&&))
  const noexcept(meta::nothrow_constructible<meta::remove_cvref_t<T>, T&&>)
      ->option<meta::remove_cvref_t<T>> {
    return {*this, VEG_FWD(arg)};
  }

private:
  constexpr some_t() = default;
  constexpr explicit some_t(some_t* /*unused*/) {}
  template <typename T>
  friend struct meta::internal::static_const;
};
VEG_ODR_VAR(some, some_t);
struct some_ref_t {
  template <typename T>
  constexpr auto operator()(T&& arg) const noexcept -> option<T&&> {
    return {some, VEG_FWD(arg)};
  }

private:
  constexpr some_ref_t() = default;
  constexpr explicit some_ref_t(some_ref_t* /*unused*/) {}
  template <typename T>
  friend struct meta::internal::static_const;
};
VEG_ODR_VAR(some_ref, some_ref_t);

namespace internal {
namespace option_ {

enum kind { has_sentinel, trivial, non_trivial };

template <
    typename T,
    kind = (meta::value_sentinel_for<T>::value > 1) ? has_sentinel
           : meta::trivially_copyable<T>            ? trivial
                                                    : non_trivial>
struct value_sentinel_impl;

template <typename T>
struct value_sentinel_impl<T, has_sentinel>
    : std::integral_constant<i64, meta::value_sentinel_for<T>::value - 1> {
  static constexpr auto invalid(i64 i) {
    return option<T>{meta::value_sentinel_for<T>::invalid(i + 1)};
  }
  static constexpr auto id(option<T> const& arg) {
    if (arg) {
      return -1;
    }
    return meta::value_sentinel_for<T>::id(
        arg.as_ref().unwrap_unchecked(unsafe));
  }
};
template <typename T>
struct value_sentinel_impl<T, trivial>
    : std::integral_constant<i64, i64(static_cast<unsigned char>(-3))> {
  static constexpr auto invalid(i64 i) {
    if (i <= static_cast<unsigned char>(-3)) {
      option<T> val = none;
      val.engaged = static_cast<unsigned char>(2 + i);
      return val;
    }
    terminate();
  }
  static constexpr auto id(option<T> const& arg) {
    return arg.engaged < 2 ? -1 : arg.engaged - 2;
  }
};

template <typename T, typename Self>
constexpr auto as_ref_impl(Self&& self) noexcept
    -> option<meta::collapse_category_t<T, Self&&>> {
  if (self) {
    return {
        some,
        static_cast<meta::collapse_category_t<option<T>, Self&&>>(VEG_FWD(self))
            ._get()};
  }
  return none;
}

template <typename Fn>
struct finally { // NOLINT(cppcoreguidelines-special-member-functions)
  Fn fn;
  VEG_CPP20(constexpr) ~finally() { fn(); }
};

template <typename Fn>
struct do_if {
  bool cond;
  Fn fn;
  void operator()() const noexcept {
    if (cond) {
      fn();
    }
  }
};

struct make_none { // NOLINT
  unsigned char* c;
  VEG_CPP20(constexpr) void operator()() const noexcept {
    mem::construct_at(c);
  }
};

template <
    typename T,
    kind = (meta::value_sentinel_for<storage::storage<T>>::value > 0) //
               ? has_sentinel
               : ((meta::mostly_trivial<T> && meta::constructible<T>))
                     ? trivial
                     : non_trivial>
// trivial
struct option_storage_base {
  storage::storage<T> some = {};
  unsigned char engaged = 0;

  constexpr option_storage_base() = default;
  constexpr explicit option_storage_base(std::remove_const_t<T>&& val)
      : some(VEG_FWD(val)), engaged(1) {}

  VEG_NODISCARD constexpr auto is_engaged() const noexcept -> bool {
    return engaged == 1;
  }

  template <typename... Args>
  constexpr auto
  emplace(Args&&... args) noexcept(meta::nothrow_constructible<T, Args&&...>) {
    VEG_DEBUG_ASSERT(!engaged);
    some = storage::storage<T>(T(args...));
    engaged = true;
  }

  template <typename U>
  constexpr auto assign(U&& rhs) noexcept(noexcept(some.assign(VEG_FWD(rhs)))) {
    VEG_DEBUG_ASSERT(engaged);
    some.assign(VEG_FWD(rhs));
  }

  constexpr auto destroy() noexcept {
    VEG_DEBUG_ASSERT(engaged);
    engaged = 0;
  }

  VEG_NODISCARD constexpr auto _get() const& noexcept -> T const& {
    VEG_DEBUG_ASSERT(is_engaged());
    return some._get();
  }
  VEG_NODISCARD constexpr auto _get() & noexcept -> std::remove_const_t<T>& {
    VEG_DEBUG_ASSERT(is_engaged());
    return some.get_mut();
  }
  VEG_NODISCARD constexpr auto _get() && noexcept -> std::remove_const_t<T>&& {
    VEG_DEBUG_ASSERT(is_engaged());
    return VEG_MOV(some).get_mov_ref();
  }
};

template <typename T>
struct option_storage_base<T, has_sentinel> {
  using sentinel_traits = meta::value_sentinel_for<storage::storage<T>>;

  static_assert(meta::trivially_destructible<T>, "um");
  static_assert(meta::trivially_copyable<storage::storage<T>>, "err");

  storage::storage<T> some = sentinel_traits::invalid(0);

  constexpr option_storage_base() = default;
  constexpr explicit option_storage_base(std::remove_const_t<T>&& val)
      : some(VEG_FWD(val)) {}

  VEG_NODISCARD constexpr auto is_engaged() const noexcept -> bool {
    return sentinel_traits::id(some) < 0;
  }

  template <typename... Args>
  constexpr auto
  emplace(Args&&... args) noexcept(meta::nothrow_constructible<T, Args&&...>) {
    VEG_DEBUG_ASSERT(!is_engaged());
    some = storage::storage<T>(T(args...));
  }

  template <typename U>
  constexpr auto assign(U&& rhs) noexcept(noexcept(some.assign(VEG_FWD(rhs)))) {
    VEG_DEBUG_ASSERT(is_engaged());
    some.assign(VEG_FWD(rhs));
  }

  constexpr auto destroy() noexcept { some = sentinel_traits::invalid(0); }

  VEG_NODISCARD constexpr auto _get() const& noexcept -> T const& {
    VEG_DEBUG_ASSERT(is_engaged());
    return some._get();
  }
  VEG_NODISCARD constexpr auto _get() & noexcept -> std::remove_const_t<T>& {
    VEG_DEBUG_ASSERT(is_engaged());
    return some.get_mut();
  }
  VEG_NODISCARD constexpr auto _get() && noexcept -> std::remove_const_t<T>&& {
    VEG_DEBUG_ASSERT(is_engaged());
    return VEG_MOV(some).get_mov_ref();
  }
};

template <typename T, bool = meta::trivially_destructible<T>>
struct option_storage_nontrivial_base {
  union {
    unsigned char none = {};
    storage::storage<T> some;
  };
  bool engaged = false;

  constexpr option_storage_nontrivial_base() noexcept {};
  constexpr explicit option_storage_nontrivial_base(T&& val) //
      noexcept(meta::nothrow_move_constructible<T>)
      : some(VEG_FWD(val)), engaged(true) {}

  VEG_NODISCARD constexpr auto is_engaged() const noexcept -> bool {
    return engaged;
  }
  constexpr void set_engaged(bool b) noexcept { engaged = b; }
};

template <typename T>
struct
    option_storage_nontrivial_base // NOLINT(cppcoreguidelines-special-member-functions)
    <T, false> {
  union {
    unsigned char none = {};
    storage::storage<T> some;
  };
  bool engaged = false;
  constexpr option_storage_nontrivial_base() noexcept {};
  constexpr explicit option_storage_nontrivial_base(T&& val) //
      noexcept(meta::nothrow_move_constructible<T>)
      : some(VEG_FWD(val)), engaged(true) {}

  VEG_NODISCARD constexpr auto is_engaged() const noexcept -> bool {
    return engaged;
  }
  constexpr void set_engaged(bool b) noexcept { engaged = b; }

  VEG_CPP20(constexpr) ~option_storage_nontrivial_base() {
    if (engaged) {
      mem::destroy_at(mem::addressof(some));
    } else {
      mem::destroy_at(mem::addressof(none));
    }
  }
};

template <typename T>
struct option_storage_base<T, non_trivial> : option_storage_nontrivial_base<T> {

  using option_storage_nontrivial_base<T>::option_storage_nontrivial_base;
  using option_storage_nontrivial_base<T>::some;
  using option_storage_nontrivial_base<T>::none;
  using option_storage_nontrivial_base<T>::is_engaged;
  using option_storage_nontrivial_base<T>::set_engaged;

  template <typename U>
  constexpr auto assign(U&& rhs) noexcept(noexcept(some.assign(VEG_FWD(rhs)))) {
    VEG_DEBUG_ASSERT(is_engaged());
    some.assign(VEG_FWD(rhs));
  }

  template <typename... Args>
  VEG_CPP20(constexpr)
  auto emplace(Args&&... args) noexcept(
      meta::nothrow_constructible<T, Args&&...>) {
    VEG_DEBUG_ASSERT(!is_engaged());

    finally<do_if<make_none>> guard{{true, {mem::addressof(none)}}};

    mem::construct_at(mem::addressof(some), VEG_FWD(args)...);

    guard.fn.cond = false;
    set_engaged(true);
  }

  VEG_CPP20(constexpr) auto destroy() noexcept {
    VEG_DEBUG_ASSERT(is_engaged());
    set_engaged(false);
    finally<make_none> guard{mem::addressof(none)};
    mem::destroy_at(mem::addressof(some));
  }

  constexpr auto _get() const& noexcept -> T const& {
    VEG_DEBUG_ASSERT(is_engaged());
    return some._get();
  }
  constexpr auto _get() & noexcept -> std::remove_const_t<T>& {
    VEG_DEBUG_ASSERT(is_engaged());
    return some.get_mut();
  }
  constexpr auto _get() && noexcept -> std::remove_const_t<T>&& {
    VEG_DEBUG_ASSERT(is_engaged());
    return VEG_MOV(some).get_mov_ref();
  }
};

template <
    typename T,
    bool = meta::reference<T> || meta::trivially_copy_constructible<T> ||
           !meta::constructible<T, T const&>>
struct option_copy_ctor_base : option_storage_base<T> {
  using option_storage_base<T>::option_storage_base;
};
template <typename T>
struct option_copy_ctor_base<T, false> : option_storage_base<T> {
  using option_storage_base<T>::option_storage_base;

  ~option_copy_ctor_base() = default;
  constexpr option_copy_ctor_base(option_copy_ctor_base const& rhs) noexcept(
      meta::nothrow_constructible<T, T const&>)
      : option_storage_base<T>{} {
    if (rhs.is_engaged()) {
      this->emplace(rhs._get());
    }
  }
  option_copy_ctor_base /* NOLINT */ (option_copy_ctor_base&&) = default;
  auto operator=(option_copy_ctor_base const&)
      -> option_copy_ctor_base& = default;
  auto operator= /* NOLINT */(option_copy_ctor_base&&)
      -> option_copy_ctor_base& = default;
};

template <
    typename T,
    bool = meta::reference<T> || meta::trivially_move_constructible<T> ||
           !meta::constructible<T, T&&>>
struct option_move_ctor_base : option_copy_ctor_base<T> {
  using option_copy_ctor_base<T>::option_copy_ctor_base;
};
template <typename T>
struct option_move_ctor_base<T, false> : option_copy_ctor_base<T> {
  using option_copy_ctor_base<T>::option_copy_ctor_base;

  ~option_move_ctor_base() = default;
  option_move_ctor_base(option_move_ctor_base const&) = default;
  constexpr option_move_ctor_base(option_move_ctor_base&& rhs) noexcept(
      meta::nothrow_constructible<T, T&&>)
      : option_copy_ctor_base<T>{} {
    if (rhs.is_engaged()) {
      this->emplace(VEG_MOV(rhs)._get());
    }
  }
  auto operator=(option_move_ctor_base const&)
      -> option_move_ctor_base& = default;
  auto operator= /* NOLINT */(option_move_ctor_base&&)
      -> option_move_ctor_base& = default;
};

template <
    typename T,
    bool = meta::reference<T> || meta::trivially_copy_assignable<T> ||
           !meta::assignable<T, T const&>>
struct option_copy_assign_base : option_move_ctor_base<T> {
  using option_move_ctor_base<T>::option_move_ctor_base;
};
template <typename T>
struct option_copy_assign_base<T, false> : option_move_ctor_base<T> {
  using option_move_ctor_base<T>::option_move_ctor_base;
  ~option_copy_assign_base() = default;

  option_copy_assign_base(option_copy_assign_base const&) = default;
  option_copy_assign_base /* NOLINT */ (option_copy_assign_base&&) = default;
  constexpr auto operator=
      /* NOLINT(cert-oop54-cpp) */(option_copy_assign_base const& rhs) noexcept(
          (meta::nothrow_constructible<T, T const&> &&
           meta::nothrow_assignable<T&, T const&>))
          -> option_copy_assign_base& {
    if (rhs.is_engaged()) {
      if (this->is_engaged()) {
        this->assign(rhs._get());
      } else {
        this->emplace(rhs._get());
      }
    } else {
      if (this->is_engaged()) {
        this->destroy();
      }
    }
    return *this;
  }
  auto operator= /* NOLINT */(option_copy_assign_base&&)
      -> option_copy_assign_base& = default;
};

template <
    typename T,
    bool = meta::reference<T> || meta::trivially_move_assignable<T> ||
           !meta::assignable<T, T&&>>
struct option_move_assign_base : option_copy_assign_base<T> {
  using option_copy_assign_base<T>::option_copy_assign_base;
};
template <typename T>
struct option_move_assign_base<T, false> : option_copy_assign_base<T> {
  using option_copy_assign_base<T>::option_copy_assign_base;

  ~option_move_assign_base() = default;
  option_move_assign_base(option_move_assign_base const&) = default;
  option_move_assign_base /* NOLINT */ (option_move_assign_base&&) = default;
  auto operator=(option_move_assign_base const&)
      -> option_move_assign_base& = default;
  constexpr auto operator=(option_move_assign_base&& rhs) noexcept((
      meta::nothrow_constructible<T, T&&> && meta::nothrow_assignable<T&, T&&>))
      -> option_move_assign_base& {
    if (rhs.is_engaged()) {
      if (this->is_engaged()) {
        this->assign(VEG_MOV(rhs)._get());
      } else {
        this->emplace(VEG_MOV(rhs)._get());
      }
    } else {
      if (this->is_engaged()) {
        this->destroy();
      }
    }
    return *this;
  }
};

template <typename T, bool = meta::constructible<T, T const&>>
struct option_nocopy_interface_base {

  constexpr auto clone() && noexcept(meta::nothrow_move_constructible<T>)
      -> option<T> {
    auto& self = static_cast<option<T>&>(*this);
    if (self) {
      return {some, VEG_MOV(self).as_ref().unwrap_unchecked(unsafe)};
    }
    return none;
  }
};

template <typename T>
struct option_nocopy_interface_base<T, true>
    : option_nocopy_interface_base<T, false> {

  using option_nocopy_interface_base<T, false>::clone;

  constexpr auto
  clone() const& noexcept(meta::nothrow_constructible<T, T const&>)
      -> option<T> {
    auto const& self = static_cast<option<T> const&>(*this);
    if (self) {
      return {some, self.as_ref().unwrap_unchecked(unsafe)};
    }
    return none;
  }
};

template <typename T, typename U, typename Fn>
constexpr auto
cmp(option<T> const& lhs, option<U> const& rhs, Fn fn) noexcept(noexcept(
    fn(lhs.as_cref().unwrap_unchecked(unsafe), //
       rhs.as_cref().unwrap_unchecked(unsafe)))) -> bool {
  if (lhs) {
    if (rhs) {
      return static_cast<bool>(
          fn(lhs.as_cref().unwrap_unchecked(unsafe),
             rhs.as_cref().unwrap_unchecked(unsafe)));
    }
  }
  return (static_cast<bool>(lhs) == static_cast<bool>(rhs));
}

#define VEG_CMP(op, fn, ...)                                                   \
  VEG_TEMPLATE(                                                                \
      (typename T, typename U),                                                \
      requires(__VA_ARGS__),                                                   \
      VEG_NODISCARD constexpr auto                                             \
      operator op,                                                             \
      (lhs, option<T> const&),                                                 \
      (rhs, option<U> const&))                                                 \
  noexcept(noexcept(option_::cmp(lhs, rhs, fn)))                               \
      ->bool {                                                                 \
    return option_::cmp(lhs, rhs, fn);                                         \
  }                                                                            \
  static_assert(true, "")

VEG_CMP(==, cmp_equal, meta::is_equality_comparable_with<T, U>::value);
VEG_CMP(<, cmp_less, meta::is_partially_ordered_with<T, U>::value);
VEG_CMP(>, cmp_greater, meta::is_partially_ordered_with<U, T>::value);
VEG_CMP(<=, cmp_less_equal, meta::is_partially_ordered_with<U, T>::value);
VEG_CMP(>=, cmp_greater_equal, meta::is_partially_ordered_with<T, U>::value);

#undef VEG_CMP

VEG_TEMPLATE(
    (typename T, typename U),
    requires(meta::is_equality_comparable_with<T, U>::value),
    constexpr auto
    operator!=,
    (a, option<T> const&),
    (b, option<U> const&))
noexcept(noexcept(option_::cmp(a, b, cmp_equal))) -> bool {
  return !option_::cmp(a, b, cmp_equal);
}
VEG_TEMPLATE(
    (typename T),
    requires(true),
    constexpr auto
    operator==,
    (lhs, option<T> const&),
    (/*rhs*/, none_t))
noexcept -> bool {
  return !static_cast<bool>(lhs);
}
VEG_TEMPLATE(
    (typename T),
    requires(true),
    constexpr auto
    operator==,
    (/*lhs*/, none_t),
    (rhs, option<T> const&))
noexcept -> bool {
  return !static_cast<bool>(rhs);
}
VEG_TEMPLATE(
    (typename T),
    requires(true),
    constexpr auto
    operator!=,
    (lhs, option<T> const&),
    (/*rhs*/, none_t))
noexcept -> bool {
  return static_cast<bool>(lhs);
}
VEG_TEMPLATE(
    (typename T),
    requires(true),
    constexpr auto
    operator!=,
    (/*lhs*/, none_t),
    (rhs, option<T> const&))
noexcept -> bool {
  return static_cast<bool>(rhs);
}

template <typename T>
struct option_flatten_base {};

template <typename T>
struct option_flatten_base<option<T>> {
  VEG_NODISCARD constexpr auto
  flatten() && noexcept(meta::nothrow_constructible<T, T&&>) -> option<T> {
    if (static_cast<option<option<T>> const&>(*this)) {
      return VEG_MOV(static_cast<option<option<T>>&>(*this))
          .as_ref()
          .unwrap_unchecked(unsafe);
    }
    return none;
  }
};

template <typename T, bool = meta::is_equality_comparable_with<T, T>::value>
struct eq_cmp_base {
  VEG_NODISCARD constexpr auto
  contains(meta::remove_cvref_t<T> const& val) const
      noexcept(noexcept(cmp_equal(val, val))) -> bool {
    auto& self = static_cast<option<T> const&>(*this);
    if (self) {
      return self.as_ref().unwrap_unchecked(unsafe) == val;
    }
    return false;
  }
};

template <typename T>
struct eq_cmp_base<T, false> {};

} // namespace option_
} // namespace internal

template <typename T>
struct VEG_NODISCARD option
    : private internal::option_::option_move_assign_base<T>,
      public internal::option_::option_nocopy_interface_base<T>,
      public internal::option_::option_flatten_base<T>,
      public internal::option_::eq_cmp_base<T> {
  static_assert(meta::move_constructible<T>, "err");

  option() = default;
  ~option() = default;
  option(option const&) = default;
  option(option&&) noexcept(meta::nothrow_move_constructible<T>) = default;
  auto operator=(option const&) & -> option& = default;
  auto operator=(option&&) & noexcept(
      (meta::nothrow_move_constructible<T> && meta::nothrow_move_assignable<T>))
      -> option& = default;

  explicit constexpr option(T value) noexcept(
      meta::nothrow_constructible<T, T&&>)
      : internal::option_::option_move_assign_base<T>(VEG_FWD(value)) {}

  constexpr option(some_t /*tag*/, T value) noexcept(
      meta::nothrow_constructible<T, T&&>)
      : internal::option_::option_move_assign_base<T>(VEG_FWD(value)) {}
  constexpr option // NOLINT(hicpp-explicit-conversions)
      (none_t /*tag*/) noexcept
      : option{} {}

  VEG_TEMPLATE(
      (typename Fn),
      requires(meta::is_invocable<Fn&&, T&&>::value),
      VEG_NODISCARD constexpr auto map,
      (fn, Fn&&)) && noexcept(meta::is_nothrow_invocable<Fn&&, T&&>::value)
      -> option<meta::detected_t<meta::invoke_result_t, Fn&&, T&&>> {

    if (!*this) {
      return none;
    }
    return option<meta::invoke_result_t<Fn&&, T&&>>{
        invoke(VEG_FWD(fn), VEG_MOV(*this).as_ref().unwrap_unchecked(unsafe))};
  }

  VEG_TEMPLATE(
      (typename Fn),
      requires(meta::is_option<
               meta::detected_t<meta::invoke_result_t, Fn&&, T&&>>::value),
      VEG_NODISCARD constexpr auto and_then,
      (fn, Fn&&)) && noexcept(meta::is_nothrow_invocable<Fn&&, T&&>::value)
      -> meta::detected_t<meta::invoke_result_t, Fn&&, T&&> {
    if (!*this) {
      return none;
    }
    return invoke(
        VEG_FWD(fn), VEG_MOV(*this).as_ref().unwrap_unchecked(unsafe));
  }

  VEG_TEMPLATE(
      (typename Fn, typename D),
      requires(
          (meta::is_invocable<D&&>::value && //
           VEG_SAME_AS(
               (meta::detected_t<meta::invoke_result_t, Fn&&, T&&>),
               (meta::detected_t<meta::invoke_result_t, D&&>)))),
      VEG_NODISCARD constexpr auto map_or_else,
      (fn, Fn&&),
      (d, D&&)) && noexcept(meta::is_nothrow_invocable<Fn&&>::value)
      -> meta::detected_t<meta::invoke_result_t, Fn&&, T&&> {
    if (*this) {
      return invoke(
          VEG_FWD(fn), VEG_MOV(*this).as_ref().unwrap_unchecked(unsafe));
    }
    return invoke(VEG_FWD(d));
  }

  VEG_TEMPLATE(
      (typename Fn, typename D),
      requires(VEG_SAME_AS(
          (meta::detected_t<meta::invoke_result_t, Fn&&, T&&> &&), D&&)),
      VEG_NODISCARD constexpr auto map_or,
      (fn, Fn&&),
      (d, D&&)) && noexcept(meta::is_nothrow_invocable<Fn&&, T&&>::value)
      -> meta::detected_t<meta::invoke_result_t, Fn&&, T&&> {
    if (!*this) {
      return VEG_FWD(d);
    }
    return invoke(
        VEG_FWD(fn), VEG_MOV(*this).as_ref().unwrap_unchecked(unsafe));
  }

  VEG_TEMPLATE(
      (typename Fn),
      requires(VEG_SAME_AS(
          (meta::detected_t<meta::invoke_result_t, Fn&&>), veg::option<T>)),
      VEG_NODISCARD constexpr auto or_else,
      (fn, Fn&&)) &&

      noexcept(
          (meta::is_nothrow_invocable<Fn&&>::value &&
           meta::nothrow_constructible<T, T const&>)) -> option<T> {
    if (*this) {
      return {some, VEG_MOV(*this).as_ref().unwrap_unchecked(unsafe)};
    }
    return invoke(VEG_FWD(fn));
  }

  VEG_TEMPLATE(
      (typename Fn),
      requires(meta::constructible<
               bool,
               meta::detected_t<
                   meta::invoke_result_t,
                   Fn,
                   meta::remove_cvref_t<T> const&>>&&
                   meta::move_constructible<T>),
      VEG_NODISCARD constexpr auto filter,
      (fn, Fn&&)) && noexcept -> option<T> {
    if (*this) {
      if (invoke(VEG_FWD(fn), as_cref().unwrap_unchecked(unsafe))) {
        return {some, VEG_MOV(*this).as_ref().unwrap_unchecked(unsafe)};
      }
    }
    return none;
  }

  VEG_NODISCARD constexpr auto as_cref() const noexcept
      -> option<meta::remove_cvref_t<T> const&> {
    if (*this) {
      return {some, as_ref().unwrap_unchecked(unsafe)};
    }
    return none;
  }

  VEG_NODISCARD constexpr explicit operator bool() const noexcept {
    return this->is_engaged();
  }

  VEG_NODISCARD constexpr auto take() noexcept(meta::move_constructible<T>)
      -> option<T> {
    if (*this) {
      T val = VEG_MOV(*this)._get();
      *this = none;
      return {some, VEG_FWD(val)};
    }
    return none;
  }

  VEG_CVREF_DUPLICATE(
      VEG_NODISCARD constexpr auto as_ref(),
      internal::option_::as_ref_impl<T>,
      ());

  VEG_NODISCARD HEDLEY_ALWAYS_INLINE constexpr auto unwrap_unchecked(
      unsafe_t /*tag*/) && noexcept(meta::nothrow_move_constructible<T>) -> T {
    return VEG_MOV(*this)._get();
  }

  VEG_NODISCARD constexpr auto
  unwrap() && noexcept(meta::nothrow_move_constructible<T>) -> T {
    VEG_ASSERT(*this);
    return VEG_MOV(*this).unwrap_unchecked(unsafe);
  }

private:
  template <typename U, internal::option_::kind K>
  friend struct internal::option_::value_sentinel_impl;

  template <typename U, bool B>
  friend struct internal::option_::option_nocopy_interface_base;

  template <typename U, typename Self>
  friend constexpr auto internal::option_::as_ref_impl(Self&& self) noexcept
      -> option<meta::collapse_category_t<U, Self&&>>;
};
VEG_CPP17(

    template <typename T> option(some_t, T) -> option<T>;
    template <typename T> option(some_ref_t, T&&) -> option<T&&>;

)

template <typename T>
struct meta::value_sentinel_for<option<T>>
    : veg::internal::option_::value_sentinel_impl<T> {};

} // namespace veg

#endif /* end of include guard VEG_OPTION_HPP_8NVLXES2S */
