; test closure's and enviornment garbage collection

(def make-account 
  (fn (val) 
    (fn (action) 
      (if (eq? action 'deposit) 
        (fn (n) (set! val (+ val n))) 
        (fn (n) (set! val (- val n)))))))

(def justin (make-account 100))
(def ryan (make-account 200))


(display ((justin 'deposit) 20))
(newline)

(display ((ryan 'withdraw) 20))
(newline)


