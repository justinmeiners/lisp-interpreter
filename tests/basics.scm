
; check some calculations
(assert (= (+ (* 2 4) (- 4 6)) 6))
(define a 3)
(define b (+ a 1))
(assert (= (+ a b (* a b)) 19))


(assert (= (if (and (> b a) (< b (* a b)))
          b
          a) 4))

; and or expansion
(let ((a 1))
  (if (and (= a 0) (garbage here))
    (assert 0)
    'pass)

  (if (or (= a 1) (garbage here))
    'pass
    (assert 0)))

