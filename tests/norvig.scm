; From norvig's tests
(assert (= (+ 2 2) 4))
(assert (= (+ (* 2 100) (* 1 10)) 210))
(assert (= (if (> 6 5) (+ 1 1) (+ 2 2)) 2))
(assert (= (if (< 6 5) (+ 1 1) (+ 2 2)) 4))
(define x 3)
(assert (= x 3))
(assert (= (+ x x) 6))
(assert (= (begin (define x 1) (set! x (+ x 1)) (+ x 1)) 3))
(assert (= ((lambda (x) (+ x x)) 5) 10))
(define twice (lambda (x) (* 2 x)))
(assert (= (twice 5)) 10)
;(define compose (lambda (f g) (lambda (x) (f (g x)))))
;((compose list twice) 5) 



