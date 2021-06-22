# Python-C-API
C++20 and Python/C API.

## Example
### consoleapp.cpp
```cpp
#include "python.h"

#include <iostream>

using namespace std;

PyObject* f_hello_world(PyObject* pdata, PyObject* pargs, PyObject* pkwargs)
{
    Py_KwArgsFromCFunc(pargs, pkwargs);

    string hello = Py_GetItemStr(args, 0);  // 'hello'
    string world = Py_GetItemStr(args, 1);  // 'world'
    string end   = Py_GetItemStr(args, 2);  // '!'

    string space = Py_GetValueStr(kwargs, "space");  // ' '

    return Py::Str(hello + space + world + end)
        .AddRef();
};

int main()
{
    Py::Initialize();

    // Load module 'main.py'.
    Py::Module mod("main");

    // Generate 1000 digits of Pi.
    Py::Callable pi_digits_str = mod.GetAttr("pi_digits_str");
    Py::Str result = pi_digits_str(Py::Int(1000l));
    cout << result.GetUTF8() << endl;

    cout << endl;

    // Print *args and **kwargs.
    Py::Callable print_args = mod.GetAttr("print_args");
    print_args(
        Py::Tuple::FromValues(
            Py::Str("value #1"),
            Py::Str("value #2")
        ),
        Py::Dict::FromValues(
            Py::Str("key #1"), Py::Int(256l),
            Py::Str("key #2"), Py::Float(3.14)
        )
    );

    cout << endl;

    // Print 'Hello World!'.
    Py::Callable hello_world = mod.GetAttr("hello_world");
    Py::Str hello_world_str = hello_world(Py::Callable(f_hello_world));
    cout << hello_world_str.GetUTF8() << endl;

    return 0;
}
```
### main.py
```py3
def pi_digits(n):
    """
    Generate n digits of Pi.
    https://gist.github.com/deeplook/4947835
    """
    k, a, b, a1, b1 = 2, 4, 1, 12, 4
    while n > 0:
        p, q, k = k * k, 2 * k + 1, k + 1
        a, b, a1, b1 = a1, b1, p * a + q * a1, p * b + q * b1
        d, d1 = a / b, a1 / b1
        while d == d1 and n > 0:
            yield int(d)
            n -= 1
            a, a1 = 10 * (a % b), 10 * (a1 % b1)
            d, d1 = a / b, a1 / b1

def pi_digits_str(n):
    digits = [str(n) for n in list(pi_digits(n))]

    return '{}.{}'.format(
        digits.pop(0),
        ''.join(digits)
    )

def print_args(*args, **kwargs):
    print('\n'.join(args))
    for key, value in kwargs.items():
        print(f'{key}={value}')

def hello_world(f_hello_world):
    return f_hello_world('Hello', 'World', '!', space=' ')
```
