(==> (force (delay (+ 1 2))) 3)

(==> (let ((p (delay (+ 1 2))))
      (list (force p) (force p))) (3 3))

(assert (promise? (delay (+ 1 2))))


; promises computed at most once
(define count 0)

(define p
  (delay
    (begin
        (set! count (+ count 1))
        (* x 3))))

(define x 5)

(==> count 0)
(assert (promise? p))
(==> (force p) 15)
(assert (promise? p))
(==> count 1)
(==> (force p) 15)
(==> count 1)


(define (integers-starting-from n)
  (cons-stream n (integers-starting-from (+ n 1))))

(assert (equal? (stream-head (integers-starting-from 0) 5) '(0 1 2 3 4)))
