(assert (= (+ 2 2) 4))
(assert (= (+ (* 2 100) (* 1 10)) 210))

(assert (= (if (> 6 5) (+ 1 1) (+ 2 2)) 2))
(assert (= (if (< 6 5) (+ 1 1) (+ 2 2)) 4)) 

(assert (= (gcd 32 -36) 4))
(assert (= (gcd 4 3) 1))
(assert (= (gcd) 0))

(assert (= (abs -1) 1))

(=> (map + '(1 1 1) '(2 2 2)) (3 3 3))
(=> (map abs '(-1 -2 3)) (1 2 3))
(=> (vector-map abs #(-1 -2 3)) #(1 2 3))

(assert (integer? 3))
(assert (real? 3))

(assert (real? 3.5))
(assert (not (integer? 3.5)))

(assert (< 3 4))
(assert (> 4 3))
(assert (>= 4 3))
(assert (<= 3 4))
(assert (<= 1 1))
(assert (< -5 5))
(assert (not (> 3 4)))

(assert (= (modulo -13 4) 3))
(assert (= (remainder -13 4) -1))

(assert (= (remainder 13 -4) 1))

(assert (even? 2))
(assert (not (odd? 2)))
(assert (odd? 3))
(assert (odd? 7))
(assert (not (odd? 4)))

(assert (exact? (+ 1 2 3)))
(assert (inexact? (+ 1 2.5 3)))
(assert (inexact? (+ 1.3 2 3)))

(assert (exact? (* 1 2 3)))
(assert (inexact? (* 1 2.5 3)))
(assert (inexact? (* 1.3 2 3)))

(assert (exact? (- 1 2)))
(assert (inexact? (- 1 2.5)))
(assert (inexact? (- 1.3 2)))




 
