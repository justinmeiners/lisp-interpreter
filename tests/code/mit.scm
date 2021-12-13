; copy examples from MIT scheme documentation and add related ones.

; Conditionals
; https://www.gnu.org/software/mit-scheme/documentation/stable/mit-scheme-ref/Conditionals.html

(assert (and (= 2 2) (> 2 1)))
(assert (and))

(define (bit-type x)
  (cond ((= x 0) 'OFF)
        ((= x 1) 'ON)
         (else 'UNKNOWN)))

(=> (bit-type 0) OFF)
(=> (bit-type 1) ON)
(=> (bit-type 25) UNKNOWN)

; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_13.html

; Universl Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
(assert (integer? (get-universal-time)))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Procedure-Operations.html#Procedure-Operations
(assert (procedure? (lambda (x) x)))
(assert (compound-procedure? (lambda (x) x)))
(assert (not (compiled-procedure? (lambda (x) x))))
(assert (not (procedure? 3)))
(assert (= 18 (apply + (list 3 4 5 6))))
(assert (compiled-procedure? eval))

(let ((x "hello")
      (y "world"))
(=> (string-append x y) "helloworld"))

(let* ((x 2)
       (y (+ x 1)))
  (=> (+ x y) 5))

(do ((i 0 (+ i 1)))
  ((>= i 10))
  (assert (>= i 0))
  (display i))

(=> (eval '(+ 2 2) (user-initial-environment)) 4)



