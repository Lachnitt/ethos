(declare-const true Bool)
(declare-const false Bool)

(declare-sort Int 0)
(declare-consts <numeral> Int)

(declare-const and (-> Bool Bool Bool))

(program select (Int Bool) Bool
  ((f1 Bool) (f2 Bool))
  ; cases
  (
  ((select 0 (and f1 f2)) f1)
  ((select 1 (and f1 f2)) f2)
  )
)

(program less-than (Int Int) Bool
  ()
  (
  ((less-than 0 2) true)
  ((less-than 1 2) true)
  )
)

(declare-rule and_elim ((f Bool) (g Bool) (i Int))
   :premises ((and f g))
   :args (i)
   :requires (((less-than i 2) true))
   :conclusion (select i (and f g))
)

(declare-fun P () Bool)
(declare-fun Q () Bool)
(assume a0 (and P Q))
(step a1 Q :rule and_elim :premises (a0) :args (1))

