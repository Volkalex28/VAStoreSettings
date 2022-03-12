#pragma once

#include <any>
#include <functional>
#include <string.h>
#include "type_traits.hpp"

class linker;

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

  template<class T>
  linker & operator<<(const T & value)
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
        arr[i] = (linker() << value[i]);

      this->m_value = arr;
    }
    else if constexpr (is_linker_obj_v<std::remove_const_t<T>>)
    {
      this->m_value = value;
    }
    else if constexpr (is_vector_v<T> || is_list_v<T>     || is_forward_list_v<T>
                    || is_set_v<T>    || is_multiset_v<T> || is_unordered_set_v<T>
                    || is_array_v<T>  || is_valarray_v<T> || is_unordered_multiset_v<T>
                    || is_map_v<T>    || is_multimap_v<T> || is_unordered_multimap_v<T>
                    || is_deque_v<T>  || is_unordered_map_v<T>)
    {
      array_t arr(std::distance(std::cbegin(value), std::cend(value)));

      auto inIt = std::cbegin(value);
      for(auto it = std::begin(arr);
          it != std::end(arr) && inIt != std::cend(value);
          it++, inIt++)
      {
        *it = (linker() << *inIt);
      }

      this->m_value = arr;
    }
    else if constexpr (is_pair_v<T>)
    {
      this->m_value = object_t{ { "f", linker() << value.first }, { "s", linker() << value.second } };
    }
    else if constexpr (is_bitset_v<T>)
    {
      array_t arr(value.size());

      for(std::size_t i = 0; i < value.size(); i++)
        arr[i] << value[i];

      this->m_value = arr;
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

      this->m_value = arr;
    }
    else if constexpr (is_complex_v<T>)
    {
      this->m_value = object_t { { "r", linker() << value.real() }, { "i", linker() << value.imag() } };
    }
    else if constexpr (is_tuple_v<T>)
    {
      object_t obj = {};
      this->input_tuple(std::make_index_sequence<std::tuple_size_v<T>>(), obj, value);

      this->m_value = obj;
    }
    else if constexpr (is_variant_v<T>)
    {
      this->input_variant(std::make_index_sequence<std::variant_size_v<T>>(), value);
    }
    else if constexpr (is_linker_v<T>)
    {
      this->m_type = value.m_type;
      this->m_value = value.m_value;
    }

    return *this;
  }

  template<class T>
  T value(void) const
  {
    try
    {
      T retVal;
      return *this >> retVal;
    }
    catch(...)
    {
      return T();
    }
  }

  template<class T>
  T & operator>>(T & retVal) const
  {
    try
    {
      constexpr Types m_type = linker::get_type<T>();
      if (m_type != this->m_type) return retVal;

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
      else if constexpr (is_array_v<T>)
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
        const array_t arr { this->cast<array_t>() };

        T ret;
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

        *const_cast<first*>(&retVal.first)   = obj["f"].value<first>();
        *const_cast<second*>(&retVal.second) = obj["s"].value<second>();
      }
      else if constexpr (is_bitset_v<T>)
      {
        const array_t arr { this->cast<array_t>() };

        for(std::size_t i = 0; i < arr.size() && i < retVal.size(); i++)
          retVal[i] = arr[i].value<bool>();
      }
      else if constexpr (is_queue_v<T> || is_priority_queue_v<T> || is_stack_v<T>)
      {
        const array_t arr { this->cast<array_t>() };

        T ret;
        for(std::size_t i = 0; i < arr.size(); i++)
          ret.push(arr[i].value<typename T::value_type>());

        retVal.swap(ret);
      }
      else if constexpr (is_complex_v<T>)
      {
        object_t obj { this->cast<object_t>() };

        using type = std::complex<std::remove_const_t<typename T::value_type>>;

        const_cast<type*>(&retVal)->real(obj["r"].value<number_t>());
        const_cast<type*>(&retVal)->imag(obj["i"].value<number_t>());
      }
      else if constexpr (is_tuple_v<T>)
      {
        object_t obj { this->cast<object_t>() };
        this->output_tuple(std::make_index_sequence<std::tuple_size_v<T>>(), obj, retVal);
      }
      else if constexpr (is_variant_v<T>)
      {
        this->output_variant(std::make_index_sequence<std::variant_size_v<T>>(), retVal);
      }
      else if constexpr (is_linker_v<T>)
      {
        retVal = this->cast<linker>();
      }
    }
    catch(...) { }

    return retVal;
  }

  inline Types type(void) const { return this->m_type; }

  template<typename T>
  static T value(const linker & lnk)
  {
    return lnk.value<T>();
  }
  template<typename T>
  static linker from(T value)
  {
    return linker() << value;
  }

