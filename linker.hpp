#pragma once

#include <any>
#include <functional>
#include <cstring>
#include <optional>

#include "type_traits.hpp"

class linker;
class Serializer;

// linker
template<typename T>        struct is_linker : std::false_type {};
template<>                  struct is_linker<linker> : std::true_type {};
template<typename T>        constexpr bool is_linker_v = is_linker<T>::value;

// linker_obj
template<typename T, typename U = void>
struct is_linker_obj : std::false_type {};
template<>
struct is_linker_obj<std::map<std::string, linker>> : std::true_type {};
template<typename T>        constexpr bool is_linker_obj_v = is_linker_obj<T>::value;

// linker_arr
template<typename T, typename U = void>
struct is_linker_arr : std::false_type {};
template<>
struct is_linker_arr<std::vector<linker>> : std::true_type {};
template<typename T>
constexpr bool is_linker_arr_v = is_linker_arr<T>::value;

class linker
{
  template<std::size_t I, class T>
  using get_el_t = std::remove_const_t<std::tuple_element_t<I, T>>;

public:
  enum class Types : uint8_t
  {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
    Other
  };

  using null_t   = std::nullptr_t;
  using bool_t   = bool;
  using number_t = long double;
  using string_t = std::string;
  using array_t  = std::vector<linker>;
  using object_t = std::map<std::string, linker>;

  auto operator<<(const Serializer & value) -> linker &;
  auto operator>>(Serializer & retVal) const -> Serializer &;

  template<class T>
  auto operator<<(const T & value) -> linker &
  {
    constexpr Types m_type = linker::get_type<T>();
    this->m_type = m_type;

    if constexpr      (m_type == Types::Null)   this->m_value = null_t();
    else if constexpr (m_type == Types::Bool)   this->m_value = bool_t(value);
    else if constexpr (m_type == Types::Number) this->m_value = number_t(value);
    else if constexpr (m_type == Types::String) this->m_value = string_t(value);
    else if constexpr (std::is_array_v<T>)
    {
      array_t arr(std::extent_v<T>);

      for(std::size_t i = 0; i < arr.size(); i++)
        arr[i] = linker::from(value[i]);

      this->m_value = std::move(arr);
    }
    else if constexpr (is_linker_obj_v<std::remove_const_t<T>>)
    {
      this->m_value = std::move(value);
    }
    else if constexpr (is_vector_v<T> || is_list_v<T>     || is_forward_list_v<T>
                    || is_set_v<T>    || is_multiset_v<T> || is_unordered_set_v<T>
                    || IS_array_v<T>  || is_valarray_v<T> || is_unordered_multiset_v<T>
                    || is_map_v<T>    || is_multimap_v<T> || is_unordered_multimap_v<T>
                    || is_deque_v<T>  || is_unordered_map_v<T>)
    {
      auto inIt = std::cbegin(value);
      array_t arr(std::distance(std::cbegin(value), std::cend(value)));

      for(auto it = std::begin(arr);
          it != std::end(arr) && inIt != std::cend(value);
          it++, inIt++)
      {
        *it = linker::from(*inIt);
      }

      this->m_value = std::move(arr);
    }
    else if constexpr (is_pair_v<T>)
    {
      this->m_value = object_t { { "f", linker::from(value.first) },
                                 { "s", linker::from(value.second) } };
    }
    else if constexpr (is_bitset_v<T>)
    {
      array_t arr(value.size());

      for(std::size_t i = 0; i < value.size(); i++)
        arr[i] << value[i];

      this->m_value = std::move(arr);
    }
    else if constexpr (is_queue_v<T> || is_priority_queue_v<T> || is_stack_v<T>)
    {
      array_t arr(value.size());

      T copy = value;
      if constexpr (is_stack_v<T>)
      {
        for(std::size_t i = value.size(); i > 0; i--)
        {
          arr[i-1] << copy.top();
          copy.pop();
        }
      }
      else
      {
        for(std::size_t i = 0; i < value.size(); i++)
        {
          if constexpr (is_queue_v<T>) arr[i] << copy.front();
          else                         arr[i] << copy.top();
          copy.pop();
        }
      }

      this->m_value = std::move(arr);
    }
    else if constexpr (is_complex_v<T>)
    {
      this->m_value = object_t { { "r", linker::from(value.real()) },
                                 { "i", linker::from(value.imag()) } };
    }
    else if constexpr (is_tuple_v<T>)
    {
      object_t obj = {};

      [&]<std::size_t ... I>(std::index_sequence<I ...>)
      {
          [](auto && ...){}((obj["t" + std::to_string(I)] << std::get<I>(value))...);
      }(std::make_index_sequence<std::tuple_size_v<T>>());

      this->m_value = std::move(obj);
    }
    else if constexpr (is_variant_v<T>)
    {
      [&]<std::size_t ... I>(std::index_sequence<I ...>)
      {
          [&](auto && ...){}((I == value.index() ? [&]<typename V>(V && value)
          {
              this->m_value = object_t { { "i", linker::from(I) },
                                         { "v", linker::from(std::any_cast<V>(value)) } };
              return std::nullopt;
          } (std::get<I>(value)) : std::nullopt)...);
      }(std::make_index_sequence<std::variant_size_v<T>>());
    }
    else if constexpr (is_linker_v<T>)
    {
      this->m_type = value.m_type;
      this->m_value = value.m_value;
    }
    else if(std::is_base_of_v<Serializer, T>)
    {
      this->operator<<(*(Serializer*)&value);
    }

    return *this;
  }

