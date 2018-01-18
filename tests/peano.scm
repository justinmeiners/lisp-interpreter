; Test tail call optimization

(define peano
  (lambda (a b)
    (if (= a 0)
      b
      (peano (- a 1) (+ b 1)))))

(assert (= (peano 100 200) 300))
 
