#pragma once

#include <map>
#include <string>
#include <type_traits>
#include <list>
#include <forward_list>
#include <array>
#include <vector>
#include <variant>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <queue>
#include <stack>
#include <complex>
#include <valarray>
#include <tuple>

template<typename T1, typename T2>
using enable_is_same = std::enable_if_t<std::is_same_v<T1, T2>>;
template<typename Ret, typename Foo, typename ... Args>
using enable_is_same_result_of = enable_is_same<std::result_of_t<Foo(Args...)>, Ret>;
template<typename T1, typename T2>
using enable_is_not_same = std::enable_if_t<!std::is_same_v<T1, T2>>;
template<typename Ret, typename Foo, typename ... Args>
using enable_is_not_same_result_of = enable_is_same<std::result_of_t<Foo(Args...)>, Ret>;

// vector
template<typename T>        struct is_vector : std::false_type {};
template<typename ... Args> struct is_vector<std::vector<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_vector_v = is_vector<T>::value;

// array
template<typename T>        struct is_array : std::false_type {};
template<typename T, std::size_t Nb>
                            struct is_array<std::array<T, Nb>> : std::true_type {};
template<typename T>        constexpr bool is_array_v = is_array<T>::value;

// bitset
template<typename T>        struct is_bitset : std::false_type {};
template<std::size_t Nb>    struct is_bitset<std::bitset<Nb>> : std::true_type {};
template<typename T>        constexpr bool is_bitset_v = is_bitset<T>::value;

// deque
template<typename T>        struct is_deque : std::false_type {};
template<typename ... Args> struct is_deque<std::deque<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_deque_v = is_deque<T>::value;

// queue
template<typename T>        struct is_queue : std::false_type {};
template<typename ... Args> struct is_queue<std::queue<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_queue_v = is_queue<T>::value;

// priority_queue
template<typename T>        struct is_priority_queue : std::false_type {};
template<typename ... Args> struct is_priority_queue<std::priority_queue<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_priority_queue_v = is_priority_queue<T>::value;

// list
template<typename T>        struct is_list : std::false_type {};
template<typename ... Args> struct is_list<std::list<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_list_v = is_list<T>::value;

// forward list
template<typename T>        struct is_forward_list : std::false_type {};
template<typename ... Args> struct is_forward_list<std::forward_list<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_forward_list_v = is_forward_list<T>::value;

// set
template<typename T>        struct is_set : std::false_type {};
template<typename ... Args> struct is_set<std::set<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_set_v = is_set<T>::value;

// map
template<typename T>        struct is_map : std::false_type {};
template<typename ... Args> struct is_map<std::map<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_map_v = is_map<T>::value;

// multiset
template<typename T>        struct is_multiset : std::false_type {};
template<typename ... Args> struct is_multiset<std::multiset<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_multiset_v = is_multiset<T>::value;

// multimap
template<typename T>        struct is_multimap : std::false_type {};
template<typename ... Args> struct is_multimap<std::multimap<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_multimap_v = is_multimap<T>::value;

// unordered_set
template<typename T>        struct is_unordered_set : std::false_type {};
template<typename ... Args> struct is_unordered_set<std::unordered_set<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_unordered_set_v = is_unordered_set<T>::value;

// unordered_map
template<typename T>        struct is_unordered_map : std::false_type {};
template<typename ... Args> struct is_unordered_map<std::unordered_map<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_unordered_map_v = is_unordered_map<T>::value;

// unordered_multiset
template<typename T>        struct is_unordered_multiset : std::false_type {};
template<typename ... Args> struct is_unordered_multiset<std::unordered_multiset<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_unordered_multiset_v = is_unordered_multiset<T>::value;

// unordered_multimap
template<typename T>        struct is_unordered_multimap : std::false_type {};
template<typename ... Args> struct is_unordered_multimap<std::unordered_multimap<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_unordered_multimap_v = is_unordered_multimap<T>::value;

// stack
template<typename T>        struct is_stack : std::false_type {};
template<typename ... Args> struct is_stack<std::stack<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_stack_v = is_stack<T>::value;

// pair
template<typename T>        struct is_pair : std::false_type {};
template<typename ... Args> struct is_pair<std::pair<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_pair_v = is_pair<T>::value;

// complex
template<typename T>        struct is_complex : std::false_type {};
template<typename ... Args> struct is_complex<std::complex<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_complex_v = is_complex<T>::value;

// valarray
template<typename T>        struct is_valarray : std::false_type {};
template<typename ... Args> struct is_valarray<std::valarray<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_valarray_v = is_valarray<T>::value;

// tuple
template<typename T>        struct is_tuple : std::false_type {};
template<typename ... Args> struct is_tuple<std::tuple<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_tuple_v = is_tuple<T>::value;

// variant
template<typename T>        struct is_variant : std::false_type {};
template<typename ... Args> struct is_variant<std::variant<Args...>> : std::true_type {};
template<typename T>        constexpr bool is_variant_v = is_variant<T>::value;
