; test large memory allocations
; and stack recursion

(def build-list 
  (fn (n) 
    (if (= n 0) 
      null 
      (cons n (build-list (- n 1))))))

(display "10")
(build-list 10)

(display "100")
(build-list 100)

(display "1000")
(build-list 1000)