  template<class T>
  [[nodiscard]] auto value() const -> T
  {
    try
    {
      T retVal;
      return *this >> retVal;
    }
    catch(...)
    {
      return {};
    }
  }

  template<class T>
  auto operator>>(T & retVal) const -> T &
  {
    try
    {
      constexpr Types m_type = linker::get_type<T>();
//      if (m_type != this->m_type) return retVal;

      if constexpr      (m_type == Types::Bool)   retVal = this->cast<bool_t>();
      else if constexpr (m_type == Types::Number) retVal = (T)this->cast<number_t>();
      else if constexpr (m_type == Types::String)
      {
        const auto string = this->cast<string_t>();

        if constexpr (std::is_array_v<T>)
        {
          strncpy(retVal, string.c_str(), std::extent_v<T>);
          retVal[std::extent_v<T> - 1] = '\0';
        }
        else
        {
          retVal = this->cast<string_t>().c_str();
        }
      }
      else if constexpr (std::is_array_v<T>)
      {
        const array_t arr { this->cast<array_t>() };

        for(std::size_t i = 0; i < std::extent_v<T> && i < arr.size(); i++)
          arr[i] >> retVal[i];
      }
      else if constexpr (IS_array_v<T>)
      {
        const array_t arr { this->cast<array_t>() };
        T ret;

        for(std::size_t i = 0; i < ret.size() && i < arr.size(); i++)
          arr[i] >> ret[i];

        std::swap(retVal, ret);
      }
      else if constexpr (is_vector_v<T>   || is_list_v<T> || is_forward_list_v<T>
                      || is_valarray_v<T> || is_deque_v<T>)
      {
        const array_t arr { this->cast<array_t>() };
        T ret(arr.size());

        auto retIt = std::begin(ret);
        for(auto it = std::cbegin(arr); it != std::cend(arr); it++, retIt++)
        {
          *it >> *retIt;
        }

        std::swap(retVal, ret);
      }
      else if constexpr (is_linker_obj_v<std::remove_const_t<T>>)
      {
        retVal = this->cast<linker::object_t>();
      }
      else if constexpr (is_set_v<T> || is_multiset_v<T> || is_unordered_set_v<T>
                      || is_unordered_multiset_v<T>      || is_multimap_v<T>
                      || is_map_v<T> || is_unordered_multimap_v<T>
                      || is_unordered_map_v<T>)
      {
        T ret;
        const array_t arr { this->cast<array_t>() };

        for(auto it = std::cbegin(arr); it != std::cend(arr); it++)
        {
          if constexpr (is_map_v<T> || is_multimap_v<T> || is_unordered_map_v<T>
                      || is_unordered_multimap_v<T>)
          {
            using first  = std::remove_const_t<typename T::key_type>;
            using second = std::remove_const_t<typename T::mapped_type>;

            ret.insert(it->value<std::pair<first, second>>());
          }
          else
          {
            ret.insert(it->value<typename T::value_type>());
          }
        }

        std::swap(retVal, ret);
      }
      else if constexpr (is_pair_v<T>)
      {
        object_t obj { this->cast<object_t>() };

        using first  = std::remove_const_t<typename T::first_type>;
        using second = std::remove_const_t<typename T::second_type>;

        retVal.first  = std::move(obj["f"].value<first>());
        retVal.second = std::move(obj["s"].value<second>());
      }
      else if constexpr (is_bitset_v<T>)
      {
        const array_t arr { this->cast<array_t>() };

        for(std::size_t i = 0; i < arr.size() && i < retVal.size(); i++)
          retVal[i] = arr[i].value<bool>();
      }
      else if constexpr (is_queue_v<T> || is_priority_queue_v<T> || is_stack_v<T>)
      {
        T ret;
        const array_t arr { this->cast<array_t>() };

        for(auto & cell : arr)
          ret.push(cell.value<typename T::value_type>());

        retVal.swap(ret);
      }
      else if constexpr (is_complex_v<T>)
      {
        object_t obj { this->cast<object_t>() };

        retVal->real(obj["r"].value<number_t>());
        retVal->imag(obj["i"].value<number_t>());
      }
      else if constexpr (is_tuple_v<T>)
      {
        object_t obj { this->cast<object_t>() };
        [&]<std::size_t ... I>(std::index_sequence<I ...>)
        {
            [](auto && ...) { } (obj["t" + std::to_string(I)] >> std::get<I>(retVal)...);
        }(std::make_index_sequence<std::tuple_size_v<T>>());
      }
      else if constexpr (is_variant_v<T>)
      {
        [&]<std::size_t ... I>(std::index_sequence<I ...>)
        {
            auto obj = this->cast<object_t>();
            [](auto && ...){}((I == obj["i"].value<std::size_t>() ? [&]()
            {
                retVal = obj["v"].value<std::variant_alternative_t<I, T>>();
                return std::nullopt;
            } () : std::nullopt)...);
        }(std::make_index_sequence<std::variant_size_v<T>>());
      }
      else if constexpr (is_linker_v<T>)
      {
        retVal = this->cast<linker>();
      }
      else if(std::is_base_of_v<Serializer, T>)
      {
        this->operator>>(*(Serializer*)&retVal);
      }
    }
    catch(...) { }

    return retVal;
  }

