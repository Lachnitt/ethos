

(declare-const true Bool)
(declare-const false Bool)

(declare-const or (-> Bool Bool Bool) :left-assoc false)
(declare-const and (-> Bool Bool Bool) :right-assoc true)

(declare-const a Bool)
(declare-const b Bool)

(declare-rule and_elim1 ((f Bool) (g Bool))
   :premises ((and f g))
   :args ()
   :conclusion f
)
(declare-rule or_elim1 ((f Bool) (g Bool))
   :premises ((or f g))
   :args ()
   :conclusion f
)

(assume a1 (or a b b b))
(assume a2 (and a b b b))

(step a3 (or a b b) :rule or_elim1 :premises (a1))
(step a4 a :rule and_elim1 :premises (a2))



