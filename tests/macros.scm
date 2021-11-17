
; QUASIQUOTE

(assert (equal? `(1 2 3) '(1 2 3)))

(let ((x 1))
  (assert (equal? `(,x 2 3) '(1 2 3))))

(let ((x 'a))
  (assert (equal? `(,x x ,x) '(a x a))))


; nil! macro

(define-macro nil! (lambda (x)
                     `(set! ,x '())))

(define x 3)
(assert (= x 3))
(nil! x)
(assert (null? x))

; ntimes macro

(define-macro ntimes (lambda (n . body)
        (let ((i (gensym)))
             (cons 'DO (cons `((,i 0 (+ ,i 1)))
                        (cons `((>= ,i ,n) '()) body))
                ))))

(define x 0)
(ntimes 10 (set! x (+ x 1)))
(assert (= x 10))


