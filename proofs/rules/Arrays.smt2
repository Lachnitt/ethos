(include "../theories/Arrays.smt2")

(declare-rule arrays_read_over_write ((T Type) (U Type) (i T) (j T) (e U) (a (Array T U)))
  :premises ((not (= i j)))
  :args ((select (store a i e) j))
  :conclusion (= (select (store a i e) j) (select a j)))

(declare-rule arrays_read_over_write_contra ((T Type) (U Type) (i T) (j T) (e U) (a (Array T U)))
  :premises ((not (= (select (store a i e) j) (select a j))))
  :conclusion (= i j))

(declare-rule arrays_read_over_write_1 ((T Type) (U Type) (i T) (e U) (a (Array T U)))
  :args ((select (store a i e) i))
  :conclusion (= (select (store a i e) i) e))

(declare-rule arrays_ext ((T Type) (U Type) (a (Array T U)) (b (Array T U)))
  :premises ((not (= a b)))
  :conclusion (not (= (select a (skolem (@k.ARRAY_DEQ_DIFF a b))) (select b (skolem (@k.ARRAY_DEQ_DIFF a b))))))
