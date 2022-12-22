//
//  shared_ptr.h
//  zmz_shared_ptr
//
//  Created by mingze.zou on 2022/11/10.
//

#include <atomic>
#include "unique_ptr.h"
//using namespace std;

#pragma mark - 控制块

//原子操作 加
template <class _Tp>
inline _Tp
__libcpp_atomic_refcount_increment(_Tp& __t) _NOEXCEPT
{
    //gcc提供的atomic函数族
    return __atomic_add_fetch(&__t, 1, __ATOMIC_RELAXED);
}

//减
template <class _Tp>
inline _Tp
__libcpp_atomic_refcount_decrement(_Tp& __t) _NOEXCEPT
{
    return __atomic_add_fetch(&__t, -1, __ATOMIC_ACQ_REL);
}

//gcc原子操作函数族，取指针的值
template <class _ValueType>
inline _ValueType
__libcpp_relaxed_load(_ValueType const* __value) {
    //返回指针的值
    return __atomic_load_n(__value, __ATOMIC_RELAXED);
}


class __shared_count
{
    //没有具体实现，应该是默认生成了
    __shared_count(const __shared_count&);
    __shared_count& operator=(const __shared_count&);
    
protected:
    long __shared_owners_;//引用计数，比实际小1，刚创建时为0;
    virtual ~__shared_count() = default;
    
private:
    //纯虚函数，调用析构
    virtual void __on_zero_shared() _NOEXCEPT = 0;
    
public:
    explicit __shared_count(long __refs = 0) _NOEXCEPT
            : __shared_owners_(__refs) {}
    
    void __add_shared() _NOEXCEPT {
          ::__libcpp_atomic_refcount_increment(__shared_owners_);
        }
    
    bool __release_shared() _NOEXCEPT {
          if (::__libcpp_atomic_refcount_decrement(__shared_owners_) == -1) {
            __on_zero_shared();
            return true;
          }
          return false;
        }
    
    long use_count() const _NOEXCEPT {
            return ::__libcpp_relaxed_load(&__shared_owners_) + 1;
        }
};


class __shared_weak_count
    : private ::__shared_count
{
    long __shared_weak_owners_;

public:
    explicit __shared_weak_count(long __refs = 0) _NOEXCEPT
        : __shared_count(__refs),
          __shared_weak_owners_(__refs) {}
protected:
    virtual ~__shared_weak_count() = default;

public:
    
    void __add_shared() _NOEXCEPT {
      __shared_count::__add_shared();
    }
    
    void __add_weak() _NOEXCEPT {
      ::__libcpp_atomic_refcount_increment(__shared_weak_owners_);
    }
    
    void __release_shared() _NOEXCEPT {
      if (__shared_count::__release_shared())
        __release_weak();
    }
    
    

    void __release_weak() _NOEXCEPT;
    
    long use_count() const _NOEXCEPT {return __shared_count::use_count();}
    __shared_weak_count* lock() _NOEXCEPT;

    virtual const void* __get_deleter(const std::type_info&) const _NOEXCEPT = 0;
private:
    virtual void __on_zero_shared_weak() _NOEXCEPT = 0;
};

//普通构造时使用的控制块
template <class _Tp, class _Dp>
class __shared_ptr_pointer
    : public ::__shared_weak_count
{
    std::pair<_Tp, _Dp> __data_;
    
public:
    
    
    __shared_ptr_pointer(_Tp __p, _Dp __d)
        :  __data_(std::pair<_Tp, _Dp>(__p, _VSTD::move(__d))) {}

#ifndef _LIBCPP_NO_RTTI
    virtual const void* __get_deleter(const std::type_info&) const _NOEXCEPT;
#endif

private:
    virtual void __on_zero_shared() _NOEXCEPT;
    virtual void __on_zero_shared_weak() _NOEXCEPT;
};


template <class _Tp, class _Dp>
const void*
::__shared_ptr_pointer<_Tp, _Dp>::__get_deleter(const std::type_info& __t) const _NOEXCEPT
{
    return __t == typeid(_Dp) ? _VSTD::addressof(__data_.second) : nullptr;
}

template <class _Tp, class _Dp>
void
::__shared_ptr_pointer<_Tp, _Dp>::__on_zero_shared() _NOEXCEPT
{
    __data_.second(__data_.first);
    __data_.second.~_Dp();
}

template <class _Tp, class _Dp>
void
::__shared_ptr_pointer<_Tp, _Dp>::__on_zero_shared_weak() _NOEXCEPT
{
//    typedef typename __allocator_traits_rebind<_Alloc, __shared_ptr_pointer>::type _Al;
//    typedef allocator_traits<_Al> _ATraits;
//    typedef pointer_traits<typename _ATraits::pointer> _PTraits;
//
//    _Al __a(__data_.second());
//    __data_.second().~_Alloc();
//    __a.deallocate(_PTraits::pointer_to(*this), 1);
}



#pragma mark - shared_ptr

template<class _Tp>
class zmz_shared_ptr
{
private:
    _Tp*                   __ptr_;
    __shared_weak_count* __cntrl_;

public:
    
#pragma mark 构造和析构
    
