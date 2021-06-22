/*
* C++20 and Python 3.
* 
* Python/C API Reference Manual.
* https://docs.python.org/3/c-api/
*/

#pragma once

#include <Python.h>
#include <string>
#include <vector>

#define Py_ObjWrap(pobj) \
    { (PyObject*)pobj , false }
#define Py_ObjectWrap(pobj) \
    Object((PyObject*)pobj, false)

#define Py_ArgsFromCFunc(pargs) \
    Py::Tuple args = pargs;
#define Py_KwArgsFromCFunc(pargs, pkwargs) \
    Py::Tuple args = pargs;                \
    Py::Dict kwargs = pkwargs;

#define Py_GetItemStr(obj, index) \
    ((Py::Str)obj.GetItem((size_t)index))
#define Py_GetValueStr(obj, key) \
    ((Py::Str)obj.GetItem(Py::Str(key)))


namespace Py
{
    typedef PyObject* (*Function)(PyObject* data);
    typedef PyObject* (*FunctionArgs)(PyObject* data, PyObject* args);
    typedef PyObject* (*FunctionKwArgs)(PyObject* data, PyObject* args, PyObject* kwargs);

    class BaseMem abstract
    {
    public:
        virtual ~BaseMem() { }

        operator void*() const
        {
            return ptr;
        }

    protected:
        void* ptr = nullptr;
    };

    class Mem : public BaseMem
    {
    public:
        Mem(void* p)
        {
            ptr = p;
        }

        Mem(size_t n)
        {
            ptr = PyMem_Malloc(n);
            if (ptr == nullptr)
                throw;
        }

        ~Mem()
        {
            if (ptr != nullptr)
                PyMem_Free(ptr);
        }

        void Realloc(size_t n)
        {
            void* p = PyMem_Realloc(ptr, n);
            if (p == nullptr)
                throw;
            ptr = p;
        }
    };

    class RawMem : public BaseMem
    {
    public:
        RawMem(void* p)
        {
            ptr = p;
        }

        RawMem(size_t n)
        {
            ptr = PyMem_RawMalloc(n);
            if (ptr == nullptr)
                throw;
        }

        ~RawMem()
        {
            if (ptr != nullptr)
                PyMem_RawFree(ptr);
        }

        void Realloc(size_t n)
        {
            void* p = PyMem_RawRealloc(ptr, n);
            if (p == nullptr)
                throw;
            ptr = p;
        }
    };

    class WideMem : public Mem
    {
    public:
        WideMem(wchar_t* s)
            : Mem(s)
        { }

        operator wchar_t*() const
        {
            return (wchar_t*)ptr;
        }
    };

    /// <summary>
    /// wchar_t -> char
    /// </summary>
    class EncodedString : public Mem
    {
    public:
        EncodedString(char* s)
            : Mem((void*)s)
        { }

        EncodedString(const wchar_t* s)
            : Mem(Py_EncodeLocale(s, nullptr))
        { }

        operator char*()
        {
            return (char*)ptr;
        }
    };

    /// <summary>
    /// char -> wchar_t
    /// </summary>
    class DecodedString : public RawMem
    {
    public:
        DecodedString(wchar_t* s)
            : RawMem((void*)s)
        { }

        DecodedString(const char* s)
            : RawMem(Py_DecodeLocale(s, nullptr))
        { }

        operator wchar_t*()
        {
            return (wchar_t*)ptr;
        }
    };

    class U8Str
    {
    public:
        U8Str(const char* s)
        {
            ptr = const_cast<char*>(s);
        }

        U8Str(const wchar_t* s)
        {
            encoded = new EncodedString(s);
            ptr = static_cast<char*>(*encoded);
        }

        U8Str(const std::string& s)
        {
            ptr = const_cast<char*>(s.data());
        }

        U8Str(const std::wstring& s)
        {
            encoded = new EncodedString(s.data());
            ptr = static_cast<char*>(*encoded);
        }

        virtual ~U8Str()
        {
            delete encoded;
        }

        operator char*() const
        {
            return ptr;
        }

    private:
        char* ptr = nullptr;
        EncodedString* encoded = nullptr;
    };

    class WideStr
    {
    public:
        WideStr(const wchar_t* s)
        {
            ptr = const_cast<wchar_t*>(s);
        }

