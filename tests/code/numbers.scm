(=> (+ 2 2) 4)
(=> (+ (* 2 100) (* 1 10)) 210)

(=> (if (> 6 5) (+ 1 1) (+ 2 2)) 2)
(=> (if (< 6 5) (+ 1 1) (+ 2 2)) 4) 

(=> (gcd 32 -36) 4)
(=> (gcd 4 3) 1)
(=> (gcd) 0)

(=> (lcm 32 -36) 288)
(assert (exact? (lcm 32 -36)))
(assert (inexact? (lcm 32.0 -36)))

(=> (lcm) 1)

(=> (abs -1) 1)
(=> (map + '(1 1 1) '(2 2 2)) (3 3 3))
(=> (map abs '(-1 -2 3)) (1 2 3))
(=> (vector-map abs #(-1 -2 3)) #(1 2 3))

(=> (- 1) -1)
(=> (- 436) -436)
(=> (- -7) 7)

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

(assert (exact? (expt 3 3)))
(=> (expt 3 3) 27)

(assert (inexact? (expt 3 2.5)))
 
