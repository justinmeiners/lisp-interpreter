(define (integers-starting-from n)
  (cons-stream n (integers-starting-from (+ n 1))))


(assert (equal? (stream-head (integers-starting-from 0) 5) '(0 1 2 3 4)))
