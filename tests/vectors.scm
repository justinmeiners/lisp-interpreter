
; test basic vector creation and operations

(define v #(1 2 3 4 5 6 7 8 9 10))

(define (sum-to-n n) (/ (* n (+ n 1)) 2))

(define (sum-vector v)
  (define (iter sum i)
    (if (= i (vector-length v))
        sum
        (iter (+ sum (vector-ref v i)) (+ i 1))))
  (iter 0 0))


(display "Sum to 10: ")
(display (sum-vector v))
(newline)
(assert (= (sum-to-n 10) (sum-vector v)))

        
