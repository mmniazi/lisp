# Lisp Interpreter
Lisp interpreter written in C

## Usage
- Clone: `git clone https://github.com/mmniazi/lisp.git`
- Build (required for run or repl): `make`
- Run: `lisp my_file.lisp`
- Repl: `lisp`

## Features
- Primitive data types and strings
- Common operators (`+`, `-`, `*`, `/`, `>`, `<=`, `==` ...)
- Comments
- Variables
- Flow control (`if/else`, `while`, `switch`)
- Functions & lambdas 
- Higher order functions
- Scopes
- S-expressions
- Q-expressions/lists and list operators
- Varargs
- Importing code
- Error reporting and stack traces

### Example Usage
```
; Fibonacci
(fun {fib n} {
  select
    { (== n 0) 0 }
    { (== n 1) 1 }
    { otherwise (+ (fib (- n 1)) (fib (- n 2))) }
})
```

## Standard Library
#### curry
Call function with given list of args
```
= {params} {1 2 3}
fun {plus x y z} {+ x y z}
(curry plus params)
> 6
```
#### uncurry
Call function with single list created from args
```
uncurry print 1 2 3
> {1 2 3}
```
#### do
Perform no of actions in sequence
```
do (= {x} 2) (+ x 4)
> 6
```
#### let
Create a local scope
```
let {do (= {x} 100) (x)}
> 100
x
> Error: Unbound Symbol 'x'
```
#### not/or/and
Logical functions
```
or (and true false) true
> 1
```
#### flip
Flip arguments for currying
```
flip / 1 2
> 2
```
#### comp
apply functions in sequence `(comp f g x) -> f(g(x))`
```
comp head last {1 2 {3 4 5}} 
```
#### fst/snd/trd
Return first, second or third element of list
```
fst {1 2 3 4}
> 1
snd {1 2 3 4}
> 2
trd {1 2 3 4}
> 3
```
#### nth
Return element on nth index
```
nth 2 {1 2 3 4}
> 3
```
#### last
Return last element of list
```
last {1 2 3}
> 3
```
#### len
Length of list
```
len {1 2 3 5}
> 4
```
#### take
Take N items from list
```
take 2 {1 2 3 5}
> {1 2}
```
#### drop
Drop N items from list
```
drop 2 {1 2 3 5}
> {3 5}
```
#### split
Split list at Nth element
```
split 1 {1 2 3 5}
> {{1 2} {3 5}}
```
#### elem
Check for presence of element
```
elem 5 {1 2 3 5}
> 1
elem 9 {1 2 3 5}
> 0
```
#### map
Create new list by applying function to every elem of list
```
fun {square x} {* x x}
map square {1 2 3}
> {1 4 9}
```
#### filter
Create a new list of items which match the condition
```
fun {even x} {== (% x 2) 0}
filter even {1 2 3 4}
> {2 4}
```
#### foldl
Fold left
```
foldl * 1 {2 2}
> 4
```
#### case
switch statement, takes zero or more (cond, body) pairs
```
(fun {numbers x} { case x {0 "Zero"} {1 "One"} })

numbers 0
> Zero
```