private:
  template<class T>
  static constexpr Types get_type(void)
  {
    if (std::is_null_pointer_v<T>)                      return Types::Null;
    else if (std::is_same_v<std::decay_t<T>, bool_t>)   return Types::Bool;
    else if (std::is_enum_v<T>)                         return Types::Number;
    else if (std::is_arithmetic_v<T>)                   return Types::Number;
    else if (std::is_convertible_v<T, string_t>)        return Types::String;
    else if (is_linker_obj_v<T>)                        return Types::Object;
    else if (is_linker_arr_v<T>)                        return Types::Array;
    else if (std::is_array_v<T>)                        return Types::Array;
    else if (is_array_v<T>)                             return Types::Array;
    else if (is_bitset_v<T>)                            return Types::Array;
    else if (is_vector_v<T>)                            return Types::Array;
    else if (is_list_v<T>)                              return Types::Array;
    else if (is_forward_list_v<T>)                      return Types::Array;
    else if (is_set_v<T>)                               return Types::Array;
    else if (is_multiset_v<T>)                          return Types::Array;
    else if (is_unordered_set_v<T>)                     return Types::Array;
    else if (is_unordered_multiset_v<T>)                return Types::Array;
    else if (is_valarray_v<T>)                          return Types::Array;
    else if (is_map_v<T>)                               return Types::Array;
    else if (is_multimap_v<T>)                          return Types::Array;
    else if (is_unordered_map_v<T>)                     return Types::Array;
    else if (is_unordered_multimap_v<T>)                return Types::Array;
    else if (is_bitset_v<T>)                            return Types::Array;
    else if (is_deque_v<T>)                             return Types::Array;
    else if (is_queue_v<T>)                             return Types::Array;
    else if (is_priority_queue_v<T>)                    return Types::Array;
    else if (is_stack_v<T>)                             return Types::Array;
    else if (is_pair_v<T>)                              return Types::Object;
    else if (is_complex_v<T>)                           return Types::Object;
    else if (is_tuple_v<T>)                             return Types::Object;
    else if (is_variant_v<T>)                           return Types::Object;
    else                                                return Types::Other;
  }

  template<class T>
  T cast(void) const
  {
    try
    {
      return std::any_cast<T>(this->m_value);
    }
    catch(...)
    {
      return T();
    }
  }

  template<typename T, std::size_t ... I>
  void input_tuple(std::index_sequence<I ...>, object_t & obj, T & value)
  {
    [](auto && ...){}((obj["t" + std::to_string(I)] << std::get<I>(value))...);
  }

  template<typename T, std::size_t ... I>
  void input_variant(std::index_sequence<I ...>, T & value)
  {
    [&](auto && ...){}((I == value.index() ? this->input_variant<T, I>(std::get<I>(value))
                                           : std::nullopt)...);
  }
  template<typename T, std::size_t I, typename U>
  std::optional<std::any> input_variant(U & value)
  {
    this->m_value = object_t
    {
      { "i", linker() << I },
      { "v", linker() << std::any_cast<std::variant_alternative_t<I, T>>(value) }
    };
    return std::nullopt;
  }

  template<typename T, std::size_t ... I>
  void output_tuple(std::index_sequence<I ...>, object_t & obj, T & retVal) const
  {
    [](auto && ...) { }
    (obj["t" + std::to_string(I)] >> *const_cast<get_el_t<I, T> *>(&std::get<I>(retVal))...);
  }

  template<typename T, std::size_t ... I>
  void output_variant(std::index_sequence<I ...>, T & retVal) const
  {
    auto obj = this->cast<object_t>();
    [](auto && ...){}
    ((I == obj["i"].value<std::size_t>() ? output_variant<T, I>(retVal, obj)
                                         : std::nullopt)...);
  }
  template<typename T, std::size_t I>
  std::optional<std::any> output_variant(T & retVal, object_t & obj) const
  {
    retVal = obj["v"].value<std::variant_alternative_t<I, T>>();
    return std::nullopt;
  }

  Types m_type = Types::Other;
  std::any m_value = std::nullopt;

  friend class linkerFile;
};