  [[nodiscard]] inline auto type() const -> Types { return this->m_type; }

  template<typename T>
    static inline auto value(const linker & lnk) -> T
  {
    return lnk.value<T>();
  }
  template<typename T>
  static inline auto from(const T & value) -> linker
  {
    return linker() << value;
  }

private:
  template<class T>
  static constexpr auto get_type() -> Types
  {
    if constexpr (std::is_null_pointer_v<T>)
        return Types::Null;
    else if constexpr (std::is_same_v<std::decay_t<T>, bool_t>)
        return Types::Bool;
    else if constexpr (std::is_enum_v<T> || std::is_arithmetic_v<T>)
        return Types::Number;
    else if constexpr (std::is_convertible_v<T, string_t>)
        return Types::String;
    else if constexpr (is_linker_obj_v<T> || is_pair_v<T> || is_complex_v<T>
                    || is_tuple_v<T> || is_variant_v<T> || std::is_base_of_v<Serializer, T>)
        return Types::Object;
    else if constexpr (is_linker_arr_v<T> || std::is_array_v<T> || IS_array_v<T> || is_bitset_v<T>
                    || is_vector_v<T> || is_list_v<T> || is_forward_list_v<T> || is_set_v<T>
                    || is_multiset_v<T> || is_unordered_set_v<T> || is_unordered_multiset_v<T>
                    || is_valarray_v<T> || is_map_v<T> || is_multimap_v<T> || is_unordered_map_v<T>
                    || is_unordered_multimap_v<T> || is_bitset_v<T> || is_deque_v<T> || is_queue_v<T>
                    || is_priority_queue_v<T> || is_stack_v<T>)
        return Types::Array;
    else return Types::Other;
  }

  template<class T>
  [[nodiscard]] auto cast() const -> T
  {
    try
    {
      return std::any_cast<T>(this->m_value);
    }
    catch(...)
    {
      return {};
    }
  }

  Types m_type = Types::Other;
  std::any m_value = std::nullopt;

  friend class linkerFile;
};

void operator>>(const linker::object_t & map, Serializer * object);