    zmz_shared_ptr() _NOEXCEPT;
    
    constexpr zmz_shared_ptr(nullptr_t) _NOEXCEPT;

    template<class _Yp>
    explicit zmz_shared_ptr(_Yp* __p) : __ptr_(__p) {
        typedef __shared_ptr_default_delete<_Tp, _Yp> _Delete;
        __cntrl_ = new ::__shared_ptr_pointer<_Yp*, _Delete>(__p, _Delete());
        //TODO：看不懂，为什么要先用一个unique去hold然后再释放；__enable_weak_this的作用是？
//        unique_ptr<_Yp> __hold(__p);
//        typedef typename __shared_ptr_default_allocator<_Yp>::type _AllocT;
//        typedef __shared_ptr_pointer<_Yp*, __shared_ptr_default_delete<_Tp, _Yp>, _AllocT > _CntrlBlk;
//        __cntrl_ = new _CntrlBlk(__p, __shared_ptr_default_delete<_Tp, _Yp>(), _AllocT());
//        __hold.release();
//        __enable_weak_this(__p, __p);
    }

    template<class _Yp, class _Dp>
    zmz_shared_ptr(_Yp* __p, _Dp __d) : __ptr_(__p) {
        try {
            typedef __shared_ptr_pointer<_Yp*, _Dp> _CntrlBlk;
            __cntrl_ = new _CntrlBlk(__p, _VSTD::move(__d));
        }
        catch (...)
        {
            __d(__p);
            throw;
        }
    }
    
    template <class _Dp> zmz_shared_ptr(nullptr_t __p, _Dp __d) : __ptr_(nullptr)
    {
        try
        {
            typedef __shared_ptr_pointer<nullptr_t, _Dp> _CntrlBlk;
            __cntrl_ = new _CntrlBlk(__p, std::move(__d));
        }
        catch (...)
        {
            __d(__p);
            throw;
        }
    }

    //别名构造函数，__p是__r管理的对象的成员，或者是r.get()的别名
    template<class _Yp>
    inline
    zmz_shared_ptr(const zmz_shared_ptr<_Yp>& __r, _Tp* __p) _NOEXCEPT
        : __ptr_(__p),
          __cntrl_(__r.__cntrl_)
    {
        if (__cntrl_)
            __cntrl_->__add_shared();
    }
    
    //拷贝构造函数1
    inline
    zmz_shared_ptr(const zmz_shared_ptr& __r) _NOEXCEPT
        : __ptr_(__r.__ptr_),
          __cntrl_(__r.__cntrl_)
    {
        if (__cntrl_)
            __cntrl_->__add_shared();
    }
    
    //拷贝构造函数2
    template<class _Yp>
    inline
    zmz_shared_ptr(const zmz_shared_ptr<_Yp>& __r) _NOEXCEPT
        : __ptr_(__r.__ptr_),
          __cntrl_(__r.__cntrl_)
    {
        if (__cntrl_)
            __cntrl_->__add_shared();
    }
    
    //移动构造函数1
    inline
    zmz_shared_ptr(zmz_shared_ptr&& __r) _NOEXCEPT
        : __ptr_(__r.__ptr_),
          __cntrl_(__r.__cntrl_)
    {
        __r.__ptr_ = nullptr;
        __r.__cntrl_ = nullptr;
    }
    
    //移动构造函数2
    template<class _Yp>
    inline
    zmz_shared_ptr(zmz_shared_ptr<_Yp>&& __r) _NOEXCEPT
        : __ptr_(__r.__ptr_),
          __cntrl_(__r.__cntrl_)
    {
        __r.__ptr_ = nullptr;
        __r.__cntrl_ = nullptr;
    }
    

    template<class _Yp>
    explicit zmz_shared_ptr(const zmz_weak_ptr<_Yp>& __r)
        : __ptr_(__r.__ptr_),
          __cntrl_(__r.__cntrl_ ? __r.__cntrl_->lock() : __r.__cntrl_)
    {
        if (__cntrl_ == nullptr)
            __throw_bad_weak_ptr();
    }
    
    //unique_ptr只能移动，不能拷贝
    template <class _Yp, class _Dp>
    zmz_shared_ptr(zmz_unique_ptr<_Yp, _Dp>&& __r)
        : __ptr_(__r.get())
    {
        if (__ptr_ == nullptr)
            __cntrl_ = nullptr;
        else
        {
            typedef __shared_ptr_pointer<typename zmz_unique_ptr<_Yp, _Dp>::pointer, _Dp> _CntrlBlk;
            __cntrl_ = new _CntrlBlk(__r.get(), __r.get_deleter());
            //__enable_weak_this(__r.get(), __r.get());
        }
        __r.release();
    }
    

    ~zmz_shared_ptr() {
        if (__cntrl_)
            __cntrl_->__release_shared();
    }
    
#pragma mark 赋值符号=重载

