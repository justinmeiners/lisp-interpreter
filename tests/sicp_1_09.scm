; 1.09 - peano arithemetic
(define (inc x) (+ x 1))
(define (dec x) (- x 1))

(define (peano-add a b)
    (if (= a 0)
        b
        (peano-add (dec a) (inc b))))

(assert (= (peano-add 4 5) 9))
(assert (= (peano-add 20 10) 30))
(assert (= (peano-add 100000 200000) 300000))


