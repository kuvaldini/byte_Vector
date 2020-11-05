#pragma once

#include <string>
#include <type_traits>
#include <iostream>
//#include "byte_vector.hpp"  //todo rm, should to depend

struct byte_vector_view {
    using iterator = const unsigned char*;
    using siterator = const signed char*;
    iterator first, last=first;
    bool error_flag_=false;
    byte_vector_view(byte_vector_view const& bvv):first(bvv.first),last(bvv.last),error_flag_(bvv.error_flag()){}
    byte_vector_view(byte_vector_view     && bvv):byte_vector_view(bvv){}
    byte_vector_view(iterator beg, size_t len): first(beg),last(beg+len){}
    byte_vector_view(iterator beg, iterator end): first(beg),last(end){}
    byte_vector_view(siterator beg, size_t len): first((iterator)beg),last((iterator)beg+len){}
    //byte_vector_view(byte_vector const& bv):first(bv.data()),last(bv.data()+bv.size()){}
    //template<typename C> byte_vector_view(C const& c): byte_vector_view(c.begin(), c.end()){}
    template<typename C> byte_vector_view(C const& c): first((iterator)c.data()), last((iterator)c.data()+c.size()) {}
    template<size_t N> byte_vector_view(char const (&c)[N]): first((iterator)c), last((iterator)(c+N-1)) {}
    iterator begin()const{return first;}
    iterator end()const{return last;}
    size_t size()const{return std::distance(first,last);}
    size_t empty()const{return size()==0;}
    void clear(){ first=last; }
    operator std::pair<iterator,iterator>(){
        return {first,last};
    }
    /// Calling front on an empty container is undefined.
    uint8_t front() const{
        return *first;
    }
    /// Calling back on an empty container is undefined.
    uint8_t back()const{
        return *(last-1);
    }
    uint8_t const* data()const{ return first; }
    operator bool()const{ return good(); }
    bool good()const{ return not error_flag_; }
    bool bad()const{ return error_flag_; }
    bool error_flag()const{ return error_flag_; }
    bool& error_flag(){ return error_flag_; }
};
//template<typename Container> byte_vector_view(Container const&) -> byte_vector_view<decltype(c.begin())>; //deduction guide

/// to be able for rvalues byte_vector_view, returns a copy
template<class T>
inline byte_vector_view operator>>(byte_vector_view&&bvv, T&v){
    return bvv >> v;
}

template<typename T>
inline static auto operator>>(byte_vector_view&bvv, T&val)
    ->std::enable_if_t< std::is_integral_v<T>,  byte_vector_view& >
{
    using namespace std;
    if(bvv.size() < sizeof(T)){
        cerr<<"err in "<<__func__<<"(): input stream is too short (size:"<<bvv.size()
            <<", requested:"<< sizeof(T)<<")"<<endl;
        bvv.error_flag() = true;
        return bvv;
    }
 #if BYTE_ORDER == LITTLE_ENDIAN
    std::reverse_iterator<uint8_t*> out_it{(uint8_t*)(&val+1)}, out_end{(uint8_t*)(&val)};
 #else
    uint8_t* out_it{(uint8_t*)(&val)}, *out_end{(uint8_t*)(&val+1)};
 #endif
    while (out_it != out_end && bvv.first != bvv.last) {
        *out_it++ = *bvv.first++;
    }
    return bvv;
}

inline static byte_vector_view& operator>>(byte_vector_view&bvv, std::string&str){
    uint16_t /* byte_vector::strsize_t */ strsize;
    bvv>>strsize;
    if(bvv.bad() or strsize > bvv.size())
        return bvv;
    str = std::string{(char*)&*bvv.begin(),strsize};
    bvv.first+=strsize;
    return bvv;
}

inline static byte_vector_view& operator>>(byte_vector_view&bvv, uint8_t&ui8){
    ui8 = *bvv.first++;
    return bvv;
}

#include <tuple>
#include "local_type_traits.hpp"

template<typename TupleT>
inline static auto operator>>(byte_vector_view&bvv, TupleT&tu) 
->std::enable_if_t<is_tuple<TupleT>,byte_vector_view&> 
{
    apply(
        [&](auto&...elements) {
            ((bvv>>elements),...);
        }, tu
    );
    return bvv;
}


//////////////////////////
template <typename UnknownType>
struct Type_Not_Supported : std::false_type {};

/// A fallback function template to stop compiler try different combinations of types, reduce compiler output.
template<typename UnknownType>
inline static auto operator>>(byte_vector_view&bvv, UnknownType&&) ->byte_vector_view& {
    static_assert(Type_Not_Supported<UnknownType>{});
    return bvv;
}
