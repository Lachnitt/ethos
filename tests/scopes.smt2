
(declare-const => (-> Bool Bool Bool))
(declare-const not (-> Bool Bool))

(declare-rule contra ((A Bool))
  :premises (false)
  :args (A)
  :conclusion A)


(declare-const B Bool)

(declare-rule scope
  ((F Bool) (G Bool))
  :assumption F
  :premises (G)
  :args ()
  :conclusion (=> F G)
)

(declare-rule scope-false
  ((F Bool))
  :assumption F
  :premises (false)
  :args ()
  :conclusion (not F)
)
(assume-push a0 true)
(assume-push a1 false)
(step a2 B :rule contra :premises (a1) :args (B))
(step-pop a3 (=> false B)
  :rule scope
  :premises (a2)
  :args ())
(step-pop a4 (=> true (=> false B))
  :rule scope
  :premises (a3)
  :args ())

(assume-push a5 false)
(step-pop a6 (not false) :rule scope-false :premises (a1))


(exit)

