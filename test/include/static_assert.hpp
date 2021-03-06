#ifndef VEG_STATIC_ASSERT_HPP_5DPHZJEES
#define VEG_STATIC_ASSERT_HPP_5DPHZJEES

#define STATIC_ASSERT(...) static_assert((__VA_ARGS__), "_")
#if __cplusplus >= 202002L && defined(__clang__)
#define STATIC_ASSERT_IF_20(...) STATIC_ASSERT((__VA_ARGS__))
#else
#define STATIC_ASSERT_IF_20(...) CHECK(__VA_ARGS__)
#endif

#define STATIC_ASSERT(...) static_assert((__VA_ARGS__), "_")
#if __cplusplus >= 201703L
#define STATIC_ASSERT_IF_17(...) STATIC_ASSERT((__VA_ARGS__))
#else
#define STATIC_ASSERT_IF_17(...) CHECK(__VA_ARGS__)
#endif

#if __cplusplus >= 201402L
#define STATIC_ASSERT_IF_14(...) STATIC_ASSERT((__VA_ARGS__))
#else
#define STATIC_ASSERT_IF_14(...) CHECK(__VA_ARGS__)
#endif

#endif /* end of include guard VEG_STATIC_ASSERT_HPP_5DPHZJEES */
