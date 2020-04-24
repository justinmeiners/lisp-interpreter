

(define counter 500)
(define big-vector '())

(define (basic-loop)
  (begin
    (set! big-vector (make-vector 100 0))
    (set! counter (- counter 1))
    (gc-flip)
    (if (> counter 0)
      (basic-loop)
      '())
    )  )


(basic-loop)

(print-gc-statistics)

