.. _struct:

Struct Expression
-----------------

defining a struct literal::

       >>> Person = {name: "mariano", "age": 25}
       {name: "mariano", "age": 25}


accessing a field::
        
        >>> Person.name
        "mariano"
        >>> Person.age
        25

changing an attribute::

        >>> Person1 = Person.name := "Mariano"
        {name: "Mariano", "age": 25}

calling a function defined in a struct::

        >>> Animal = {name: "bob", salute: (fn (Self) { io.format("hi! my name is ~s~n", [Self.name]) })}
        {name: "bob"}
        >>> Animal.salute()
        hi! my name is bob


structs are a data structure introduced in efene to express JSON-like data
similar to Javascript objects or python dictionaries and providing a
lightweight object syntax.

another benefit of structs is that they are 100% compatible with the format
defined by `EEP`_ 18 and the one produced by couchdb.

.. _EEP: http://www.erlang.org/eeps/eep-0018.html

This format has only one slight difference with the representation used in
mochijson2, where in mochijson2 a struct is represented by a tuple like::

        (struct, [key => Val, ...])

In EEP 18/couchdb/efene a struct is represented with::

        ([key => Val, ...],) # Note that it's a one item tuple

like the examples shown above a struct can be accessed like an object in any
other object oriented language by specifying the object followed by a dot and
the name of the field.  a struct can contain structs as attributes so you can
access nested attributes with the same syntax::

        Person.address.number

to add a lightweight object oriented syntax to efene you can also define and
call methods in structs, this works like python where every method of an object
explicitly receives the object itself as first argument and is by convention called
*Self*::

        >>> P = struct.set_prototype(Person)
        {name: "mariano", "age": 25}
        >>> P.fields()
        [name, <["age"]>]
        >>> P.has(name)
        true
        >>> P.name?
        true
        >>> P.print()
        {name: "mariano", "age": 25}
        ok

if you want to build more than one object that share the same methods you can
use the proto\_ attribute in a struct that can contain a set of methods that
can be shared between structs, in this case you create a struct with all the
data you need and add a field called proto\_ that contains all the methods. You
can later reuse this proto struct to assign to other objects.

when accessing an attribute or calling a function efene will first lookup in
the struct for it, if not found it will look if there is a proto\_ attribute,
if found it will look for the field there recursively until found or no more
proto\_ fields are available.

If in both cases the field is not found it will raise an exception.

with this lookup algorithm you can override methods defined in the prototype by
creating a method with the same name in the struct, since it will look first
there it will find your version before the proto\_ version.

as any other data structure in efene, structs are inmutable, that means that
when we modify a struct we need to assign the resulting value to another
variable::

        >>> Person1 = Person.name := "Mariano"

the name of the attributes can be atoms or binary strings, all access made
using an atom will lookup the attribute as atom first and then as binary string
if not found, if a string or binary string is given then the attribute will be
looked up only as a binary string.

This is made to avoid converting strings to atoms since atoms are not garbage
collected. Other reason for doing this is that JSON structs comming from the
outside get keys converted to binary strings, to avoid converting those unknown
strings to atoms is another reason for this convention.

the convention is to use atoms as keys when creating structs in erlang and
using binary strings as keys when loading structs from outside (for example
decoding a JSON encoded string).

accessing attributes and checking existence
:::::::::::::::::::::::::::::::::::::::::::

if we want to dinamically access an attribute of a struct using a variable that holds the name of the attribute::

        >>> Person = {name: "mariano", "age": 25}
        {name: [109, 97, 114, 105, 97, 110, 111], "age": 25}
        >>> Attr = age
        age
        >>> Person.age
        25
        >>> Person.Attr
        25


if we want to check if an attribute exists::

        >>> Person.age?
        true
        >>> Person.Attr?
        true
        >>> Person.mail?
        false
        >>> Attr1 = mail
        mail
        >>> Person.Attr1?
        false

accessing attributes with an expression
:::::::::::::::::::::::::::::::::::::::

in some situations you may have attributes that aren't valid unquoted atoms or
you want to access an attribute which name is computed from an expression.

in those cases you can use the alternative syntax to access attributes that is
the same as javascript::

        Struct[<expression>]

some examples::

        >>> S1 = {"b": 2, a: 1}
        {"b": 2, a: 1}
        >>> S1["b"]
        2
        >>> F1 = fn () { a }
        #Fun<erl_eval.20.67289768>
        >>> F2 = fn () { <["b"]> }
        #Fun<erl_eval.20.67289768>
        >>> S1[F1()]
        1
        >>> S1[F2()]
        2
        >>> S1['a']
        1
        >>> V1 = a
        a
        >>> V2 = <["b"]>
        <["b"]>
        >>> S1[V1]
        1
        >>> S1[V2]
        2
        >>> S1.V1
        1
        >>> S1.V2
        2
        >>> S2 = {a: {b: 42}}
        {a: {b: 42}}
        >>> S2.a.b
        42
        >>> S2[a].b
        42
        >>> S2.a[b]
        42
        >>> S2[F1()].b
        42

.. note::
        if the expression evaluates to a string it must be a binary string to match the attribute,
        explicit strings are converted to binary strings automatically at compile time but to avoid
        adding a function call to all expressions you have to generate a binary string.

        this behavior may change in the future, generating atoms is recommended and will not change.
