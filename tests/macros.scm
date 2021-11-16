
; QUASIQUOTE

(assert (equal? `(1 2 3) '(1 2 3)))

(let ((x 1))
  (assert (equal? `(,x 2 3) '(1 2 3))))

(let ((x 'a))
  (assert (equal? `(,x x ,x) '(a x a))))





