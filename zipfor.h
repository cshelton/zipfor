#ifndef ZIPFOR_H
#define ZIPFOR_H
/* By Christianbb R. Shelton
 * (christian.r.shelton@gmail.com)
 * December 2013
 *
 * Released under MIT software licence:
 * The MIT License (MIT)
 * Copyright (c) 2013 Christian R. Shelton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* zipfor:
 * useage:
 *
 * const vector<int> x{1,2,3,4,5};
 * float f[5] = {1.5,2.5,3.5,4.5,5.5};
 * map<int,bool> m {{-5,true},{-2,false},{-9,true},{4,false},{2,true}};
 *
 * zipfor(a,b,c eachin x,f,m) {
 *   // iterates over x, f, and m in parallel
 *   // in here a is a (const) reference to member of x
 *   //         b is a reference to member of f
 *   //         c is a reference to member of m (and therefore a pair)
 *   if (c.second) cout << a+b << endl;
 *   else cout << a+b+c.first << endl;
 * }
 * // can be used with or without { } (as a normal for loop)
 *
 * Note:  For speed purposes, this only checks the first one's iterator
 *  to detemine when to end the loop!  
 *  
 *  This is really designed only for zipping containers with exactly the
 *  same number of elements.  Although, it would work fine if you know
 *  the *first* listed container is the shortest.
 *
 * Speed tests (on gcc-4.8.1 with -O3) has this taking the same time as
 * keeping the iterators explicitly or (for vectors, arrays, and the like)
 * keeping a single index (unsigned int).
 *
 *
 * mapfor:
 * useage:
 *
 * map<int,string> m{{3,"hello"},{2,"good-bye"},{-4,"another string"}};
 * mapfor(i,s eachin m)
 *     // in here, i will be a reference to the first element in the pair
 *     // and s will be a reference to the second element in the pair
 *     cout << i << ": " << s << endl;
 * // can also be used with or without {}
 *
 * also works with unordered_map (or any container that stores pairs)
 */

#include <tuple>
#include <utility>
#include <type_traits>
#include <cstdlib>
#include <iterator>

namespace internal {
// below from KennyTM on stackexchange:
// http://stackoverflow.com/questions/8569567/get-part-of-stdtuple
template <std::size_t... n>
struct ct_int_list {
    template <std::size_t m>
    struct push_back {
        using type = ct_int_list<n..., m>;
    };
};

template <std::size_t max>
struct ct_iota {
    using type=typename ct_iota<max-1>::type::template push_back<max>::type;
};

template <>
struct ct_iota<0> {
    using type=ct_int_list<>;
};
}

//------

template<typename ...Is>
class zipittT : public std::tuple<Is...> {
public:
	using BaseT = std::tuple<Is...>;

	template<typename ...Ts>
	zipittT(Ts &&...inits) : BaseT(std::forward<Ts>(inits)...) {
	}

	zipittT(BaseT &&bt) : BaseT(std::move(bt)) { }
	zipittT(const BaseT &bt) : BaseT(bt) { }

	// only check first one -- assume the others will/won't match
	inline bool operator==(const BaseT &bt) const {
		return std::get<0>(*this) == std::get<0>(bt);
	}
	inline bool operator!=(const BaseT &bt) const {
		return !(*this == bt);
	}


private:
	template<std::size_t I = 0>
	static inline
	typename std::enable_if<I == std::tuple_size<BaseT>::value>::type
	inc(BaseT &itt) {
	}

	template<std::size_t I = 0>
	static inline
	typename std::enable_if<I < std::tuple_size<BaseT>::value>::type
	inc(BaseT &itt) {
		++(std::get<I>(itt));
		inc<I+1>(itt);
	}

	template <std::size_t... indices>
	static inline auto
	deref_subset(const BaseT &tpl, internal::ct_int_list<indices...>)
	    -> decltype(std::tie(*std::get<indices-1>(tpl)...)) {
	    return std::tie(*std::get<indices-1>(tpl)...);
	}

public:
	inline zipittT &operator++() { inc(*this); return *this; }
	inline zipittT operator++(int) { zipittT ret(*this); ++(*this); return ret; }

	inline auto operator*() const
	-> decltype(deref_subset(*this,
			typename internal::ct_iota<std::tuple_size<BaseT>::value>::type())) {
		return deref_subset(*this,
			typename internal::ct_iota<std::tuple_size<BaseT>::value>::type());
	}
};

template<typename... Ts>
class ziplistT : private std::tuple<Ts &...> {
public:
	using BaseT = std::tuple<Ts...>;
	template<std::size_t I>
	using ittcompT = decltype(std::begin(std::get<I-1>
								(std::declval<BaseT>())));
private:
	template<std::size_t... indices>
	static inline auto
	begin_subset(const BaseT &tpl, internal::ct_int_list<indices...>)
	 -> zipittT<ittcompT<indices>...> {
	  return zipittT<ittcompT<indices>...>
			(std::begin(std::get<indices-1>(tpl))...);
	}

	template<std::size_t... indices>
	static inline auto
	end_subset(const BaseT &tpl, internal::ct_int_list<indices...>)
	 -> zipittT<ittcompT<indices>...> {
	  return zipittT<ittcompT<indices>...>
			(std::end(std::get<indices-1>(tpl))...);
	}


public:
	template<typename... Xs>
	ziplistT(Xs &&...xs) : BaseT(xs...) { }

	auto begin() const
	-> decltype (begin_subset(*this,
	    typename internal::ct_iota<std::tuple_size<BaseT>::value>::type())) {
		return begin_subset(*this,
		typename internal::ct_iota<std::tuple_size<BaseT>::value>::type());
	}

