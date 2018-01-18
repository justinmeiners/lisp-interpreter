; test closure's and enviornment garbage collection

(define make-account 
  (lambda (val) 
    (lambda (action) 
      (if (eq? action 'deposit) 
        (lambda (n) (set! val (+ val n))) 
        (lambda (n) (set! val (- val n)))))))

(define justin (make-account 100))
(define ryan (make-account 200))


((justin 'deposit) 20)
((ryan 'withdraw) 20)

(assert (= ((justin 'withdraw) 0) 120))
(assert (= ((ryan 'deposity) 0) 180))



