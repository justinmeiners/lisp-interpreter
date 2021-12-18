
(let ((y "happy days")
      (z #(1 2 3)))
    (gc-flip)
    (assert (string=? y "happy days"))
    (assert (equal? z (vector 1 2 3))))

(let ((x 'HELLO)
      (v #( HELLO ) ))
    (display v)
    (assert (eq? x (vector-ref v 0)))
    (gc-flip)
    (assert (eq? x (vector-ref v 0))))

(define counter 500)
(define big-vector '())

(define (basic-loop)
  (begin
    (set! big-vector (make-vector 200 0))
    (set! counter (- counter 1))
    (vector-fill! big-vector counter)
    (gc-flip)
    (assert (= (vector-ref big-vector 3) counter))
    (if (> counter 0)
      (basic-loop)
      '())
    )  )


(basic-loop)

(==> (call/cc (lambda (throw) (define x '(1 2 3)) (gc-flip) (throw x))) (1 2 3))

(print-gc-statistics)