    inline
    zmz_shared_ptr&
    operator=(const zmz_shared_ptr& __r) _NOEXCEPT
    {
        zmz_shared_ptr(__r).swap(*this);
        return *this;
    }
    
    
    template<class _Yp>
    inline
    zmz_shared_ptr&
    operator=(const zmz_shared_ptr<_Yp>& __r) _NOEXCEPT
    {
        zmz_shared_ptr(__r).swap(*this);
        return *this;
    }
    

    inline
    zmz_shared_ptr&
    operator=(zmz_shared_ptr&& __r) _NOEXCEPT
    {
        zmz_shared_ptr(_VSTD::move(__r)).swap(*this);
        return *this;
    }
    

    template<class _Yp>
    inline
    zmz_shared_ptr&
    operator=(zmz_shared_ptr<_Yp>&& __r)
    {
        zmz_shared_ptr(_VSTD::move(__r)).swap(*this);
        return *this;
    }
    
    
    
    template <class _Yp, class _Dp>
    inline
    zmz_shared_ptr&
    operator=(zmz_unique_ptr<_Yp, _Dp>&& __r)
    {
        zmz_shared_ptr(_VSTD::move(__r)).swap(*this);
        return *this;
    }
    
#pragma mark 其他公开方法
    

    inline
    void
    swap(zmz_shared_ptr& __r) _NOEXCEPT
    {
        _VSTD::swap(__ptr_, __r.__ptr_);
        _VSTD::swap(__cntrl_, __r.__cntrl_);
    }
    
    
    
    inline
    void
    reset() _NOEXCEPT
    {
        zmz_shared_ptr().swap(*this);
    }
    
    

    
    template<class _Yp>
    inline
    void
    reset(_Yp* __p)
    {
        zmz_shared_ptr(__p).swap(*this);
    }
    
    
    template<class _Yp, class _Dp>
    inline
    void
    reset(_Yp* __p, _Dp __d)
    {
        zmz_shared_ptr(__p, __d).swap(*this);
    }
    


    inline
    _Tp* get() const _NOEXCEPT {return __ptr_;}
    
    
    
    
    inline
    _Tp operator*() const _NOEXCEPT
        {return *__ptr_;}
    
    
    inline
    _Tp* operator->() const _NOEXCEPT
    {
        static_assert(!std::is_array<_Tp>::value,
                      "std::shared_ptr<T>::operator-> is only valid when T is not an array type.");
        return __ptr_;
    }
    
    inline
    long use_count() const _NOEXCEPT
    {
        return __cntrl_ ? __cntrl_->use_count() : 0;
    }

    //判断是否能转为unique
    inline
    bool unique() const _NOEXCEPT {return use_count() == 1;}
        
    //允许zmz_shared_ptr类的对象转换为bool类型
    inline
    explicit operator bool() const _NOEXCEPT {return get() != nullptr;}
        
        
//    template <class _Up>
//        _LIBCPP_INLINE_VISIBILITY
//        bool owner_before(shared_ptr<_Up> const& __p) const _NOEXCEPT
//        {return __cntrl_ < __p.__cntrl_;}
//    template <class _Up>
//        _LIBCPP_INLINE_VISIBILITY
//        bool owner_before(weak_ptr<_Up> const& __p) const _NOEXCEPT
//        {return __cntrl_ < __p.__cntrl_;}
//    _LIBCPP_INLINE_VISIBILITY
//    bool
//    __owner_equivalent(const shared_ptr& __p) const
//        {return __cntrl_ == __p.__cntrl_;}
//
//#if _LIBCPP_STD_VER > 14
//    typename add_lvalue_reference<element_type>::type
//    _LIBCPP_INLINE_VISIBILITY
//    operator[](ptrdiff_t __i) const
//    {
//            static_assert(is_array<_Tp>::value,
//                          "std::shared_ptr<T>::operator[] is only valid when T is an array type.");
//            return __ptr_[__i];
//    }
//#endif
//
//#ifndef _LIBCPP_NO_RTTI
//    template <class _Dp>
//        _LIBCPP_INLINE_VISIBILITY
//        _Dp* __get_deleter() const _NOEXCEPT
//            {return static_cast<_Dp*>(__cntrl_
//                    ? const_cast<void *>(__cntrl_->__get_deleter(typeid(_Dp)))
//                      : nullptr);}
//#endif // _LIBCPP_NO_RTTI
//
//    template<class _Yp, class _CntrlBlk>
//    static shared_ptr<_Tp>
//    __create_with_control_block(_Yp* __p, _CntrlBlk* __cntrl) _NOEXCEPT
//    {
//        shared_ptr<_Tp> __r;
//        __r.__ptr_ = __p;
//        __r.__cntrl_ = __cntrl;
//        __r.__enable_weak_this(__r.__ptr_, __r.__ptr_);
//        return __r;
//    }
//
 
    
    
    
    template <class, class _Yp>
        struct __shared_ptr_default_delete
            : ::default_delete<_Yp> {};

    template <class _Yp, class _Un, size_t _Sz>
        struct __shared_ptr_default_delete<_Yp[_Sz], _Un>
            : ::default_delete<_Yp[]> {};

    template <class _Yp, class _Un>
        struct __shared_ptr_default_delete<_Yp[], _Un>
            : ::default_delete<_Yp[]> {};

};
