; test large memory allocations
; and stack recursion

(define (build-list n)
  (if (= n 0) 
    null 
    (cons n (build-list (- n 1)))))

(display "Alloc 10")
(newline)
(build-list 10)

(display "Alloc 100")
(newline)
(build-list 100)

(display "Alloc 1000")
(build-list 1000)

(display "Alloc 10000")
;(build-list 10000)

(display "Alloc 100000")
;(build-list 100000)
