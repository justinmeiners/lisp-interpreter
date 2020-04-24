

(define counter 500)
(define big-vector '())

(define (basic-loop)
  (begin
    (set! big-vector (make-vector 100 0))
    (set! counter (- counter 1))
    (gc-collect)
    (print counter)
    (if (> counter 0)

      (basic-loop)
      '())
    )  )


