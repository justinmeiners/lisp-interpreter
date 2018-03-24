; Test tail call optimization

(define (peano a b)
  (if (= a 0)
    b
    (peano (- a 1) (+ b 1))))

(assert (= (peano 10000 20000) 30000))
 
