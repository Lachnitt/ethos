(include "./substitution.smt2")

(declare-const a Bool)
(declare-const b Bool)
(declare-const c Bool)
(assume a0 
(let ((a1 (or a b)))
(let ((a2 (or a1 a1)))
(let ((a3 (or a2 a2)))
(let ((a4 (or a3 a3)))
(let ((a5 (or a4 a4)))
(let ((a6 (or a5 a5)))
(let ((a7 (or a6 a6)))
(let ((a8 (or a7 a7)))
(let ((a9 (or a8 a8)))
(let ((a10 (or a9 a9)))
(let ((a11 (or a10 a10)))
(let ((a12 (or a11 a11)))
(let ((a13 (or a12 a12)))
(let ((a14 (or a13 a13)))
(let ((a15 (or a14 a14)))
(let ((a16 (or a15 a15)))
(let ((a17 (or a16 a16)))
(let ((a18 (or a17 a17)))
(let ((a19 (or a18 a18)))
(let ((a20 (or a19 a19)))
(let ((a21 (or a20 a20)))
(let ((a22 (or a21 a21)))
(let ((a23 (or a22 a22)))
(or a23 a23)))))))))))))))))))))))))

                 
(assume a00 (= a b))
(step a3 
(let ((a1 (or b b)))
(let ((a2 (or a1 a1)))
(let ((a3 (or a2 a2)))
(let ((a4 (or a3 a3)))
(let ((a5 (or a4 a4)))
(let ((a6 (or a5 a5)))
(let ((a7 (or a6 a6)))
(let ((a8 (or a7 a7)))
(let ((a9 (or a8 a8)))
(let ((a10 (or a9 a9)))
(let ((a11 (or a10 a10)))
(let ((a12 (or a11 a11)))
(let ((a13 (or a12 a12)))
(let ((a14 (or a13 a13)))
(let ((a15 (or a14 a14)))
(let ((a16 (or a15 a15)))
(let ((a17 (or a16 a16)))
(let ((a18 (or a17 a17)))
(let ((a19 (or a18 a18)))
(let ((a20 (or a19 a19)))
(let ((a21 (or a20 a20)))
(let ((a22 (or a21 a21)))
(let ((a23 (or a22 a22)))
(or a23 a23)))))))))))))))))))))))) :rule eq-subs :premises (a0 a00))