        WideStr(const char* s)
        {
            decoded = new DecodedString(s);
            ptr = static_cast<wchar_t*>(*decoded);
        }

        WideStr(const std::string& s)
        {
            decoded = new DecodedString(s.data());
            ptr = static_cast<wchar_t*>(*decoded);
        }

        WideStr(const std::wstring& s)
        {
            ptr = const_cast<wchar_t*>(s.data());
        }

        WideStr(const U8Str& s)
        {
            decoded = new DecodedString((const char*)s);
            ptr = *decoded;
        }

        virtual ~WideStr()
        {
            delete decoded;
        }

        operator wchar_t*() const
        {
            return ptr;
        }

    private:
        wchar_t* ptr = nullptr;
        DecodedString* decoded = nullptr;
    };

    class Object
    {
    public:
        Object()
            : ptr(nullptr)
        { }

        Object(const Object& obj)
            : ptr(obj)
        {
            AddRef();
        }

        Object(PyObject* obj, bool addref = true)
            : ptr(obj)
        {
            if (addref)
                AddRef();
        }

        virtual ~Object()
        {
            Release();
        }

        /// <summary>
        /// Get the current reference count for this object.
        /// </summary>
        size_t GetRef() const
        {
            return ptr->ob_refcnt;
        }

        /// <summary>
        /// Increment the reference count for this object.
        /// </summary>
        Object& AddRef()
        {
            Py_IncRef(ptr);  // may be null
            return *this;
        }

        /// <summary>
        /// Decrement the reference count for this object.
        /// </summary>
        void Release()
        {
            Py_DecRef(ptr);  // may be null
            ptr = nullptr;
        }

        /// <summary>
        /// Get the length of this object.
        /// </summary>
        size_t GetSize() const
        {
            return PyObject_Size(ptr);
        }

        /// <summary>
        /// Get the documentation string for this object.
        /// </summary>
        const U8Str GetDocumentation()
        {
            return ptr->ob_type->tp_doc;
        }

        /// <summary>
        /// Get the name of this object, for printing, in format 'module.name'.
        /// </summary>
        const U8Str GetName()
        {
            return ptr->ob_type->tp_name;
        }

        /// <summary>
        /// Gets an attribute from the object.
        /// </summary>
        Object GetAttr(PyObject* obj) const
        {
            return Py_ObjWrap(PyObject_GetAttr(ptr, obj));
        }

        /// <summary>
        /// Gets an attribute from the object.
        /// </summary>
        Object GetAttr(const U8Str& s) const
        {
            return Py_ObjWrap(PyObject_GetAttrString(ptr, s));
        }

        /// <summary>
        /// Determine if this object is the Python None object.
        /// </summary>
        bool IsNone() const
        {
            return ptr == (PyObject*)Py_None;
        }

