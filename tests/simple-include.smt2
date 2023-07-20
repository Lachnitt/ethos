
(include "./tests/Bools.smt2")

(declare-rule eq-symm ((T Type) (x T) (y T))
  :premises ((= T x y))
  :args ()
  :conclusion (= T y x))
  
(declare-rule contra ((p Bool))
  :premises (p (not p))
  :args ()
  :conclusion false
)
