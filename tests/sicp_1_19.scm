
(define (fib-helper n a b p q)
  (cond
       ((= n 0) b)
       ((even? n) (fib-helper (/ n 2)
                              a
                              b
                              (+  (* p p) (* q q))
                              (+ (* 2 q p) (* q q ))
                              ))

       (else
          (fib-helper
            (- n 1)
            (+ (* b q) (* a q) (* a p))
            (+ (* b p) (* a q))
            p
            q))))

(define (fib n)
    (fib-helper n 1 0 0 1))

(assert (= (fib 5) 5))
(assert (= (fib 7) 13))
(assert (= (fib 8) 21))
