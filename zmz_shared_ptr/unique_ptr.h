//
//  unique_ptr.h
//  zmz_shared_ptr
//
//  Created by mingze.zou on 2022/11/17.
//

//using namespace std;

#pragma mark - 默认删除器

//template <class _Tp>
//struct default_delete {
//
//    static_assert(!is_function<_Tp>::value,
//                  "default_delete cannot be instantiated for function types");
//
//    constexpr default_delete() _NOEXCEPT = default;
//
//    template <class _Up>
//    default_delete(const default_delete<_Up>&,
//                   typename enable_if<is_convertible<_Up*, _Tp*>::value>::type* = 0) _NOEXCEPT {}
//
//    void operator()(_Tp* __ptr) const _NOEXCEPT {
//        static_assert(sizeof(_Tp) > 0,
//                      "default_delete can not delete incomplete type");
//        static_assert(!is_void<_Tp>::value,
//                      "default_delete can not delete incomplete type");
//        delete __ptr;
//    }
//};
//
//template <class _Tp>
//struct ::default_delete<_Tp[]> {
//private:
//    template <class _Up>
//    struct _EnableIfConvertible
//    : enable_if<is_convertible<_Up(*)[], _Tp(*)[]>::value> {};
//
//public:
//    constexpr default_delete() _NOEXCEPT = default;
//
//    template <class _Up>
//    default_delete(const default_delete<_Up[]>&,
//                   typename _EnableIfConvertible<_Up>::type* = 0) _NOEXCEPT {}
//
//    template <class _Up>
//    typename _EnableIfConvertible<_Up>::type
//    operator()(_Up* __ptr) const _NOEXCEPT {
//        static_assert(sizeof(_Tp) > 0,
//                      "default_delete can not delete incomplete type");
//        static_assert(!is_void<_Tp>::value,
//                      "default_delete can not delete void type");
//        delete[] __ptr;
//    }
//};

template <class _Tp>
struct default_delete {
    constexpr default_delete() _NOEXCEPT = default;
    
    void operator()(_Tp* __ptr) const _NOEXCEPT {
        delete __ptr;
    }
};

template <class _Tp>
struct default_delete<_Tp[]> {
public:
    constexpr default_delete() _NOEXCEPT = default;
    
    template <class _Up>
    void operator()(_Up* __ptr) const _NOEXCEPT {
        delete[] __ptr;
    }
};

#pragma mark - unique_ptr

template <class _Tp, class _Dp = default_delete<_Tp> >
class zmz_unique_ptr {
public:
    //类型_Dp不能是右值
    static_assert(!std::is_rvalue_reference<_Dp>::value,
                  "the specified deleter type cannot be an rvalue reference");
    
private:
    _Tp* __ptr_;
    _Dp  __deleter_;
    
public:
    zmz_unique_ptr() _NOEXCEPT;
    
    zmz_unique_ptr(nullptr_t) _NOEXCEPT {}
    
    template<class _Yp>
    explicit zmz_unique_ptr(_Yp* __p) _NOEXCEPT : __ptr_(__p), __deleter_(_Dp()) {}
    
    template<class _Yp, class _Ep>
    zmz_unique_ptr(_Yp* __p, _Ep __d) _NOEXCEPT
    : __ptr_(__p), __deleter_(__d) {}

    //移动构造
    template <class _Up, class _Ep>
    zmz_unique_ptr(zmz_unique_ptr<_Up, _Ep>&& __u) _NOEXCEPT
    : __ptr_(__u.release()), __deleter_(_VSTD::forward<_Ep>(__u.get_deleter())) {}
    
    //移动赋值运算符重载
    zmz_unique_ptr& operator=(zmz_unique_ptr&& __u) _NOEXCEPT {
        reset(__u.release());
        __deleter_ = _VSTD::forward<_Dp>(__u.get_deleter());
        return *this;
    }

    //移动赋值运算符重载
    template <class _Up, class _Ep>
    zmz_unique_ptr& operator=(zmz_unique_ptr<_Up, _Ep>&& __u) _NOEXCEPT {
        reset(__u.release());
        __deleter_ = _VSTD::forward<_Ep>(__u.get_deleter());
        return *this;
    }

    //拷贝构造和拷贝赋值 禁用
    zmz_unique_ptr(zmz_unique_ptr const&) = delete;
    zmz_unique_ptr& operator=(zmz_unique_ptr const&) = delete;

    ~zmz_unique_ptr() { reset(); }

    zmz_unique_ptr& operator=(nullptr_t) _NOEXCEPT {
        reset();
        return *this;
    }

    _Tp operator*() const {
        return *__ptr_;
    }
    
    _Tp* operator->() const _NOEXCEPT {
        return __ptr_;
    }
    
    _Tp* get() const _NOEXCEPT {
        return __ptr_;
    }
    
    _Dp& get_deleter() _NOEXCEPT {
        return __deleter_;
    }
    
    const _Dp& get_deleter() const _NOEXCEPT {
        return __deleter_;
    }
    
    explicit operator bool() const _NOEXCEPT {
        return __ptr_ != nullptr;
    }

    _Tp* release() _NOEXCEPT {
        _Tp* __t = __ptr_;
        __ptr_ = nullptr;
        return __t;
    }

    void reset(_Tp* __p = nullptr) _NOEXCEPT {
        _Tp* __tmp = __ptr_;
        __ptr_ = __p;
        if (__tmp)
            __deleter_(__tmp);
    }

    void swap(zmz_unique_ptr& __u) _NOEXCEPT {
        swap(__ptr_, __u.__ptr_);
    }
};
