; Atoms
(def {nil} {})
(def {true} 1)
(def {false} 0)

; Call function with given list of args
(fun {unpack f l} {
	eval(join (list f) l)
})

; Call function with single list created from args
(fun {pack f & args} {
	f args
})

; Curry aliases
(def {curry} unpack)
(def {uncurry} pack)

; Perform no of actions in sequence
(fun {do & l} {
	if (== l nil) {nil} {last l}
})

; Create a local scope
(fun {let body} {
	((lambda {_} b) ())
})

; Logical Functions
(fun {not x}   {- 1 x})
(fun {or x y}  {+ x y})
(fun {and x y} {* x y})

; Flip arguments for currying
(fun {flip f a b} {f b a})

; comp(f g x) -> f(g(x))
(fun {comp f g x} {f (g x)})

; First elem of list
(fun {fst l} { eval (head l) })
(fun {snd l} { eval (head (tail l)) })
(fun {trd l} { eval (head (tail (tail l))) })

; List Length
(fun {len l} {
  if (== l nil)
    {0}
    {+ 1 (len (tail l))}
})

; Nth item in List
(fun {nth n l} {
  if (== n 0)
    {fst l}
    {nth (- n 1) (tail l)}
})

; Last item in List
(fun {last l} {nth (- (len l) 1) l})

; Take N items
(fun {take n l} {
  if (== n 0)
    {nil}
    {join (head l) (take (- n 1) (tail l))}
})

; Drop N items
(fun {drop n l} {
  if (== n 0)
    {l}
    {drop (- n 1) (tail l)}
})

; Split at N
(fun {split n l} {list (take n l) (drop n l)})

; Element of List
(fun {elem x l} {
  if (== l nil)
    {false}
    {if (== x (fst l)) {true} {elem x (tail l)}}
})

; Create new list by applying function to every elem of list
(fun {map f l} {
  if (== l nil)
    {nil}
    {join (list (f (fst l))) (map f (tail l))}
})

; create a new list of items which match the condition
(fun {filter f l} {
	if (== l nil)
	{nil}
	{join (if (f (fst l)) {head l} {nil}) (filter f (tail l))}
})

; Fold Left
(fun {foldl f z l} {
  if (== l nil)
    {z}
    {foldl f (f z (fst l)) (tail l)}
})

; switch statement, takes zero or more (cond, body) pairs
(fun {select & cs} {
	if (fst (fst cs)) {snd (fst cs)} {unpack switch (tail cs)}
})

; Default Case
(def {otherwise} true)

; C style case statements
(fun {case x & cs} {
  if (== cs nil)
    {error "No Case Found"}
    {if (== x (fst (fst cs))) {snd (fst cs)} {
      unpack case (join (list x) (tail cs))}}
})