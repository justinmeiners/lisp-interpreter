; 1.17 - fast multiply

(define (double a) (+ a a))
(define (halve a) (/ a 2))

(define (fast-mul a b) (fast-mul-iter a b 0))

(define (fast-mul-iter a b sum)
  (cond ((= b 0) sum)
    ((even? b) (fast-mul-iter (double a) (halve b) sum))
    (else (fast-mul-iter a (- b 1) (+ sum a)))))

(assert (= (fast-mul 3 4) 12))
(assert (= (fast-mul 100 10) 1000))


