; Test tail call optimization

(define (peano a b)
  (if (= a 0)
    b
    (peano (- a 1) (+ b 1))))


(define val (peano 10000 20000))

(display val)
(assert (= val 30000))
 
