(define make-account 
  (lambda (val) 
    (lambda (action) 
      (if (eq? action 'deposit) 
        (lambda (n) (set! val (+ val n))) 
        (lambda (n) (set! val (- val n)))))))

(define justin (make-account 100))
(define ryan (make-account 200))


(display ((justin 'deposit) 20))
(newline)

(display ((ryan 'withdraw) 20))
(newline)


