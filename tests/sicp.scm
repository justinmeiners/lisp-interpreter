
; A collection of SICP excercises
; these are all my solutions from reading
; the book

; 1.01 - basic expressions
(assert (= (+ (* 2 4) (- 4 6)) 6))
(define a 3)
(define b (+ a 1))
(assert (= (+ a b (* a b)) 19))


(assert (= (if (and (> b a) (< b (* a b)))
          b
          a) 4))

(assert (= (cond ((= a 4) 6) 
             ((= b 4) (+ 6 7 a))
             (else 25)) 16))

(assert (= (+ 2 (if (> b a) b a )) 6))


(assert (= (* (cond ((> a b) a)
                ((< a b) b)
                (else -1))
              (+ a 1)) 16))

; 1.03 - largest squares
(define (sqr x) (* x x))

(define (largest-squares x y z)
  (cond ((and (< z x)  (< z y)) (+ (sqr x) (sqr y)))
    ((and (< y x)  (> y z)) (+ (sqr x) (sqr z)))
    (else (+ (sqr y) (sqr z)))
    ))

(assert (= (largest-squares 3 4 5) (+ 25 16)))
(assert (= (largest-squares 3 5 4) (+ 25 16)))

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

; 1.14 change counter

(define (count-change amount) (cc amount 5))

(define (cc amount kinds-of-coins)
    (cond ((= amount 0) 1)
          ((or (< amount 0) (= kinds-of-coins 0)) 0)
            (else (+ (cc amount
                    (- kinds-of-coins 1))
                    (cc (- amount 
                        (first-denomination kinds-of-coins))
                        kinds-of-coins)))))

(define (first-denomination kinds-of-coins)
    (cond ((= kinds-of-coins 1) 1)
          ((= kinds-of-coins 2) 5)
          ((= kinds-of-coins 3) 10)
          ((= kinds-of-coins 4) 25)
          ((= kinds-of-coins 5) 50)))

(display "counting change: ")
(display (count-change 75))
(newline)


; 1.16 - fast powers

(define (exp-fast b n) (exp-iter b n 1))

(define (exp-iter b n product)
  (cond ((= n 0) product)
    ; b^n = (b^2) n/2
    ((even? n) (exp-iter (* b b) (/ n 2) product))
    ; b^n = b * b^n-1
    (else (exp-iter b (- n 1) (* product b)))))

(assert (= (exp-fast 5 4) 625))
(assert (= (exp-fast 2 8) 256))

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


; 2.21 - square list

(define (square-list items)
    (if (null? items)
        items
        (cons (* (car items) (car items)) (square-list (cdr items)))))

(define (square-list2 items)
    (map (lambda (x) (* x x)) items))

(display (square-list (list 1 2 3 4)))
(newline)
(display (square-list2 (list 4 5 6 7)))

;  bank accounts
(define (make-account val)
    (lambda (action) 
      (if (eq? action 'deposit) 
        (lambda (n) (set! val (+ val n))) 
        (lambda (n) (set! val (- val n))))))

(define justin (make-account 100))
(define ryan (make-account 200))
((justin 'deposit) 20)
((ryan 'withdraw) 20)

(assert (= ((justin 'withdraw) 0) 120))
(assert (= ((ryan 'deposity) 0) 180))


; and or expansion
(let ((a 1))
  (if (and (= a 0) (garbage here))
    (assert 0)
    'pass)

  (if (or (= a 1) (garbage here))
    'pass
    (assert 0)))

