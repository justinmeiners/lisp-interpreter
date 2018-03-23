(define (sqr x) (* x x))

(define len 40)
(define area 0)
(set! area (sqr len))

(assert (= area 1600))
