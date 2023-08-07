(include "../rules/Booleans.smt2")

(declare-const c1 Bool)
(declare-const c2 Bool)
(declare-const c3 Bool)

; Resolution
; TODO: more tests
(assume a1 (or      c1  c2))
(assume a2 (or (not c2) c3))
(assume a3 (not c2))
(assume a4 (or c1 c2 c3 c2))

(step t1 (or c1 c3) :rule resolution :premises (a1 a2) :args (true c2))
(step t2 (or c3 c1) :rule resolution :premises (a2 a1) :args (false c2))
(step t3 c1 :rule resolution :premises (a1 a3) :args (true c2))
(step t4 c1 :rule resolution :premises (a3 a1) :args (false c2))
(step t5 (or c1 c3 c2 c3) :rule resolution :premises (a4 a2) :args (true c2))
(step t6 (or c1 c3 c2) :rule resolution :premises (a4 a3) :args (true c2))

; Chain Resolution
(assume cra1 (and (or c1 c2 (not c3) c2)
                  (not c2)
                  (or c3 c1))
)
(step cr1 (or c1 c2 c1) :rule chain_resolution :premises (cra1) :args ((and true c2 false c3)))

; Factoring
(assume faca1 (or c1 c1 c2 (not c2) (not c2)))
(step   fact1 (or c1    c2 (not c2)          ) :rule factoring :premises (faca1))
(assume faca2 (or c1 (or c1 c1) c2 (not c2) (not c2) c1))
(step   fact2 (or c1 (or c1 c1) c2 (not c2)          c1) :rule factoring :premises (faca2))

; Reordering
(assume reord1 (and c1 c2))
(step  reordt1 :rule reordering :premises (reord1) :args ((and c1 c2)))

(assume reord2 (or c1 c2))
(step  reordt2 :rule reordering :premises (reord2) :args ((or c2 c1)))

(assume reord3 (or c1 c1 (or c1 c2) (not c2) (not c2) c1))
(step  reordt3 :rule reordering :premises (reord3) :args 
       ((or (or c1 c2) c1 (not c2) c1 (not c2) c1)))

; not_and
(assume notanda1 (not (and c1 c2 (not c2))))
(step   notandt1 (or (not c1) (not c2) (not (not c2))) :rule not_and :premises (notanda1))

; cnf_and_pos
(step cnfandpost1 (or (not (and c1 c2 (not c2))) (not c2)) :rule cnf_and_pos :args ((and c1 c2 (not c2)) (not c2) 3))

; cnf_and_neg
(step cnfandnegt1 (or (and c1 c2 (not c2)) (not c1) (not c2) (not (not c2))) :rule cnf_and_neg :args ((and c1 c2 (not c2))))

; cnf_or_neg
(step cnfornegt1 (or c1 c2 (not c2) (not (not c2))) :rule cnf_or_neg :args ((or c1 c2 (not c2)) (not c2) 3))
