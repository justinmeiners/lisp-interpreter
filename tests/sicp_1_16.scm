; 1.16 - fast powers

(define (exp-fast b n) (exp-iter b n 1))

(define (exp-iter b n product)
  (cond ((= n 0) product)
    ; b^n = (b^2) n/2
    ((even? n) (exp-iter (* b b) (/ n 2) product))
    ; b^n = b * b^n-1
    (else (exp-iter b (- n 1) (* product b)))))

(assert (= (exp-fast 5 4) 625))
(assert (= (exp-fast 2 8) 256))