        /// <summary>
        /// Determine if this object is callable.
        /// </summary>
        bool IsCallable() const
        {
            return (bool)PyCallable_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a method object (PyMethod_Type).
        /// </summary>
        bool IsMethod() const
        {
            return (bool)PyMethod_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a function object (PyFunction_Type).
        /// </summary>
        bool IsFunction() const
        {
            return ptr != nullptr && PyFunction_Check(ptr);
        }

        /// <summary>
        /// Determine if this object provides numeric protocols.
        /// </summary>
        bool IsNumber() const
        {
            return (bool)PyNumber_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a Unicode object or an instance of a Unicode subtype.
        /// </summary>
        bool IsStr() const
        {
            return (bool)PyUnicode_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a Unicode object, but not an instance of a subtype.
        /// </summary>
        bool IsStrExact() const
        {
            return (bool)PyUnicode_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is of type Bool.
        /// </summary>
        bool IsBool() const
        {
            return (bool)PyBool_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyLongObject or a subtype of PyLongObject.
        /// </summary>
        bool IsInt() const
        {
            return (bool)PyLong_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyLongObject, but not a subtype of PyLongObject.
        /// </summary>
        bool IsIntExact() const
        {
            return (bool)PyLong_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyFloatObject or a subtype of PyFloatObject.
        /// </summary>
        bool IsFloat() const
        {
            return (bool)PyFloat_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyFloatObject, but not a subtype of PyFloatObject.
        /// </summary>
        bool IsFloatExact() const
        {
            return (bool)PyFloat_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyComplexObject or a subtype of PyComplexObject.
        /// </summary>
        bool IsComplex() const
        {
            return (bool)PyComplex_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyComplexObject or a subtype of PyComplexObject.
        /// </summary>
        bool IsComplexExact() const
        {
            return (bool)PyComplex_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a list object or an instance of a subtype of the list type.
        /// </summary>
        bool IsList() const
        {
            return (bool)PyList_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a list object, but not an instance of a subtype of the list type.
        /// </summary>
        bool IsListExact() const
        {
            return (bool)PyList_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a tuple object or an instance of a subtype of the tuple type.
        /// </summary>
        bool IsTuple() const
        {
            return (bool)PyTuple_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a tuple object, but not an instance of a subtype of the tuple type.
        /// </summary>
        bool IsTupleExact() const
        {
            return (bool)PyTuple_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a dict object or an instance of a subtype of the dict type.
        /// </summary>
        bool IsDict() const
        {
            return (bool)PyDict_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a dict object, but not an instance of a subtype of the dict type.
        /// </summary>
        bool IsDictExact() const
        {
            return (bool)PyDict_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a set object or an instance of a subtype.
        /// </summary>
        bool IsSet() const
        {
            return (bool)PySet_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a set object, a frozenset object, or an instance of a subtype.
        /// </summary>
        bool IsAnySet() const
        {
            return (bool)PyAnySet_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a set object or a frozenset object but not an instance of a subtype.
        /// </summary>
        bool IsAnySetExact() const
        {
            return (bool)PyAnySet_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a bytes object or an instance of a subtype of the bytes type.
        /// </summary>
        bool IsBytes() const
        {
            return (bool)PyBytes_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a bytes object, but not an instance of a subtype of the bytes type.
        /// </summary>
        bool IsBytesExact() const
        {
            return (bool)PyBytes_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a bytearray object or an instance of a subtype of the bytearray type.
        /// </summary>
        bool IsByteArray() const
        {
            return (bool)PyByteArray_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a bytearray object, but not an instance of a subtype of the bytearray type.
        /// </summary>
        bool IsByteArrayExact() const
        {
            return (bool)PyByteArray_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a context object (PyContext_Type).
        /// </summary>
        bool IsContextExact() const
        {
            return ptr != nullptr && (bool)PyContext_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a context variable token type (PyContextToken_Type).
        /// </summary>
        bool IsContextTokenExact() const
        {
            return ptr != nullptr && (bool)PyContextToken_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a context variable type (PyContextVar_Type).
        /// </summary>
        bool IsContextVarExact() const
        {
            return ptr != nullptr && PyContextVar_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a coroutine object (PyCoro_Type).
        /// </summary>
        bool IsCoro() const
        {
            return ptr != nullptr && PyCoro_CheckExact(ptr);
        }

        /// <summary>
        /// Determine if this object is a code object.
        /// </summary>
        bool IsCode() const
        {
            return (bool)PyCode_Check(ptr);
        }

        /// <summary>
        /// etermines if this object is a cell object.
        /// </summary>
        bool IsCell() const
        {
            return ptr != nullptr && PyCell_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is PyCallIter_Type.
        /// </summary>
        bool IsCallIter() const
        {
            return (bool)PyCallIter_Check(ptr);
        }

        /// <summary>
        /// Determine if this object is a PyCapsule.
        /// </summary>
        bool IsCapsule() const
        {
            return (bool)PyCapsule_CheckExact(ptr);
        }

        void SetObject(const Object& obj)
        {
            Release();
            ptr = obj.ptr;
            AddRef();
        }

        bool operator==(const Object& rhs) const
        {
            return ptr == rhs.ptr;
        }

        PyObject* operator->() const
        {
            return ptr;
        }

        operator PyObject*() const
        {
            return ptr;
        }

        operator bool() const
        {
            return ptr != nullptr;
        }

    protected:
        PyObject* ptr;
    };

    class None : public Object
    {
    public:
        None()
            : Object((PyObject*)Py_None) // should increment the reference count
        { }
    };

    class Str : public Object
    {
    public:
        Str(const Object& obj)
            : Object(obj)
        { }

        Str(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        /// <summary>
        /// Creates a Unicode object (UTF-8) from a null-terminated char buffer.
        /// </summary>
        Str(const char* s)
            : Py_ObjectWrap(PyUnicode_FromString(s))
        { }

        /// <summary>
        /// Creates a Unicode object from a wchar_t buffer of the given size.
        /// </summary>
        Str(const wchar_t* s)
            : Py_ObjectWrap(PyUnicode_FromWideChar(s, -1))
        { }

        Str(const std::string& s)
            : Py_ObjectWrap(PyUnicode_FromString(s.data()))
        { }

        Str(const std::wstring& s)
            : Py_ObjectWrap(PyUnicode_FromWideChar(s.data(), -1))
        { }

        Str(const U8Str& s)
            : Py_ObjectWrap(PyUnicode_FromString((const char*)s))
        { }

        Str(const WideStr& s)
            : Py_ObjectWrap(PyUnicode_FromWideChar((const wchar_t*)s, -1))
        {}

        /// <summary>
        /// Get a pointer to the null-terminated UTF-8 encoding of the Unicode object.
        /// This caches the UTF-8 representation of the string in the Unicode object, and subsequent calls will return a pointer to the same buffer.
        /// The caller is not responsible for deallocating the buffer.
        /// </summary>
        /// <param name="size">Stores the number of bytes of the encoded representation, excluding the trailing null termination character.</param>
        const char* GetUTF8(size_t* size = nullptr) const
        {
            return PyUnicode_AsUTF8AndSize(ptr, (Py_ssize_t*)size);
        }

        /// <summary>
        /// Convert the Unicode object to a null-terminated wide character string.
        /// </summary>
        /// <param name="size">Stores the number of wide characters, excluding the trailing null termination character.</param>
        WideMem GetWide(size_t* size = nullptr) const
        {
            return PyUnicode_AsWideCharString(ptr, (Py_ssize_t*)size);
        }

        /// <summary>
        /// Copy the Unicode object contents into a wide character string.
        /// The resulting string may or may not be null-terminated and might contain null characters.
        /// </summary>
        /// <param name="size">The maximum number of wide characters to copy, excluding a possibly trailing null termination character.</param>
        /// <returns>Returns the number of wide characters copied or -1 in case of an error.</returns>
        size_t GetWide(wchar_t* s, size_t size)
        {
            return PyUnicode_AsWideChar(ptr, s, size);
        }

        /// <summary>
        /// Get the length of the Unicode string, in code points.
        /// </summary>
        size_t GetSize() const
        {
            return PyUnicode_GET_LENGTH(ptr);
        }

        operator std::string() const
        {
            size_t size = 0;
            const char* s = GetUTF8(&size);
            return { s, size };
        }

        operator std::wstring() const
        {
            size_t size = 0;
            WideMem wmem = GetWide(&size);
            return { (wchar_t*)wmem, size };
        }
    };

    class Float : public Object
    {
    public:
        Float(const Object& obj)
            : Object(obj)
        { }

        Float(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        /// <summary>
        /// Creates a Float object based on a string.
        /// </summary>
        Float(const Str& s)
            : Py_ObjectWrap(PyFloat_FromString(s))
        { }

        /// <summary>
        /// Creates a Float object from a C double.
        /// </summary>
        Float(double d)
            : Py_ObjectWrap(PyFloat_FromDouble(d))
        { }

        operator float() const
        {
            return (float)PyFloat_AsDouble(ptr);
        }

        operator double() const
        {
            return PyFloat_AsDouble(ptr);
        }
    };

    class Int : public Object
    {
    public:
        Int(const Object& obj)
            : Object(obj)
        { }

        Int(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        /// <summary>
        /// Creates a Int object based on a string interpreted according to the specified radix.
        /// </summary>
        /// <param name="base">If base is not 0, it must be between 2 and 36, inclusive.</param>
        Int(const U8Str& s, int base = 0)
            : Py_ObjectWrap(PyLong_FromString(s, nullptr, base))
        { }

        /// <summary>
        /// Creates a Float object from a C long.
        /// </summary>
        Int(long l)
            : Py_ObjectWrap(PyLong_FromLong(l))
        { }

        /// <summary>
        /// Creates a Float object from a C long long.
        /// </summary>
        Int(long long ll)
            : Py_ObjectWrap(PyLong_FromLongLong(ll))
        { }

        /// <summary>
        /// Creates a Float object from a C unsigned long.
        /// </summary>
        Int(unsigned long ul)
            : Py_ObjectWrap(PyLong_FromUnsignedLong(ul))
        { }

        /// <summary>
        /// Creates a Float object from a C unsigned long long.
        /// </summary>
        Int(unsigned long long ull)
            : Py_ObjectWrap(PyLong_FromUnsignedLongLong(ull))
        { }

        /// <summary>
        /// Return an int64_t representation of this object.
        /// </summary>
        /// <param name="overflow">If this object is greater than LLONG_MAX or less than LLONG_MIN, set *overflow to 1 or -1, respectively.</param>
        int64_t ToInt64(int* overflow = nullptr) const
        {
            if (overflow)
                return (int64_t)PyLong_AsLongLongAndOverflow(ptr, overflow);
            return (int64_t)PyLong_AsLongLong(ptr);
        }

        operator long() const
        {
            return PyLong_AsLong(ptr);
        }

        operator long long() const
        {
            return PyLong_AsLongLong(ptr);
        }

        operator unsigned long() const
        {
            return PyLong_AsUnsignedLong(ptr);
        }

        operator unsigned long long() const
        {
            return PyLong_AsUnsignedLongLong(ptr);
        }
    };

    template<typename T>
    class ObjIterator
    {
    public:
        const T* ptr; size_t index = 0;
        Object operator*() const { return ptr->GetItem(index); }
        bool operator!=(size_t end) const { return index < end; }
        void operator++() { ++index; }
    };

    /// <summary>
    /// Represents a Python tuple object.
    /// </summary>
    class Tuple : public Object
    {
    public:
        Tuple(const Object& obj)
            : Object(obj)
        { }

        Tuple(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        /// <summary>
        /// Create a Tuple object of the specified length.
        /// Before using, items must be initialized by calling SetItemInit.
        /// </summary>
        static Tuple FromSize(size_t n)
        {
            return Py_ObjWrap(PyTuple_New(n));
        }

        /// <summary>
        /// Create a Tuple object initialized with the specified items.
        /// </summary>
        template <typename... Values>
        static Tuple FromValues(Values... values)
        {
            constexpr size_t n = sizeof...(values);
            return Py_ObjWrap(PyTuple_Pack(n, static_cast<PyObject*>(values)...));
        }

        /// <summary>
        /// Resize the tuple.
        /// Because tuples are supposed to be immutable, this must only be used if there is only one reference to the object.
        /// </summary>
        bool Resize(size_t n)
        {
            return !_PyTuple_Resize(&ptr, n);
        }

        /// <summary>
        /// Get the number of items in the tuple.
        /// </summary>
        size_t GetSize() const
        {
            return PyTuple_Size(ptr);
        }

        /// <summary>
        /// Get the item at the specified position.
        /// </summary>
        Object GetItem(size_t pos) const
        {
            return Py_ObjWrap(PyTuple_GetItem(ptr, pos));
        }

        /// <summary>
        /// Replace the item at the specified position.
        /// </summary>
        bool SetItem(size_t pos, const Object& obj)
        {
            return !PyTuple_SetItem(ptr, pos, obj);
        }

        /// <summary>
        /// Initialize an item at the specified position.
        /// This function must only be used to fill in brand new tuples.
        /// </summary>
        void SetItemInit(size_t pos, const Object& obj)
        {
            PyTuple_SET_ITEM(ptr, pos, obj);
        }

        Object operator[](size_t index) const
        {
            return GetItem(index);
        }

        auto begin() const
        {
            return ObjIterator<Tuple>{this};
        }

        auto end() const
        {
            return GetSize();
        }
    };

    class List : public Object
    {
    public:
        List()
            : Py_ObjectWrap(PyList_New(0))
        { }

        List(const Object& obj)
            : Object(obj)
        { }

        List(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        /// <summary>
        /// Create a List object of the specified length.
        /// Before using, items must be initialized by calling SetItemInit.
        /// </summary>
        static List FromSize(size_t n)
        {
            return Py_ObjWrap(PyList_New(n));
        }

        /// <summary>
        /// Create a List object initialized with the specified values.
        /// </summary>
        template <typename... Values>
        static List FromValues(Values... values)
        {
            std::vector<PyObject*> vec { values... };
            PyObject* pobj = PyList_New(vec.size());
            for (size_t i = 0; i < vec.size(); i++)
                PyList_SET_ITEM(pobj, i, vec[i]);
            return Py_ObjWrap(pobj);
        }

        /// <summary>
        /// Get the number of items in this list.
        /// </summary>
        size_t GetSize() const
        {
            return PyList_Size(ptr);
        }

        /// <summary>
        /// Get a list of the objects between low and high in this list.
        /// </summary>
        List GetSlice(size_t low, size_t high) const
        {
            return Py_ObjWrap(PyList_GetSlice(ptr, low, high));
        }

        /// <summary>
        /// Set the slice of this list between low and high to the contents of itemlist.
        /// </summary>
        bool SetSlice(size_t low, size_t high, PyObject* itemlist) const
        {
            return !PyList_SetSlice(ptr, low, high, itemlist);
        }

        /// <summary>
        /// Get the item at the specified position.
        /// </summary>
        Object GetItem(size_t index) const
        {
            return Py_ObjWrap(PyList_GetItem(ptr, index));
        }

        /// <summary>
        /// Replace the item at the specified position.
        /// </summary>
        bool SetItem(size_t index, const Object& obj) const
        {
            return !PyList_SetItem(ptr, index, obj);
        }

        /// <summary>
        /// Initialize an item at the specified position.
        /// This function must only be used to fill in brand new lists.
        /// </summary>
        void SetItemInit(size_t index, const Object& obj) const
        {
            PyList_SET_ITEM(ptr, index, obj);
        }

        /// <summary>
        /// Insert an item in front of the specified position.
        /// </summary>
        bool Insert(size_t index, const Object& obj) const
        {
            return !PyList_Insert(ptr, index, obj);
        }

        /// <summary>
        /// Append an item at the end of this list.
        /// </summary>
        bool Append(const Object& obj) const
        {
            return !PyList_Append(ptr, obj);
        }

        /// <summary>
        /// Sort the items of this list in place.
        /// </summary>
        bool Sort() const
        {
            return !PyList_Sort(ptr);
        }

        /// <summary>
        /// Reverse the items of this list in place.
        /// </summary>
        bool Reverse() const
        {
            return !PyList_Reverse(ptr);
        }

        /// <summary>
        /// Create a tuple containing the contents of this list.
        /// </summary>
        Tuple Tuple() const
        {
            return Py_ObjWrap(PyList_AsTuple(ptr));
        }

        Object operator[](size_t index) const
        {
            return GetItem(index);
        }

        auto begin() const
        {
            return ObjIterator<List>{this};
        }

        auto end() const
        {
            return GetSize();
        }
    };

    class Dict : public Object
    {
    public:
        Dict()
            : Py_ObjectWrap(PyDict_New())
        { }

        Dict(const Object& obj)
            : Object(obj)
        { }

        Dict(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        /// <summary>
        /// Create a Dict object initialized with the specified key-value pairs.
        /// </summary>
        template<typename... Values>
        static Dict FromValues(Values... kwargs)
        {
            Dict dict;
            dict.SetItems(kwargs...);
            return dict;
        }

        /// <summary>
        /// Get the number of items in the dictionary.
        /// </summary>
        size_t GetSize() const
        {
            return PyDict_Size(ptr);
        }

        /// <summary>
        /// Remove the item in dictionary with the specified key.
        /// </summary>
        bool DelItem(const Object& key) const
        {
            return !PyDict_DelItem(ptr, key);
        }

        /// <summary>
        /// Get the item in dictionary with the specified key.
        /// </summary>
        Object GetItem(const Object& key) const
        {
            return Py_ObjWrap(PyDict_GetItem(ptr, key));
        }

        /// <summary>
        /// Replace or insert the item in dictionary with the specified key.
        /// </summary>
        bool SetItem(const Object& key, const Object& value) const
        {
            return !PyDict_SetItem(ptr, key, value);
        }

        /// <summary>
        /// Replace or insert a set of items in the dictionary.
        /// </summary>
        template <typename... Args>
        void SetItems(Args... kwargs) const
        {
            std::vector<PyObject*> items { kwargs... };
            for (size_t i = 0; i < items.size(); i += 2)
                SetItem(items[i], items[i + 1]);
        }

        /// <summary>
        /// If the item is present, returns the asociated value.
        /// If the item is not present, it is inserted with the specified default value.
        /// </summary>
        Object SetDefault(const Object& key, const Object& def) const
        {
            return Py_ObjWrap(PyDict_SetDefault(ptr, key, def));
        }

        /// <summary>
        /// Empty this dictionary of all key-value pairs.
        /// </summary>
        void Clear() const
        {
            PyDict_Clear(ptr);
        }

        /// <summary>
        /// Determine if the dictionary contains the specified item.
        /// </summary>
        int Contains(const Object& key) const
        {
            return PyDict_Contains(ptr, key);
        }

        /// <summary>
        /// Create a new dictionary that contains the same key-value pairs as this one.
        /// </summary>
        Dict Copy() const
        {
            return Py_ObjWrap(PyDict_Copy(ptr));
        }
        
        /// <summary>
        /// Create a list containing all the items from the dictionary.
        /// </summary>
        List Items() const
        {
            return Py_ObjWrap(PyDict_Items(ptr));
        }

        /// <summary>
        /// Create a list containing all the keys from the dictionary.
        /// </summary>
        List Keys() const
        {
            return Py_ObjWrap(PyDict_Keys(ptr));
        }

        /// <summary>
        /// Create a list containing all the values from the dictionary.
        /// </summary>
        List Values() const
        {
            return Py_ObjWrap(PyDict_Values(ptr));
        }

        /// <summary>
        /// Iterate over all key-value pairs in this dictionary.
        /// </summary>
        /// <param name="pos">Must be initialized to 0 prior to the first call to this function to start the iteration.</param>
        /// <returns>Returns true for each pair in the dictionary, and false once all pairs have been reported.</returns>
        bool Next(size_t* pos, PyObject** pkey, PyObject** pvalue) const
        {
            return PyDict_Next(ptr, (Py_ssize_t*)pos, pkey, pvalue);
        }
    };

    class Module : public Object
    {
    public:
        Module(const Object& obj)
            : Object(obj)
        { }

        Module(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        Module(const Str& name)
            : Py_ObjectWrap(PyImport_Import(name))
        {}

        Module(const Str& name, PyObject* globals, PyObject* locals, PyObject* fromlist, int level = 0)
            : Py_ObjectWrap(PyImport_ImportModuleLevelObject(name, globals, locals, fromlist, level))
        {}

        /// <summary>
        /// Get the name of the file from which this module was loaded using module’s __file__ attribute.
        /// </summary>
        Str GetFileName()
        {
            return Py_ObjWrap(PyModule_GetFilenameObject(ptr));
        }
    };

    class Callable : public Object
    {
    public:
        Callable()
            : Object()
        { }

        Callable(const Object& obj)
            : Object(obj)
        { }

        Callable(PyObject* obj, bool addref = true)
            : Object(obj, addref)
        { }

        Callable(Function fn, PyObject* data = nullptr)
        {
            method.ml_flags = METH_NOARGS;
            method.ml_meth = (PyCFunction)fn;
            ptr = PyCFunction_New(&method, data);
        }

        Callable(FunctionArgs fn, PyObject* data = nullptr)
        {
            method.ml_flags = METH_VARARGS;
            method.ml_meth = (PyCFunction)fn;
            ptr = PyCFunction_New(&method, data);
        }

        Callable(FunctionKwArgs fn, PyObject* data = nullptr)
        {
            method.ml_flags = METH_VARARGS | METH_KEYWORDS;
            method.ml_meth = (PyCFunction)fn;
            ptr = PyCFunction_New(&method, data);
        }

        /// <summary>
        /// Call a callable Python object callable without any arguments.
        /// </summary>
        Object Call() const
        {
            return Py_ObjWrap(PyObject_CallNoArgs(ptr));
        }

        /// <summary>
        /// Call a callable Python object callable with exactly 1 positional argument.
        /// </summary>
        Object Call(const Object& arg) const
        {
            return Py_ObjWrap(PyObject_CallOneArg(ptr, arg));
        }

        /// <summary>
        /// Call a callable Python object callable with positional and/or named arguments.
        /// <para>If no positional arguments are needed, specify an empty tuple.</para>
        /// <para>If no named arguments are needed, specify nullptr.</para>
        /// </summary>
        Object Call(const Object& args, const Object& kwargs) const
        {
            return Py_ObjWrap(PyObject_Call(ptr, args, kwargs));
        }

        /// <summary>
        /// Call a callable Python object callable with a variable number of arguments.
        /// </summary>
        template<typename... Args>
        Object CallArgs(Args... args) const
        {
            return Py_ObjWrap(PyObject_CallFunctionObjArgs(ptr, static_cast<PyObject*>(args)..., nullptr));
        }

        /// <summary>
        /// Call a callable Python object callable with a variable number of C arguments described using a style format string.
        /// <para>The format can be nullptr, indicating that no arguments are provided.</para>
        /// </summary>
        template<typename... Args>
        Object CallFormat(const char* fmt, Args... args) const
        {
            return Py_ObjWrap(PyObject_CallFunction(ptr, fmt, static_cast<PyObject*>(args)...));
        }

        /// <summary>
        /// Call a callable Python object callable with arguments given by a tuple.
        /// <para>If no arguments are needed, then args can be nullptr.</para>
        /// </summary>
        Object CallObj(const Object& args) const
        {
            //return Call(args, nullptr);  // *args, **kwargs=nullptr
            return Py_ObjWrap(PyObject_CallObject(ptr, args));
        }

        /// <summary>
        /// Call a callable Python object callable without any arguments.
        /// </summary>
        Object operator()() const
        {
            return Call();
        }

        /// <summary>
        /// Call a callable Python object callable with exactly 1 positional argument.
        /// </summary>
        Object operator()(const Object& arg) const
        {
            return Call(arg);
        }

        /// <summary>
        /// Call a callable Python object callable with positional and/or named arguments.
        /// <para>If no positional arguments are needed, specify an empty tuple.</para>
        /// <para>If no named arguments are needed, specify nullptr.</para>
        /// </summary>
        Object operator()(const Object& args, const Object& kwargs) const
        {
            return Call(args, kwargs);
        }

    protected:
        PyMethodDef method { };
    };

    /// <summary>
    /// Determine if the Python interpreter has been initialized.
    /// </summary>
    inline bool IsInitialized()
    {
        return (bool)Py_IsInitialized();
    }

    /// <summary>
    /// Initialize the Python interpreter.
    /// In an application embedding Python, this should be called before using any other Python/C API functions
    /// </summary>
    /// <param name="initsigs">If initsigs is 0, it skips initialization registration of signal handlers.</param>
    inline void Initialize(bool initsigs = true)
    {
        Py_InitializeEx(initsigs);
    }

    /// <summary>
    /// Executes a Python source code in the '__main__' module. If '__main__' does not already exist, it is created.
    /// </summary>
    inline int Execute(const U8Str& s)
    {
        return PyRun_SimpleStringFlags(s, nullptr);
    }

    /// <summary>
    /// Undo all initializations made by Initialize() and subsequent use of Python/C API functions.
    /// </summary>
    inline int Finalize()
    {
        return Py_FinalizeEx();
    }

    /// <summary>
    /// Return the program name set with SetProgramName(), or the default.
    /// </summary>
    inline const WideStr GetProgramName()
    {
        return Py_GetProgramName();
    }

    /// <summary>
    /// Return the full program name of the Python executable.
    /// This is computed as a side-effect of deriving the default module search path from the program name.
    /// </summary>
    inline const WideStr GetProgramFullPath()
    {
        return Py_GetProgramFullPath();
    }

    /// <summary>
    /// Return the prefix for installed platform-independent files.
    /// </summary>
    inline const WideStr GetPrefix()
    {
        return Py_GetPrefix();
    }

    /// <summary>
    /// Return the exec-prefix for installed platform-dependent files.
    /// </summary>
    inline const WideStr GetExecPrefix()
    {
        return Py_GetExecPrefix();
    }

    /// <summary>
    /// Return the default module search path; this is computed from the program name and some environment variables.
    /// The returned string consists of a series of directory names separated by a platform dependent delimiter character.
    /// </summary>
    inline const WideStr GetPath()
    {
        return Py_GetPath();
    }

    /// <summary>
    /// Set the default module search path.
    /// </summary>
    inline void SetPath(const WideStr& s)
    {
        Py_SetPath(s);
    }

    /// <summary>
    /// Return the version of this Python interpreter.
    /// </summary>
    inline const U8Str GetVersion()
    {
        return Py_GetVersion();
    }

    /// <summary>
    /// Return the platform identifier for the current platform.
    /// </summary>
    inline const U8Str GetPlatform()
    {
        return Py_GetPlatform();
    }

    /// <summary>
    /// Calls the current “import hook function” (with an explicit level of 0, meaning absolute import).
    /// </summary>
    inline Module Import(const Object& name)
    {
        return PyImport_Import(name);
    }
}
