(define (number? x) (real? x)) 
(define (odd? x) (not (even? x))) 
(define (inexact? x) (not (exact? x))) 
(define (zero? x) (= x 0)) 
(define (positive? x) (>= x 0)) 
(define (negative? x) (< x 0)) 

(define (>= a b) (not (< a b))) 
(define (> a b) (< b a)) 
(define (<= a b) (not (< b a))) 

(define (max . ls) 
  (fold-left (lambda (m x) 
               (if (> x m) 
                   x 
                   m)) (car ls) (cdr ls))) 

(define (min . ls) 
  (fold-left (lambda (m x) 
               (if (< x m) 
                   x 
                   m)) (car ls) (cdr ls))) 

(define (_gcd-helper a b) 
  (if (= b 0) a (_gcd-helper b (modulo a b)))) 

(define (gcd . args) 
  (if (null? args) 0 
      (_gcd-helper (car args) (car (cdr args))))) 

(define (lcm . args) 
  (if (null? args) 1 
      (abs (* (/ (car args) (apply gcd args)) 
              (apply * (cdr args))))))  

