
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

(gc-flip)

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

