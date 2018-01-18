(define sqr 
    (lambda (x) (* x x)))

(define length 40)
(define area 0)
(set! area (sqr length))

(assert (= area 1600))
