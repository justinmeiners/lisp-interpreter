
; add a test for every bug that is incountered
; to avoid recreating it in the future

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

; procedures with no arguments don't expand properly

(define (hello-world) (display "hello world") (newline))
(hello-world)

; vector and list assoc

(assert
  (= (do ((i 1 (+ i 1)) (n 0 n))
        ((> i 10) n)
        (set! n  (+ i n)))
      (* 5 11)))


(let ((sym (gensym)))
   (assert (eq? sym sym)))

(assert (equal? (cons 2000 1) (cons 2000 1)))

(assert (equal? "apple" "apple"))
(assert (not (eq? 'DEFINE 'DEFINE-MACRO)))

(define (scope-test var)
  (let ((var "dog"))
    (=> var "dog"))
  (=> var "cat"))
(scope-test "cat")

(define (scope-test-named var)
  (let block-name ((var "dog"))
    (=> var "dog"))
  (=> var "cat"))
(scope-test-named "cat")


 