	auto end() const 
	-> decltype (end_subset(*this,
	    typename internal::ct_iota<std::tuple_size<BaseT>::value>::type())) {
		return end_subset(*this,
		typename internal::ct_iota<std::tuple_size<BaseT>::value>::type());
	}
};

template<typename... Ts>
ziplistT<Ts...> ziplist(Ts &&...ts) {
	return ziplistT<Ts...>(ts...);
}

#define eachin ,

#define _zipforaddvar(V,I,BN) \
	if (bool __zipforbool = false) {} \
	else for(auto &V = std::get<I>(BN); \
				!__zipforbool ; __zipforbool =true) \

#define zipfor2help(V1,V2,...) \
	for(auto __zipfor : ziplist(__VA_ARGS__))  \
		_zipforaddvar(V1,0,__zipfor) \
		_zipforaddvar(V2,1,__zipfor)

#define zipfor3help(V1,V2,V3,...) \
	zipfor2help(V1,V2,__VA_ARGS__) _zipforaddvar(V3,2,__zipfor)
#define zipfor4help(V1,V2,V3,V4,...) \
	zipfor3help(V1,V2,V3,__VA_ARGS__) _zipforaddvar(V4,3,__zipfor)
#define zipfor5help(V1,V2,V3,V4,V5,...) \
	zipfor4help(V1,V2,V3,V4,__VA_ARGS__) _zipforaddvar(V5,4,__zipfor)
#define zipfor6help(V1,V2,V3,V4,V5,V6,...) \
	zipfor5help(V1,V2,V3,V4,V5,__VA_ARGS__) _zipforaddvar(V6,5,__zipfor)
#define zipfor7help(V1,V2,V3,V4,V5,V6,V7,...) \
	zipfor6help(V1,V2,V3,V4,V5,V6,__VA_ARGS__) _zipforaddvar(V7,6,__zipfor)
#define zipfor8help(V1,V2,V3,V4,V5,V6,V7,V8,...) \
	zipfor7help(V1,V2,V3,V4,V5,V6,V7,__VA_ARGS__) \
	_zipforaddvar(V8,7,__zipfor)
#define zipfor9help(V1,V2,V3,V4,V5,V6,V7,V8,V9,...) \
	zipfor8help(V1,V2,V3,V4,V5,V6,V7,V8,__VA_ARGS__) \
	_zipforaddvar(V9,8,__zipfor)

#define zipfor2(...) zipfor2help(__VA_ARGS__)
#define zipfor3(...) zipfor3help(__VA_ARGS__)
#define zipfor4(...) zipfor4help(__VA_ARGS__)
#define zipfor5(...) zipfor5help(__VA_ARGS__)
#define zipfor6(...) zipfor6help(__VA_ARGS__)
#define zipfor7(...) zipfor7help(__VA_ARGS__)
#define zipfor8(...) zipfor8help(__VA_ARGS__)
#define zipfor9(...) zipfor9help(__VA_ARGS__)

#define zipforwrong(...) static_assert(false,"wrong zipfor format");
#define zipfortoomany(...) static_assert(false,"too many zipfor arguments");
#define zipfortoofew(...) static_assert(false,"too few zipfor arguments");

/*
 * Laurent Deniau, "__VA_NARG__," 17 January 2006, <comp.std.c>
 * (29 November 2007).
 * https://groups.google.com/forum/?fromgroups=#!topic/comp.std.c/d-6Mj5Lko_s
 *
 * (modified for above application)
 */

#define zipfor_PP_ARG_N( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
	_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
	_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
	_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
	_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
	_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
	_61,_62,_63,N,...) N

#define zipfor_PP_RSEQ_N() \
	toomany,toomany,toomany,toomany,toomany,toomany,toomany,toomany,\
	toomany,toomany,toomany,toomany,toomany,toomany,toomany,toomany,\
	toomany,toomany,toomany,toomany,toomany,toomany,toomany,toomany,\
	toomany,toomany,toomany,toomany,toomany,toomany,toomany,toomany,\
	toomany,toomany,toomany,toomany,toomany,toomany,toomany,toomany,\
	toomany,toomany,toomany,toomany,toomany,\
	9,wrong,8,wrong,7,wrong,6,wrong,\
	5,wrong,4,wrong,3,wrong,2,toofew,toofew,toofew,toofew

#define zipfor_CPPX_INVOKE( macro, args )  macro args
#if 0
// original defn:
	#define zipfor_PP_NARG_(...) zipfor_PP_ARG_N( __VA_ARGS__ )
	#define zipfor_PP_NARG( ...) \
		zipfor_PP_NARG_( __VA_ARGS__, zipfor_PP_RSEQ_N() )
#else
// according to "Alf"
// (http://stackoverflow.com/questions/15847837/variadac-macro-apply-macro-to-all-arguments)
// below silliness necessary for MSVC bug
// (I have not confirmed it -- cshelton)
	#define zipfor_PP_NARG_(...) \
		zipfor_CPPX_INVOKE( zipfor_PP_ARG_N, (__VA_ARGS__) )
	#define zipfor_PP_NARG(...) \
		zipfor_PP_NARG_(__VA_ARGS__,zipfor_PP_RSEQ_N())
#endif

#define zipfor_concat__(a,b) a ## b
#define zipfor_concat_(a,b) zipfor_concat__(a,b)
#define zipfor_concat(a,b) zipfor_concat_(a,b)
#define zipfor(...) \
	zipfor_CPPX_INVOKE(zipfor_concat(zipfor,zipfor_PP_NARG(__VA_ARGS__)),\
			(__VA_ARGS__))

#define mapforhelp(V1,V2,...) \
	for(auto __mapfor : __VA_ARGS__)  \
		_zipforaddvar(V1,0,__mapfor) \
		_zipforaddvar(V2,1,__mapfor)

#define mapfor(...) mapforhelp(__VA_ARGS__)

#endif
