; From Peter Norvig's Lispy tests
; http://norvig.com/lispy2.html

(define x 3)
(assert (= x 3))
(assert (= (+ x x) 6))
(assert (= (begin (define x 1) (set! x (+ x 1)) (+ x 1)) 3))
(assert (= ((lambda (x) (+ x x)) 5) 10))
(define twice (lambda (x) (* 2 x)))
(assert (= (twice 5) 10))
(define compose (lambda (f g) (lambda (x) (f (g x)))))
(assert (= (car ((compose list twice) 5)) 10))
(define repeat (lambda (f) (compose f f)))
(assert (= ((repeat twice) 5) 20))
(assert (= ((repeat (repeat twice)) 5) 80))
(define fact (lambda (n) (if (< n 2) 1 (* n (fact (- n 1))))))
(assert (= (fact 3) 6))
(define abs (lambda (n) ((if (> n 0) + -) 0 n)))

(assert (= (car (list (abs -3) (abs 0) (abs 3))) 3))

(define combine (lambda (f)
    (lambda (x y)
      (if (null? x) (quote ())
          (f (list (car x) (car y))
             ((combine f) (cdr x) (cdr y)))))))

(define zip (combine cons))
(assert (= (car (cdr (assoc 3 (zip (list 1 2 3 4) (list 5 6 7 8))))) 7))

(define riff-shuffle (lambda (deck) (begin
    (define take (lambda (n seq) (if (< n 1) (quote ()) (cons (car seq) (take (- n 1) (cdr seq))))))
    (define drop (lambda (n seq) (if (< n 1) seq (drop (- n 1) (cdr seq)))))
    (define mid (lambda (seq) (/ (length seq) 2)))
    ((combine append) (take (mid deck) deck) (drop (mid deck) deck)))))

(display (riff-shuffle (list 1 2 3 4 5 6 7 8)))
(newline)
(display ((repeat riff-shuffle) (list 1 2 3 4 5 6 7 8)))
(newline)
(display (riff-shuffle (riff-shuffle (riff-shuffle (list 1 2 3 4 5 6 7 8)))))
(newline)

(define fabs (lambda (n) ((if (> n 0.0) + -) 0.0 n)))

(define (newton guess function derivative epsilon)
    (define guess2 (- guess (/ (function guess) (derivative guess))))
    (if (< (fabs (- guess guess2)) epsilon) guess2
        (newton guess2 function derivative epsilon)))

(define (square-root a)
    (newton 1.0 (lambda (x) (- (* x x) a)) (lambda (x) (* 2.0 x)) 0.0001))

(display "sqrt(2)=")
(display (square-root 2.0))
(newline)

(display "sqrt(200)=")
(display (square-root 200.0))
(newline)

(=> (call/cc (lambda (throw) (+ 5 (* 10 (throw 1))))) 1)
(=> (call/cc (lambda (throw) (+ 5 (* 10 1)))) 15)
(=> (call/cc (lambda (throw) (+ 5 (* 10 (call/cc (lambda (escape) (* 100 (escape 3)))))))) 35)
(=> (call/cc (lambda (throw) (+ 5 (* 10 (call/cc (lambda (escape) (* 100 (throw 3)))))))) 3)
(=> (call/cc (lambda (throw) (+ 5 (* 10 (call/cc (lambda (escape) (* 100 1))))))) 1005)

(=> (let ((a 1) (b 2)) (+ a b)) 3)


